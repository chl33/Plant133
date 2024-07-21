// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "watering.h"

#include <og3/blink_led.h>
#include <og3/html_table.h>
#include <og3/tasks.h>
#include <og3/units.h>
#include <og3/web_server.h>

#include <algorithm>

#include "watering_constants.h"

namespace og3 {

namespace {
float clamp(float val, float minval, float maxval) {
  if (val > maxval) {
    return maxval;
  }
  if (val < minval) {
    return minval;
  }
  return val;
}

constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;

}  // namespace

const char* Watering::s_state_names[] = {
    "watering",
    "pump",
    "pump done",
    "soil is moist",
    "watering disabled",
    "pump test",
    "water level test",
    "dose check",
    "test",
};

String Watering::StateVariable::string() const { return Watering::s_state_names[m_value]; }
bool Watering::StateVariable::fromString(const String& value) {
  if (fromString(value)) {
    return true;
  }
  for (int i = 0; i <= Watering::kStateTest; i++) {
    if (0 == strcmp(value.c_str(), s_state_names[i])) {
      m_value = static_cast<Watering::State>(i);
      return true;
    }
  }
  return false;
}

Watering::Watering(const char* name, uint8_t moisture_pin, uint8_t mode_led, uint8_t pump_ctl_pin,
                   HAApp* app)
    : Module(name, &app->module_system()),
      m_app(app),
      m_dependencies({ConfigInterface::kName, ReservoirCheck::kName, OledDisplayRing::kName}),
      m_cfg_vg(name, VariableGroup::VarNameType::kWithGroup),
      m_vg(name, VariableGroup::VarNameType::kWithGroup),
      m_status_url(String("/") + name + "/status"),
      m_config_url(String("/") + name + "/config"),
      m_pump_test_url(String("/") + name + "/pump"),
      m_moisture("soil_moisture", moisture_pin, "raw moisture reading", "soil moisture %",
                 &app->module_system(), &m_cfg_vg, &m_vg),
      m_pump("pump", &app->tasks(), pump_ctl_pin, "pump state", true, &m_vg, Relay::OnLevel::kHigh),
      m_mode_led("mode_led", mode_led, app, 100 /*msec-on*/, false /*onLow*/),
      m_max_moisture_target("max_moisture_target", 80.0f, units::kPercentage, "Max moisture",
                            kCfgSet, 0, &m_cfg_vg),
      m_min_moisture_target("min_moisture_target", 70.0f, units::kPercentage, "Min moisture",
                            kCfgSet, 0, &m_cfg_vg),
      m_pump_dose_msec("pump_on_msec", 3 * kMsecInSec, units::kMilliseconds, "Pump on time",
                       kCfgSet, 0, &m_cfg_vg),
      m_state("watering_state", kStateWaitForNextCycle, "", "watering state", 0, &m_vg),
      m_watering_enabled("watering_enabled", false, "watering enabled", kCfgSet, &m_cfg_vg),
      m_reservoir_check_enabled("res_check_enabled", false, "reservior check enabled", kCfgSet,
                                &m_cfg_vg),
      m_sec_since_dose("sec_since_pump", 0, units::kSeconds, "seconds since pump dose", 0, 0,
                       &m_vg) {
  setDependencies(&m_dependencies);
  // 10 seconds after boot, start the plant state machine.
  m_next_update_msec = millis() + 10 * kMsecInSec;
  add_link_fn([this](og3::NameToModule& name_to_module) -> bool {
    m_config = ConfigInterface::get(name_to_module);
    m_reservoir_check = ReservoirCheck::get(name_to_module);
    m_oled = OledDisplayRing::get(name_to_module);
    return true;
  });
  add_init_fn([this]() {
    if (m_config) {
      m_config->read_config(&m_cfg_vg);
    }

    auto* ha_discovery = m_dependencies.ha_discovery();
    if (m_dependencies.mqtt_manager() && ha_discovery) {
      ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_state, ha::device_type::kSensor, nullptr);
      });
      ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_moisture.filter().valueVariable(), ha::device_type::kSensor,
                            ha::device_class::sensor::kMoisture);
      });
      ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_moisture.adc().mapped_value(), ha::device_type::kSensor,
                            ha::device_class::sensor::kMoisture);
      });
      ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_pump.isHighVar(), ha::device_type::kBinarySensor,
                            ha::device_class::binary_sensor::kPower);
      });
      ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_sec_since_dose, ha::device_type::kSensor,
                            ha::device_class::sensor::kDuration);
      });
    }
    m_oled->addDisplayFn([this]() {
      char txt[80];
      snprintf(txt, sizeof(txt), "moisture: %.1f%%", m_moisture.filteredValue());
      m_oled->display(txt);
    });
  });
  add_update_fn([this]() { loop(); });
  m_app->web_server().on(statusUrl(), [this](AsyncWebServerRequest* request) {
    this->handleStatusRequest(request);
  });
  m_app->web_server().on(configUrl(), [this](AsyncWebServerRequest* request) {
    this->handleConfigRequest(request);
  });
  m_app->web_server().on(pumpTestUrl(), [this](AsyncWebServerRequest* request) {
    testPump();
    request->redirect(statusUrl());
  });
}

void Watering::setPumpEnable(bool enable) {
  if (enable) {
    setState(kStateWaitForNextCycle, kMsecInSec, "set pump enabled");
  } else {
    setState(kStateDisabled, 100, "set pump disabled");
  }
}

void Watering::loop() {
  // Detect updated watering enable.
  if (m_watering_enabled.value()) {
    if (state() == kStateDisabled) {
      // Detected watering enabled.
      setPumpEnable(true);
    }
  } else {
    if (state() != kStateDisabled) {
      // Detected watering disabled.
      setPumpEnable(false);
    }
  }

  const auto nowMsec = millis();

  if (nowMsec < m_next_update_msec) {
    return;
  }
  m_next_update_msec = nowMsec + static_cast<unsigned long>(2000);

  // Read sensors.
  if (m_reservoir_check) {
    m_reservoir_check->read();
  }

  const long msecSincePump = nowMsec - m_pump.lastOnMsec();
  m_sec_since_dose = msecSincePump * 1e-3;
  const bool shouldReadMoisture = msecSincePump >= kWaitBetweenPumpAndMoisureReadingMsec;
  // We found that if we read moisture level after pump has been running, the reading
  //  is significantly lower, so a reference voltage must be dropping.
  if (shouldReadMoisture) {
    if (m_state.value() == kStateWaitForNextCycle) {
      // After watering, increase the kernel size from the watering amount to
      //  the amount used between watering, but don't weigh data from
      //  before the watering by more than they were during watering mode.
      // The time in seconds that the state machine might have switched out of watering mode.
      const float stateChangeSec =
          static_cast<float>(m_pump.lastOnMsec() + kPumpOffMsec) / kMsecInSec;
      const float secSinceStateChange = static_cast<float>(nowMsec) / kMsecInSec - stateChangeSec;
      // The growing sigma value that should be kKernelWateringSec when the state changed.
      const float sigma1 = secSinceStateChange + kKernelWateringSec;
      // Keep sigma between the minimum and maximum values.
      const float sigma = clamp(sigma1, kKernelWateringSec, kKernelNotWateringSec);
      m_moisture.setSigma(sigma);
    } else {
      m_moisture.setSigma(kKernelWateringSec);
    }

#if 0
    if (m_shtc3.ok() && !m_shtc3.temperatureVar().failed()) {
      // Add current temperature, adjust moisture reading.
      m_moisture.setTempC(m_shtc3.temperature());
    }
#endif
    m_moisture.read(nowMsec);
    const float level = m_moisture.filteredValue();
    if (level > m_max_moisture_target.value()) {
      m_mode_led.delayedBlink(2000, 2);
    } else if (level < m_min_moisture_target.value()) {
      m_mode_led.delayedBlink(2000, 3);
    } else {
      m_mode_led.delayedBlink(2000, 1);
    }
  }

  // Run the state machine.
  switch (state()) {
    case kStateEval: {
      // Check moisture level, reservior level during watering cycle.
      // Make sure the pump is off.
      m_pump.turnOff();
      // Don't consider turning the pump back on until it has been off for the
      //  required amount of time.
      if (msecSincePump < kPumpOffMsec) {
        setState(kStateEval, kWaitForNextCycleMsec, "pump not off for long enough");
      } else if (reservoirCheckEnabled() && isReservoirEmpty()) {
        // Reservoir may be low, so don't consider using the pump.
        setState(kStateEval, kWaitForNextCycleMsec, "reservoir too low");
      } else if (m_moisture.readingIsFailed()) {
        setState(kStateDisabled, 1, "failed reading moisture sensor");
      } else if (m_moisture.filteredValue() < kMinPlausibleMoisture) {
        log()->logf("Moisture sensor reading is too low (%.1f < %.1f).", m_moisture.filteredValue(),
                    kMinPlausibleMoisture);
        setState(kStateDisabled, 1, "moisture level implausibly low");
      } else {
        // Check whether to turn on the pump.
        const float val = m_moisture.filteredValue();
        if (val > m_max_moisture_target.value()) {
          // Moisure level is above the maximum threshold, so switch the a state
          //  where we wait for it to fall back below the minimum to start the
          //  watering cycle again.
          setState(kStateWaitForNextCycle, kWaitForNextCycleMsec, "moisture past maximum range");
        } else {
          setState(kStateDose, 1, "start pump");
        }
      }

      break;
    }

    case kStateDose:
      // Start the pump, and run for kPumpOnMsec.
      m_pump.turnOn();
      setState(kStateEndOfDose, m_pump_dose_msec.value(), "end watering dose");
      break;

    case kStateEndOfDose:
      // Turn pump off at end of dose.
      m_pump.turnOff();
      if (m_reservoir_check) {
        m_reservoir_check->pumpRanForMsec(m_pump_dose_msec.value());
      }
      // Eval mode will wait until kPumpOffMsec until it will allow the pump to run again.
      setState(kStateEval, kWaitForNextCycleMsec, "continue watering");
      break;

    case kStateWaitForNextCycle: {
      // After moisture level reaches maximum level, wait for it to reach minimum
      //  moisture level and also wait for minimum time between watering cycles, then
      //  go back to kStateEval.
      const float val = m_moisture.filteredValue();
      if (val < m_min_moisture_target.value()) {
        setState(kStateEval, 1, "start watering");
      } else {
        setState(kStateWaitForNextCycle, kWaitForNextCycleMsec, "");
      }
      break;
    }

    case kStateDisabled:
      // State for when pump is disabled.
      m_pump.turnOff();
      // Update every 10 seconds to get latest readings.
      setState(kStateDisabled, 10 * kMsecInSec, "");
      break;

    case kStatePumpTest:
      // State for when testing a single pump cycle, transitions to disabled mode.
      m_pump.turnOn();
      setState(kStateDisabled, m_pump_dose_msec.value(), "end of pump test");
      break;

    case kStateTest:
      _fullTest();
      setState(kStateDisabled, kMsecInSec, "end of  test");
      break;
  }

  m_app->mqttSend(m_vg);
  if (m_reservoir_check) {
    m_reservoir_check->mqttUpdate();
  }
}

void Watering::setState(State state, unsigned msec, const char* msg) {
  if (m_state.value() != state) {
    log()->logf("%s -> %s in %d.%03d: %s.", s_state_names[m_state.value()], s_state_names[state],
                msec / 1000, msec % 1000, msg);
  } else {
    log()->debugf("%s -> %s in %d.%03d: %s.", s_state_names[m_state.value()], s_state_names[state],
                  msec / 1000, msec % 1000, msg);
  }
  m_state = state;
  m_next_update_msec = millis() + static_cast<unsigned long>(msec);
}

void Watering::_fullTest() {
  // test
  m_moisture.read(millis());
  log()->logf("Moisture: %s: %d, %.1f", m_moisture.readingIsFailed() ? "NOT OK" : "OK",
              m_moisture.rawCounts(), m_moisture.filteredValue());
  if (m_reservoir_check) {
    m_reservoir_check->read();
    log()->logf("WaterLevel %s", m_reservoir_check->floatIsFloating() ? "OK" : "LOW");
  }
  m_mode_led.on();
  delay(100);
  m_mode_led.off();
}

void Watering::handleStatusRequest(AsyncWebServerRequest* request) {
#ifndef NATIVE
  m_html.clear();
  html::writeTableInto(&m_html, variables());
  add_html_button(&m_html, "Configure", configUrl());  
  add_html_button(&m_html, "Test pump", pumpTestUrl());
  m_html += F(HTML_BUTTON("/", "Back"));
  sendWrappedHTML(request, m_app->board_cname(), this->name(), m_html.c_str());
#endif
}
void Watering::handleConfigRequest(AsyncWebServerRequest* request) {
#ifndef NATIVE
  ::og3::read(*request, &m_cfg_vg);
  m_html.clear();
  html::writeFormTableInto(&m_html, m_cfg_vg);
  m_html += F(HTML_BUTTON("/", "Back"));
  sendWrappedHTML(request, m_app->board_cname(), this->name(), m_html.c_str());
  if (m_config) {
    m_config->write_config(m_cfg_vg);
  }
#endif
}

}  // namespace og3
