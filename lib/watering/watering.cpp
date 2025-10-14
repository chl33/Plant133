// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "watering.h"

#include <og3/blink_led.h>
#include <og3/html_table.h>
#include <og3/tasks.h>
#include <og3/units.h>
#include <og3/web_server.h>

#include <algorithm>

#include "ArduinoJson/Variant/JsonVariant.hpp"
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

int wateringDirection(Watering::State state) {
  switch (state) {
    case Watering::State::kStateEval:
    case Watering::State::kStateDose:
    case Watering::State::kStateEndOfDose:
      return +1;

    case Watering::State::kStateWaitForNextCycle:
      return -1;

    case Watering::State::kStateWateringPaused:
    case Watering::State::kStateDisabled:
    case Watering::State::kStatePumpTest:
    case Watering::State::kStateTest:
      return 0;
  }
  return 0;
}

bool isWatering(Watering::State state) { return wateringDirection(state) == 1; }

}  // namespace

const char* Watering::s_state_names[] = {
    "watering",           // kStateEval
    "pump",               // kStateDose
    "pump done",          // kStateEndOfDose
    "soil is moist",      // kStateWaitForNextCycle
    "watering paused",    // kStateWateringPaused
    "watering disabled",  // kStateDisabled
    "pump test",          // kStatePumpTest
    "test",               // kStateTest
};

// static
int Watering::direction() const { return wateringDirection(m_state.value()); }

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

const char* Watering::varname(const char* elname, std::string* str) {
  *str = std::string(name()) + "_" + elname;
  return str->c_str();
}

Watering::Watering(unsigned index, const char* name, uint8_t moisture_pin, uint8_t mode_led,
                   uint8_t pump_ctl_pin, HAApp* app)
    : Module(name, &app->module_system()),
      m_app(app),
      m_dependencies({ConfigInterface::kName, ReservoirCheck::kName}),
      m_index(index),
      m_cfg_vg(name),
      m_vg(name),
      m_status_url(String("/") + name + "/status"),
      m_config_url(String("/") + name + "/config"),
      m_pump_test_url(String("/") + name + "/pump"),
      m_moisture(varname("soil_moisture", &m_moisture_varname), moisture_pin,
                 "raw moisture reading", "soil moisture %", &app->module_system(), m_cfg_vg, m_vg),
      m_pump(varname("pump", &m_pump_varname), &app->tasks(), pump_ctl_pin, "pump state", true,
             m_vg, Relay::OnLevel::kHigh),
      m_mode_led("mode_led", mode_led, app, 100 /*msec-on*/, false /*onLow*/),
      m_dose_log(m_vg, m_cfg_vg, &app->module_system(), this),
      m_plant_name("name", name, nullptr, nullptr, kCfgSet, m_cfg_vg),
      m_max_moisture_target("max_moisture_target", 80.0f, units::kPercentage, "Max moisture",
                            kCfgSet, 0, m_cfg_vg),
      m_min_moisture_target("min_moisture_target", 70.0f, units::kPercentage, "Min moisture",
                            kCfgSet, 0, m_cfg_vg),
      m_pump_dose_msec("pump_on_msec", 3 * kMsecInSec, units::kMilliseconds, "Pump on time",
                       kCfgSet, 0, m_cfg_vg),
      m_between_doses_sec("between_doses_sec", kPumpOffSec, units::kSeconds, "Wait between doses",
                          kCfgSet, 0, m_cfg_vg),
      m_state(varname("watering_state", &m_watering_varname), kStateWaitForNextCycle,
              "watering state", 0, m_vg),
      m_sec_since_dose(varname("sec_since_pump", &m_sec_dose_varname), 0, units::kSeconds,
                       "seconds since pump dose", 0, 0, m_vg),
      m_watering_enabled("watering_enabled", false, "watering enabled", kCfgSet, m_cfg_vg),
      m_reservoir_check_enabled("res_check_enabled", false, "reservior check enabled", kCfgSet,
                                m_cfg_vg) {
  setDependencies(&m_dependencies);
  // 10 seconds after boot, start the plant state machine.
  m_next_update_msec = millis() + (10 + 15 * index) * kMsecInSec;
  add_link_fn([this](og3::NameToModule& name_to_module) -> bool {
    m_config = ConfigInterface::get(name_to_module);
    m_reservoir_check = ReservoirCheck::get(name_to_module);
    return true;
  });
  add_init_fn([this]() {
    if (m_config) {
      m_config->read_config(m_cfg_vg);
    }

    if (!m_watering_enabled.value()) {
      return;
    }

    auto addEntry = [this](HADiscovery::Entry& entry, HADiscovery* had, JsonDocument* json) {
      char device_id[80];
      snprintf(device_id, sizeof(device_id), "%s_%s", had->deviceId(), this->name());
      entry.device_name = this->plantName().c_str();
      entry.device_id = device_id;
      entry.via_device = had->deviceId();
      return had->addEntry(json, entry);
    };

    auto* ha_discovery = m_dependencies.ha_discovery();
    if (m_dependencies.mqtt_manager() && ha_discovery) {
      ha_discovery->addDiscoveryCallback([this, addEntry](HADiscovery* had, JsonDocument* json) {
        HADiscovery::Entry entry(m_state, ha::device_type::kSensor);
        return addEntry(entry, had, json);
      });
      ha_discovery->addDiscoveryCallback([this, addEntry](HADiscovery* had, JsonDocument* json) {
        HADiscovery::Entry entry(m_moisture.filter().valueVariable(), ha::device_type::kSensor,
                                 ha::device_class::sensor::kMoisture);
        return addEntry(entry, had, json);
      });
      ha_discovery->addDiscoveryCallback([this, addEntry](HADiscovery* had, JsonDocument* json) {
        HADiscovery::Entry entry(m_moisture.adc().mapped_value(), ha::device_type::kSensor,
                                 ha::device_class::sensor::kMoisture);
        return addEntry(entry, had, json);
      });
      ha_discovery->addDiscoveryCallback([this, addEntry](HADiscovery* had, JsonDocument* json) {
        HADiscovery::Entry entry(m_pump.isHighVar(), ha::device_type::kBinarySensor,
                                 ha::device_class::binary_sensor::kPower);
        return addEntry(entry, had, json);
      });
      ha_discovery->addDiscoveryCallback([this, addEntry](HADiscovery* had, JsonDocument* json) {
        HADiscovery::Entry entry(m_sec_since_dose, ha::device_type::kSensor,
                                 ha::device_class::sensor::kDuration);
        return addEntry(entry, had, json);
      });
      m_dose_log.addHADiscovery(ha_discovery);
    }
  });
  add_update_fn([this]() { loop(); });
  m_app->web_server().on(
      statusUrl(), [this](AsyncWebServerRequest* request) { this->handleStatusRequest(request); });
  m_app->web_server().on(
      configUrl(), [this](AsyncWebServerRequest* request) { this->handleConfigRequest(request); });
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

  // Add a new dose record if newly watering, and expire dose records more than a day old
  //  when not watering.
  m_dose_log.update(isWatering(state()));

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
          static_cast<float>(m_pump.lastOnMsec()) / kMsecInSec + kPumpOffSec;
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
    if (m_index == 0) {
      const float level = m_moisture.filteredValue();
      if (level > m_max_moisture_target.value()) {
        m_mode_led.delayedBlink(2000, 2);
      } else if (level < m_min_moisture_target.value()) {
        m_mode_led.delayedBlink(2000, 3);
      } else {
        m_mode_led.delayedBlink(2000, 1);
      }
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
      if (msecSincePump < (m_between_doses_sec.value() * kMsecInSec)) {
        setState(kStateEval, kWaitForNextCycleMsec, "pump not off for long enough");
      } else if (reservoirCheckEnabled() && isReservoirEmpty()) {
        // Reservoir may be low, so don't consider using the pump.
        setState(kStateEval, kWaitForNextCycleMsec, "reservoir too low");
      } else if (m_moisture.readingIsFailed()) {
        setState(kStateDisabled, 1, "failed reading moisture sensor");
      } else if (m_moisture.filteredValue() < kMinPlausibleMoisture) {
        log()->logf("plant%d: Moisture sensor reading is too low (%.1f < %.1f).", m_index,
                    m_moisture.filteredValue(), kMinPlausibleMoisture);
        setState(kStateDisabled, 1, "moisture level implausibly low");
      } else {
        // Check whether to turn on the pump.
        const float val = m_moisture.filteredValue();
        if (val > m_max_moisture_target.value()) {
          // Moisure level is above the maximum threshold, so switch to the state
          //  where we wait for it to fall back below the minimum to start the
          //  watering cycle again.
          setState(kStateWaitForNextCycle, kWaitForNextCycleMsec, "moisture past maximum range");
        } else if (m_dose_log.shouldPauseWatering()) {
          setState(kStateWateringPaused, kWaitForNextCycleMsec, "too many doses in cycle");
        } else {
          setState(kStateDose, 1, "start pump");
        }
      }

      break;
    }

    case kStateDose:
      // Start the pump, and run for kPumpOnMsec.
      m_pump.turnOn();
      m_dose_log.addDose();
      setState(kStateEndOfDose, m_pump_dose_msec.value(), "end watering dose");
      break;

    case kStateEndOfDose:
      // Turn pump off at end of dose.
      m_pump.turnOff();
      if (m_reservoir_check) {
        m_reservoir_check->pumpRanForMsec(m_pump_dose_msec.value());
      }
      // Eval mode will wait until kPumpOffSec until it will allow the pump to run again.
      setState(kStateEval, kWaitForNextCycleMsec, "continue watering");
      break;

    case kStateWateringPaused: {
      const float val = m_moisture.filteredValue();
      m_pump.turnOff();
      if (val > m_max_moisture_target.value()) {
        // Moisure level is above the maximum threshold, so switch to the state
        //  where we wait for it to fall back below the minimum to start the
        //  watering cycle again.
        setState(kStateWaitForNextCycle, kWaitForNextCycleMsec, "moisture past maximum range");
      } else if (m_dose_log.shouldPauseWatering()) {
        // Stay in the paused state until it times-out.
        setState(kStateWateringPaused, kWaitForNextCycleMsec, "");
      } else {
        // If the pause time expires, go back to eval state.
        setState(kStateEval, 1, "re-enable watering after pause");
      }
      break;
    }

    case kStateWaitForNextCycle: {
      // After moisture level reaches maximum level, wait for it to reach minimum
      //  moisture level and also wait for minimum time between watering cycles, then
      //  go back to kStateEval.
      const float val = m_moisture.filteredValue();
      m_pump.turnOff();
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
      m_pump.turnOff();
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
    // The watering state changed.
    log()->logf("plant%u: %s -> %s in %d.%03d: %s.", m_index, s_state_names[m_state.value()],
                s_state_names[state], msec / 1000, msec % 1000, msg);
  } else {
    // The watering state is staying the same.
    log()->debugf("plant%u: %s -> %s in %d.%03d: %s.", m_index, s_state_names[m_state.value()],
                  s_state_names[state], msec / 1000, msec % 1000, msg);
  }
  m_state = state;
  // If we don't update m_watering_enabled, kStateDisabled will only last until the next update().
  m_watering_enabled = (m_state.value() != kStateDisabled);
  m_next_update_msec = millis() + static_cast<unsigned long>(msec);
}

void Watering::_fullTest() {
  // test
  m_moisture.read(millis());
  log()->logf("plant%u: moisture: %s: %d, %.1f", m_index,
              m_moisture.readingIsFailed() ? "NOT OK" : "OK", m_moisture.rawCounts(),
              m_moisture.filteredValue());
  if (m_reservoir_check) {
    m_reservoir_check->read();
    log()->logf("plant%u: waterLevel %s", m_index,
                m_reservoir_check->floatIsFloating() ? "OK" : "LOW");
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
  m_html += HTML_BUTTON("/", "Back");
  sendWrappedHTML(request, m_app->board_cname(), this->name(), m_html.c_str());
#endif
}
void Watering::handleConfigRequest(AsyncWebServerRequest* request) {
#ifndef NATIVE
  ::og3::read(*request, m_cfg_vg);
  m_html.clear();
  html::writeFormTableInto(&m_html, m_cfg_vg);
  add_html_button(&m_html, "Back", statusUrl());
  sendWrappedHTML(request, m_app->board_cname(), this->name(), m_html.c_str());
  if (m_config) {
    m_config->write_config(m_cfg_vg);
  }
#endif
}

void Watering::getApiPlants(JsonObject json) const {
  json["name"] = plantName();
  json["minMoisture"] = minTarget();
  json["maxMoisture"] = maxTarget();
  json["adc0"] = m_moisture.adc().in_min();
  json["adc100"] = m_moisture.adc().in_max();
  json["enabled"] = isEnabled();
  json["currentMoisture"] = moisturePercent();
  json["pumpOnTime"] = m_pump_dose_msec.value();
  json["secsBetweenDoses"] = m_between_doses_sec.value();
  json["maxDosesPerCycle"] = m_dose_log.maxDoesPerCycle();
  json["doseCount"] = m_dose_log.dose_count();
  json["state"] = m_state.string();
}

namespace {
template <typename T>
bool getVal(JsonObject json, const char* name, const std::function<void(const T& val)>& fn) {
  const JsonVariant var = json[name];
  if (!var.is<T>()) {
    return false;
  }
  fn(var.as<T>());
  return true;
}
}  // namespace

bool Watering::putApiPlants(JsonObject json) {
  const bool res =
      getVal<const char*>(json, "name", [this](const char* name) { m_plant_name = name; }) &&
      getVal<int>(json, "minMoisture", [this](const int& val) { m_min_moisture_target = val; }) &&
      getVal<int>(json, "maxMoisture", [this](const int& val) { m_max_moisture_target = val; }) &&
      getVal<int>(json, "adc0", [this](const int& val) { m_moisture.adc().set_in_min(val); }) &&
      getVal<int>(json, "adc100", [this](const int& val) { m_moisture.adc().set_in_max(val); }) &&
      getVal<int>(json, "pumpOnTime", [this](const int& val) { m_pump_dose_msec = val; }) &&
      getVal<int>(json, "secsBetweenDoses",
                  [this](const int& val) { m_between_doses_sec = val; }) &&
      getVal<int>(json, "maxDosesPerCycle",
                  [this](const int& val) { m_dose_log.setMaxDoesPerCycle(val); }) &&
      getVal<bool>(json, "enabled", [this](const bool& val) { m_watering_enabled = val; });
  if (res && m_config) {
    m_config->write_config(m_cfg_vg);
  }
  return true;
}

}  // namespace og3
