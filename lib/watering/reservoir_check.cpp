// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "reservoir_check.h"

#include "og3/ha_discovery.h"
#include "watering_constants.h"

namespace og3 {
namespace {
constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;

}
const char ReservoirCheck::kName[] = "reservoir";

ReservoirCheck::ReservoirCheck(uint8_t pin, HAApp* app_)
    : Module(kName, &app_->module_system()),
      m_app(app_),
      m_deps({ConfigInterface::kName, OledDisplayRing::kName}),
      m_cfg_vg(kName),
      m_vg(kName),
      m_din("reservoir", &m_app->module_system(), pin, "reservoir has water", &m_vg, true),
      m_pump_seconds_after_low("pump_after_low", kLowWaterSecsRemaining, units::kSeconds,
                               "pump seconds after low water", kCfgSet, 0, &m_cfg_vg),
      m_pump_seconds_remaining("pump_sec_left", kLowWaterSecsRemaining, units::kSeconds,
                               "reservoir seconds left", 0, 0, &m_vg) {
  setDependencies(&m_deps);
  add_link_fn([this](og3::NameToModule& name_to_module) -> bool {
    m_config = ConfigInterface::get(name_to_module);
    m_oled = OledDisplayRing::get(name_to_module);
    return true;
  });
  add_init_fn([this]() {
    if (m_deps.mqtt_manager() && m_deps.ha_discovery()) {
      m_deps.ha_discovery()->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addBinarySensor(json, m_din.isHighVar(),
                                    ha::device_class::binary_sensor::kMoisture);
      });
      m_deps.ha_discovery()->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_pump_seconds_remaining, ha::device_type::kSensor,
                            ha::device_class::sensor::kDuration);
      });
      if (m_oled) {
        m_oled->addDisplayFn([this]() {
          if (!floatIsFloating()) {
            m_oled->display("Fill reservoir.");
          }
        });
      }
    }
  });
}

void ReservoirCheck::read() {
  m_din.read();
  if (floatIsFloating()) {
    m_pump_seconds_remaining = m_pump_seconds_after_low.value();
  }
}
void ReservoirCheck::pumpRanForMsec(float msecs) {
  if (!floatIsFloating()) {
    const float remaining = m_pump_seconds_remaining.value() - 1.0e-3 * msecs;
    m_pump_seconds_remaining = remaining > 0.0f ? remaining : 0.0f;
  }
}

}  // namespace og3
