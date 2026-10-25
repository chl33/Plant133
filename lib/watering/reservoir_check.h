// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <og3/config_interface.h>
#include <og3/din.h>
#include <og3/ha_app.h>
#include <og3/oled_display_ring.h>

namespace og3 {

// This module tracks the state of the reservoir level with a float-sensor.
// The float sensor is checked with a digital input pin.
class ReservoirCheck : public Module {
 public:
  ReservoirCheck(uint8_t pin, HAApp* app_);
  ReservoirCheck(ReservoirCheck&) = delete;
  ReservoirCheck& operator=(ReservoirCheck&) = delete;

  void read();
  bool floatIsFloating() const { return m_din.isHigh(); }
  float secondsRemaining() const { return m_pump_seconds_remaining.value(); }
  bool haveWater() const { return floatIsFloating() || secondsRemaining() > 0.0f; }
  void pumpRanForMsec(float secs);
  const VariableGroup& variables() const { return m_vg; }
  const VariableGroup& configVariables() const { return m_cfg_vg; }

  static const char kName[];
  static const char kConfigUrl[];

  void mqttUpdate();
  void add_html_status_button(String* body) const { add_html_button(body, name(), "/config"); }

 private:
  NetHandlerStatus handleConfigRequest(NetRequest* request, NetResponse* response);

  HAApp* const m_app;
  VariableGroup m_cfg_vg;
  VariableGroup m_vg;
  String m_din_name;
  DIn m_din;
  FloatVariable m_pump_seconds_after_low;
  FloatVariable m_pump_seconds_remaining;
  ConfigInterface* m_config = nullptr;
  MqttManager* m_mqtt_manager = nullptr;
  HADiscovery* m_ha_discovery = nullptr;
  OledDisplayRing* m_oled = nullptr;
  String m_html;
};

}  // namespace og3
