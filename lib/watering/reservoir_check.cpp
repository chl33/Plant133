// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "reservoir_check.h"

#include <og3/ha_discovery.h>
#include <og3/html_table.h>
#include <og3/mqtt_manager.h>

#include "watering_constants.h"

namespace og3 {
namespace {
constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;

}
const char ReservoirCheck::kName[] = "reservoir_check";

ReservoirCheck::ReservoirCheck(uint8_t pin, HAApp* app_)
    : Module(kName, &app_->module_system()),
      m_app(app_),
      m_cfg_vg(kName),
      m_vg(kName),
      m_din("reservoir", &m_app->module_system(), pin, "reservoir has water", m_vg, true),
      m_pump_seconds_after_low("pump_after_low", kLowWaterSecsRemaining, units::kSeconds,
                               "pump seconds after low water", kCfgSet, 0, m_cfg_vg),
      m_pump_seconds_remaining("pump_sec_left", kLowWaterSecsRemaining, units::kSeconds,
                               "reservoir seconds left", 0, 0, m_vg) {
  require(ConfigInterface::kName, &m_config);
  require(MqttManager::kName, &m_mqtt_manager);
  require(HADiscovery::kName, &m_ha_discovery);
  require(OledDisplayRing::kName, &m_oled);
  add_init_fn([this]() {
    if (m_mqtt_manager && m_ha_discovery) {
      m_ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addBinarySensor(json, m_din.isHighVar(),
                                    ha::device_class::binary_sensor::kMoisture);
      });
      m_ha_discovery->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
        return had->addMeas(json, m_pump_seconds_remaining, ha::device_type::kSensor,
                            ha::device_class::sensor::kDuration);
      });
    }
    m_app->web_server_module().on("/config", [this](NetRequest* request, NetResponse* response) {
      return this->handleConfigRequest(request, response);
    });
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
void ReservoirCheck::mqttUpdate() {
  if (m_mqtt_manager && m_mqtt_manager->mode() != MqttManager::Mode::kAdafruitIO) {
    m_app->mqttSend(m_vg);
  }
}

NetHandlerStatus ReservoirCheck::handleConfigRequest(NetRequest* request, NetResponse* response) {
#ifndef NATIVE
  ::og3::read(*request, m_cfg_vg);
  String html;
  html::writeFormTableInto(&html, m_cfg_vg);
  add_html_button(&html, "Back", "/");
  sendWrappedHTML(request, response, m_app->board_cname(), this->name(), html.c_str());
  if (m_config) {
    m_config->write_config(m_cfg_vg);
  }
#endif
  NET_REPLY(request, ESP_OK);
}

}  // namespace og3
