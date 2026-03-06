// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>
#include <og3/constants.h>
#include <og3/ha_app.h>
#include <og3/html_table.h>
#include <og3/oled_wifi_info.h>
#include <og3/shtc3.h>
#include <og3/units.h>
#include <og3/variable.h>
#include <og3/wifi_watchdog.h>

#include <algorithm>
#include <array>

#include "svelteesp32async.h"
#include "watering.h"

#define SW_VERSION "0.9.6"

namespace {

const char kManufacturer[] = "Chris Lee";
const char kModel[] = "Plantl337";
const char kSoftware[] = "PlantL33 " SW_VERSION;

// -- Hardware configuration,
constexpr uint8_t kWaterPin = 23;
constexpr uint8_t kModeLED = 17;

// Soil moisure sensor ADC pin assignments.
// - gpio32: adc1_ch4 -> mois1
// - gpio33: adc1_ch5 -> mois2
// - gpio34: adc1_ch6 -> mois3
// - gpio35: adc1_ch7 -> mois4
constexpr uint8_t kMoistureAnalogPin[4] = {32, 33, 34, 35};
#if BOARD_V13
// The pins on the v1.3 board. Changed pump-2 pin to not be on when power is first applied.
constexpr uint8_t kPumpCtlPin[4] = {18, 13, 16, 19};
#else
// The pins on the v1.2 board.
constexpr uint8_t kPumpCtlPin[4] = {18, 5, 16, 19};
#endif
constexpr unsigned kOledSwitchMsec = 5000;

#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
constexpr og3::App::LogType kLogType = og3::App::LogType::kUdp;
#else
constexpr og3::App::LogType kLogType = og3::App::LogType::kSerial;  // kSerial
#endif

// og3 application definition
og3::HAApp s_app(og3::HAApp::Options(
    kManufacturer, kModel,
    og3::WifiApp::Options()
        .withSoftwareName(kSoftware)
        .withDefaultDeviceName("plant133")
        .withOta(og3::OtaManager::Options(OTA_PASSWORD))
#if defined(AP_PASSWORD)
        .withWifi(og3::WifiManager::Options().withApPassword(AP_PASSWORD))
#endif
#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
        .withUdpLogHost(IPAddress(LOG_UDP_ADDRESS))
#endif
        .withApp(og3::App::Options().withLogType(kLogType).withReserveTasks(32))));

// Have oled display IP address or AP status.
og3::OledWifiInfo wifi_infof(&s_app.tasks());
// Have OLED screen rotate between different views over time.
og3::OledDisplayRing s_oled(&s_app.module_system(), kModel, kOledSwitchMsec, og3::Oled::kSixteenPt,
                            og3::Oled::Orientation::kDefault);

// Temperature/humidity sensing in the vicinity of the device.
og3::VariableGroup s_climate_vg("plant133");
og3::Shtc3 s_shtc3("temperature", "humidity", &s_app.module_system(), "temperature", s_climate_vg);

// A periodic task to monitor temperature/humidity and send the results via MQTT.
og3::PeriodicTaskScheduler climate_scheduler(
    10 * og3::kMsecInSec, og3::kMsecInMin,
    []() {
      s_shtc3.read();
      s_app.mqttSend(s_climate_vg);
    },
    &s_app.tasks());

og3::WifiWatchdog s_watchdog(&s_app, std::chrono::seconds(5), std::chrono::seconds(1));

// s_reservior monitors the water level of the reservoir: the float, and the number of seconds
//  the pumps have run since the float detected low water level.
og3::ReservoirCheck s_reservoir(kWaterPin, &s_app);

// s_plants are the 4 different plant watering sytems.
// The code for the plant watering system is in lib/watering/.
std::array<og3::Watering, 4> s_plants{{
    {0, "plant1", kMoistureAnalogPin[0], kModeLED, kPumpCtlPin[0], &s_app},
    {1, "plant2", kMoistureAnalogPin[1], kModeLED, kPumpCtlPin[1], &s_app},
    {2, "plant3", kMoistureAnalogPin[2], kModeLED, kPumpCtlPin[2], &s_app},
    {3, "plant4", kMoistureAnalogPin[3], kModeLED, kPumpCtlPin[3], &s_app},
}};

// Web interface buttons for the main device web page.
og3::WebButton s_button_wifi_config = s_app.createWifiConfigButton();
og3::WebButton s_button_mqtt_config = s_app.createMqttConfigButton();
og3::WebButton s_button_app_status = s_app.createAppStatusButton();
og3::WebButton s_button_restart = s_app.createRestartButton();

static String s_body;

// Web callback for main device web page.
og3::NetHandlerStatus handleWebRoot(og3::NetRequest* request, og3::NetResponse* response) {
  s_body.clear();
  s_shtc3.read();
  og3::html::writeTableInto(&s_body, s_climate_vg);
  // Write a table of watering state variables.
  og3::html::writeTableInto(&s_body, s_reservoir.variables());
  // Write state of Wifi
  og3::html::writeTableInto(&s_body, s_app.wifi_manager().variables());
  // Write state of MQTT
  og3::html::writeTableInto(&s_body, s_app.mqtt_manager().variables());
  // Add config for reservoir.
  s_reservoir.add_html_status_button(&s_body);
  // Add a button for watering status for each system
  for (const auto& plant : s_plants) {
    plant.add_html_status_button(&s_body);
  }
  // Add a button for configuring Wifi.
  s_button_wifi_config.add_button(&s_body);
  // Add a button for configuring MQTT.
  s_button_mqtt_config.add_button(&s_body);
  // Add a button for looking at app state.
  s_button_app_status.add_button(&s_body);

  s_body +=
      ("<p><button onclick=\"location.href='/static/test.html'\" type=\"button\">"
       "Test</button></p>\n");

  // Add a button for rebooting the device.
  s_button_restart.add_button(&s_body);
  // Send the page back to the web client.
  og3::sendWrappedHTML(request, response, s_app.board_cname(), kSoftware, s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

void draw_graphs() {
  constexpr int16_t kScreenWidth = 128;
  constexpr int16_t kScreenHeight = 32;
  constexpr int16_t kMargin = 3;
  constexpr int kYScreenTop = kMargin;
  constexpr int kYScreenBot = kScreenHeight - kMargin;

  s_oled.clear();
  auto& scr = s_oled.screen();
  for (size_t i = 0; i < s_plants.size(); i++) {
    const auto& plant = s_plants[i];
    if (!plant.isEnabled()) {
      continue;
    }

    struct PercentToY {
      float m_slope, m_pmin;
      PercentToY(float min, float max) {
        const float mid = (min + max) / 2.0f;
        const float diff = max - min;
        const float pmax = std::min(100.0f, mid + diff);
        m_pmin = std::max(0.0f, mid - diff);
        m_slope = static_cast<float>(kYScreenTop - kYScreenBot) / (pmax - m_pmin);
      }
      int16_t y(float percent) const {
        const int16_t out = static_cast<int16_t>((percent - m_pmin) * m_slope) + kYScreenBot;
        return (out < kYScreenTop) ? kYScreenTop : (out > kYScreenBot) ? kYScreenBot : out;
      }
    } p2y(plant.minTarget(), plant.maxTarget());

    auto line = [&scr, &p2y](int16_t x, float percent, int16_t offset) {
      const int16_t y1 = p2y.y(percent);
      const int16_t y2 = y1 + offset;
      scr.drawLine(x - 3, y2, x, y1);
      scr.drawLine(x, y1, x + 3, y2);
    };

    const int16_t x = kScreenWidth * (1 + 2 * i) / 8;
    scr.drawVerticalLine(x, kMargin, kScreenHeight - (2 * kMargin));
    line(x, plant.minTarget(), 0);
    line(x, plant.maxTarget(), 0);
    line(x, plant.moisturePercent(), 3 * plant.direction());
  }
  s_oled.screen().display();
}

og3::NetHandlerStatus statusJson(og3::NetRequest* request, og3::NetResponse* response) {
  s_body.clear();
  s_shtc3.read();
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  s_climate_vg.toJson(json, 0);
  s_reservoir.variables().toJson(json, 0);
  for (const auto& plant : s_plants) {
    plant.variables().toJson(json, 0);
  }
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus apiGetPlants(og3::NetRequest* request, og3::NetResponse* response) {
  JsonDocument jsondoc;
  JsonArray array = jsondoc.to<JsonArray>();
  int id = 0;
  for (const auto& plant : s_plants) {
    JsonObject json = array.add<JsonObject>();
    id += 1;
    json["id"] = id;
    plant.getApiPlants(json);
  }
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus apiGetMoisture(og3::NetRequest* request, og3::NetResponse* response) {
  JsonDocument jsondoc;
  JsonArray array = jsondoc.to<JsonArray>();
  int id = 0;
  for (const auto& plant : s_plants) {
    JsonObject json = array.add<JsonObject>();
    id += 1;
    json["id"] = id;
    json["moisture"] = plant.moisturePercent();
    json["rawMoisture"] = plant.rawMoisture();
    json["doseCount"] = plant.doseLog().doseCount();
    json["state"] = plant.stateName();
  }
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus apiGetStatus(og3::NetRequest* request, og3::NetResponse* response) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  json["temperature"] = s_shtc3.temperature();
  json["humidity"] = s_shtc3.humidity();
  json["waterLevel"] = s_reservoir.haveWater();
  json["pumpTimeRemaining"] = s_reservoir.secondsRemaining();
  json["mqttConnected"] = s_app.mqtt_manager().isConnected();
  json["software"] = SW_VERSION;
#if BOARD_V13
  json["hardware"] = "1.3";
#else
  json["hardware"] = "1.2";
#endif
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus putApiPlant(int id, og3::NetRequest* request, og3::NetResponse* response,
                                  JsonVariant& jsonIn) {
  if (id < 1 || id > static_cast<int>(s_plants.size())) {
    response->send(500, "text/plain", "bad plant id");
    NET_REPLY(request, ESP_FAIL);
  }
  if (!jsonIn.is<JsonObject>()) {
    response->send(500, "text/plain", "not a json object");
    NET_REPLY(request, ESP_FAIL);
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  if (!s_plants[id - 1].putApiPlants(obj)) {
    response->send(500, "text/plain", "failed to update plant");
    NET_REPLY(request, ESP_FAIL);
  }
  response->send(200, "text/plain", "ok");
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus pumpTest(og3::NetRequest* request, og3::NetResponse* response,
                               JsonVariant& jsonIn) {
  if (!jsonIn.is<JsonObject>()) {
    response->send(200, "application/json", "{\"isOk\":false,\"message\":\"Not an object\"}");
    NET_REPLY(request, ESP_FAIL);
  }
  const JsonObject jsonObj = jsonIn.as<JsonObject>();
  const int pump_id = jsonObj["pumpId"].as<int>();
  const int duration = jsonObj["duration"].as<int>();
  if (pump_id >= 1 && pump_id <= 4 && duration >= 0) {
    s_plants[pump_id - 1].relay().turnOn(duration);
    response->send(200, "application/json", "{\"isOk\":true}");
    NET_REPLY(request, ESP_OK);
  }
  response->send(200, "application/json", "{\"isOk\":false,\"message\":\"Bad values\"}");
  NET_REPLY(request, ESP_FAIL);
}

og3::NetHandlerStatus configJson(og3::NetRequest* request, og3::NetResponse* response) {
  s_body.clear();
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  s_reservoir.configVariables().toJson(json, og3::VariableBase::Flags::kConfig);
  for (const auto& plant : s_plants) {
    plant.configVariables().toJson(json, og3::VariableBase::Flags::kConfig);
  }
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus apiGetWifi(og3::NetRequest* request, og3::NetResponse* response) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  const auto& wifi = s_app.wifi_manager();
  json["board"] = wifi.board();
  json["password"] = wifi.password();
  json["essid"] = wifi.essid();
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus putWifiConfig(og3::NetRequest* request, og3::NetResponse* response,
                                    JsonVariant& jsonIn) {
  if (!jsonIn.is<JsonObject>()) {
    response->send(500, "text/plain", "not a json object");
    NET_REPLY(request, ESP_FAIL);
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  s_app.wifi_manager().variables().updateFromJson(obj);
  s_app.config().write_config(s_app.wifi_manager().variables());
  response->send(200, "text/plain", "ok");
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus apiGetMqtt(og3::NetRequest* request, og3::NetResponse* response) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  const auto& mqtt = s_app.mqtt_manager();
  json["enabled"] = mqtt.isEnabled();
  json["host"] = mqtt.host();
  json["password"] = mqtt.auth_password();
  json["user"] = mqtt.auth_user();
  serializeJson(jsondoc, s_body);
  response->send(200, "application/json", s_body.c_str());
  NET_REPLY(request, ESP_OK);
}

og3::NetHandlerStatus putMqttConfig(og3::NetRequest* request, og3::NetResponse* response,
                                    JsonVariant& jsonIn) {
  if (!jsonIn.is<JsonObject>()) {
    response->send(500, "text/plain", "not a json object");
    NET_REPLY(request, ESP_FAIL);
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  s_app.mqtt_manager().variables().updateFromJson(obj);
  s_app.config().write_config(s_app.mqtt_manager().variables());
  if (s_app.mqtt_manager().isEnabled() && !s_app.mqtt_manager().isConnected()) {
    s_app.mqtt_manager().connect();
  } else if (!s_app.mqtt_manager().isEnabled() && s_app.mqtt_manager().isConnected()) {
    s_app.mqtt_manager().disconnect();
  }
  response->send(200, "text/plain", "ok");
  NET_REPLY(request, ESP_OK);
}

}  // namespace

void setup() {
  s_oled.addDisplayFn(draw_graphs);
  s_app.web_server_module().native_server().serveStatic("/config/", LittleFS, "/");
  s_app.web_server_module().on("/test/status", statusJson);
  s_app.web_server_module().on("/test/config", configJson);
  s_app.web_server_module().on("/root", handleWebRoot);
  initSvelteStaticFiles(&s_app.web_server_module());
  s_app.web_server_module().on("/api/plants", HTTP_GET, apiGetPlants);
  s_app.web_server_module().on("/api/wifi", HTTP_GET, apiGetWifi);
  s_app.web_server_module().on("/api/mqtt", HTTP_GET, apiGetMqtt);
  s_app.web_server_module().on("/api/moisture", HTTP_GET, apiGetMoisture);
  s_app.web_server_module().on("/api/status", HTTP_GET, apiGetStatus);
  s_app.web_server_module().onJson("/test/pump", HTTP_POST, pumpTest);

  for (int id = 1; id <= static_cast<int>(s_plants.size()); id++) {
    char path[80];
    snprintf(path, sizeof(path), "/api/plants/%d", id);
    s_app.web_server_module().onJson(
        path, HTTP_PUT,
        [id](og3::NetRequest* request, og3::NetResponse* response, JsonVariant& json) {
          return putApiPlant(id, request, response, json);
        });
  }

  s_app.web_server_module().onJson("/api/wifi", HTTP_PUT, putWifiConfig);
  s_app.web_server_module().onJson("/api/mqtt", HTTP_PUT, putMqttConfig);

  s_app.web_server_module().on("/api/restart", HTTP_POST,
                               [](og3::NetRequest* request, og3::NetResponse* response) {
                                 response->send(200, "text/plain", "restarting");
                                 s_app.tasks().runIn(1000, []() { ESP.restart(); });
                                 NET_REPLY(request, ESP_OK);
                               });

  s_app.setup();
}

void loop() { s_app.loop(); }
