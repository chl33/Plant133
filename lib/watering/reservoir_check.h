// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/config_interface.h>
#include <og3/din.h>
#include <og3/ha_app.h>
#include <og3/ha_dependencies.h>
#include <og3/oled_display_ring.h>

namespace og3 {

// This module tracks the state of the reservoir level with a float-sensor.
// The float sensor is checked with a digital input pin.
class ReservoirCheck : public Module {
 public:
  ReservoirCheck(uint8_t pin, HAApp* app_);

  void read();
  bool floatIsFloating() const { return m_din.isHigh(); }
  float secondsRemaining() const { return m_pump_seconds_remaining.value(); }
  bool haveWater() const { return floatIsFloating() || secondsRemaining() > 0.0f; }
  void pumpRanForMsec(float secs);
  const VariableGroup& variables() const { return m_vg; }

  static const char kName[];
  static const char kConfigUrl[];

  static ReservoirCheck* get(const NameToModule& n2m) {
    return GetModule<ReservoirCheck>(n2m, kName);
  }

  void mqttUpdate() { m_app->mqttSend(m_vg); }

 private:
  HAApp* const m_app;
  HADependenciesArray<2> m_deps;
  VariableGroup m_cfg_vg;
  VariableGroup m_vg;
  DIn m_din;
  FloatVariable m_pump_seconds_after_low;
  FloatVariable m_pump_seconds_remaining;
  ConfigInterface* m_config = nullptr;
  OledDisplayRing* m_oled = nullptr;
};

}  // namespace og3
