// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <Arduino.h>
#include <AsyncJson.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <og3/constants.h>
#include <og3/ha_app.h>
#include <og3/html_table.h>
#include <og3/oled_wifi_info.h>
#include <og3/shtc3.h>
#include <og3/units.h>
#include <og3/variable.h>

#include <algorithm>
#include <array>

#include "ArduinoJson/Deserialization/DeserializationError.hpp"
#include "ArduinoJson/Deserialization/deserialize.hpp"
#include "ArduinoJson/Document/JsonDocument.hpp"
#include "svelteesp32async.h"
#include "watering.h"

#define SW_VERSION "0.9.1"

// TODO:
//  Fixes to make to the svelte inteface:
//  - Can we shrink or hide the cards for disabled plants in the overview?
//  - Add a pump-test button for each plant on configuration page.
//  - Restart button. Maybe put in an app-status page.

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
void handleWebRoot(AsyncWebServerRequest* request) {
  // The send of the web page happens asynchronously after this function exits, so we need to make
  // sure the storage for the page remains.  I don't know of a great way to handle the case where
  // multiple clients are being served data at once.
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
  og3::sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_body.c_str());
}

// This code draws a graphical display of the watering states of plants that are enabled.
// A display like this is drawn for each enabled plant, but vertically: -|-->-|-
// The location of the '>' shows where the moisture level is comparted to the target min and
//  max value.  The arrow head '>' (drawn as 'v' in the real vertical display) shows that the
//  plant is not currently being watered.
// The arrow head points upward when the plant is being watered.
constexpr int16_t kScreenWidth = 128;
constexpr int16_t kScreenHeight = 32;
constexpr int16_t kMargin = 3;

constexpr int kYScreenTop = kMargin;
constexpr int kYScreenBot = kScreenHeight - kMargin;

class PercentToY {
 public:
  PercentToY(float min, float max) : m_min(min), m_max(max) {
    const float mid = (m_min + m_max) / 2.0f;
    const float diff = m_max - m_min;
    const float pmax = std::min(100.0f, mid + diff);
    m_pmin = std::max(0.0f, mid - diff);
    m_slope = (kYScreenTop - kYScreenBot) / (pmax - m_pmin);
  }

  int16_t y(float percent) const {
    const int16_t out = static_cast<int16_t>((percent - m_pmin) * m_slope) + kYScreenBot;
    return (out < kYScreenTop) ? kYScreenTop : (out > kYScreenBot) ? kYScreenBot : out;
  }

 private:
  const float m_min;
  const float m_max;
  float m_slope = 0.0f;
  float m_pmin = 0.0f;
};

void draw_graphs() {
  s_oled.clear();
  auto& scr = s_oled.screen();
  for (size_t i = 0; i < s_plants.size(); i++) {
    const auto& plant = s_plants[i];
    if (!plant.isEnabled()) {
      continue;
    }

    PercentToY p2y(plant.minTarget(), plant.maxTarget());
    auto line = [&scr, &p2y](int16_t x, float percent, int16_t offset) {
      const int16_t y1 = p2y.y(percent);
      const int16_t y2 = y1 + offset;
      // s_app.log().logf("x:%d y1:%d y2:%d", (int)x, (int)y1, (int)y2);
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

// Return current system status as JSON for AJAX status calls.
void statusJson(AsyncWebServerRequest* request) {
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
  request->send(200, "application/json", s_body);
}

void apiGetPlants(AsyncWebServerRequest* request) {
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
  request->send(200, "application/json", s_body);
}

void apiGetMoisture(AsyncWebServerRequest* request) {
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
  request->send(200, "application/json", s_body);
}

void apiGetStatus(AsyncWebServerRequest* request) {
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
  request->send(200, "application/json", s_body);
}

// Return current system status as JSON for AJAX status calls.
void putApiPlant(int id, AsyncWebServerRequest* request, JsonVariant& jsonIn) {
  if (id < 1 || id > static_cast<int>(s_plants.size())) {
    request->send(500, "text/plain", "bad plant id");
    return;
  }
  if (!jsonIn.is<JsonObject>()) {
    request->send(500, "text/plain", "not a json object");
    return;
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  if (!s_plants[id - 1].putApiPlants(obj)) {
    request->send(500, "text/plain", "failed to update plant");
    return;
  }
  request->send(200, "text/plain", "ok");
}

// Handle ajax POSTS with pump-test commands like: "{pumpId: 1, duration: 1000}"
void pumpTest(AsyncWebServerRequest* request, JsonVariant jsonIn) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();
  auto failed = [&json](const char* text) {
    json["message"] = text;
    return false;
  };

  if (!jsonIn.is<JsonObject>()) {
    failed("Not an object");
    serializeJson(jsondoc, s_body);
    request->send(200, "application/json", s_body);
    return;
  }
  const JsonObject jsonObj = jsonIn.as<JsonObject>();

  auto run = [&jsonObj, &json]() -> bool {
    auto get = [&jsonObj, &json](const char* id, int min, int max) -> int {
      const JsonVariant val = jsonObj[id];
      if (!val.is<int>()) {
        json["message"] = (String("No ") + id);
        return min - 1;
      }
      const int ival = val.as<int>();
      if (ival < min || ival > max) {
        json["message"] = (String("Bad value for ") + id);
        return min - 1;
      }
      return ival;
    };
    const int pump_id = get("pumpId", 1, 4);
    const int duration = get("duration", 0, 10000);
    if (pump_id < 1 || duration < 0) {
      return false;
    }
    auto& plant = s_plants[pump_id - 1];
    plant.relay().turnOn(duration);
    return true;
  };

  s_body.clear();
  json["isOk"] = run();
  serializeJson(json, s_body);
  request->send(200, "application/json", s_body);
}

// Return current system status as JSON for AJAX status calls.
void configJson(AsyncWebServerRequest* request) {
  s_body.clear();
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();

  s_reservoir.configVariables().toJson(json, og3::VariableBase::Flags::kConfig);
  for (const auto& plant : s_plants) {
    plant.configVariables().toJson(json, og3::VariableBase::Flags::kConfig);
  }
  serializeJson(jsondoc, s_body);
  request->send(200, "application/json", s_body);
}

void apiGetWifi(AsyncWebServerRequest* request) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();

  const auto& wifi = s_app.wifi_manager();

  json["board"] = wifi.board();
  json["password"] = wifi.password();
  json["essid"] = wifi.essid();
  serializeJson(jsondoc, s_body);
  request->send(200, "application/json", s_body);
}

// Return current system status as JSON for AJAX status calls.
void putWifiConfig(AsyncWebServerRequest* request, JsonVariant& jsonIn) {
  if (!jsonIn.is<JsonObject>()) {
    request->send(500, "text/plain", "not a json object");
    return;
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  if (s_app.wifi_manager().variables().updateFromJson(obj) == 0) {
    request->send(500, "text/plain", "no values updated");
  }
  s_app.config().write_config(s_app.wifi_manager().variables());
  request->send(200, "text/plain", "ok");
}

void apiGetMqtt(AsyncWebServerRequest* request) {
  JsonDocument jsondoc;
  JsonObject json = jsondoc.to<JsonObject>();

  const auto& mqtt = s_app.mqtt_manager();

  json["host"] = mqtt.host();
  json["password"] = mqtt.auth_password();
  json["user"] = mqtt.auth_user();
  serializeJson(jsondoc, s_body);
  request->send(200, "application/json", s_body);
}

// Return current system status as JSON for AJAX status calls.
void putMqttConfig(AsyncWebServerRequest* request, JsonVariant& jsonIn) {
  if (!jsonIn.is<JsonObject>()) {
    request->send(500, "text/plain", "not a json object");
    return;
  }
  JsonObject obj = jsonIn.as<JsonObject>();
  if (s_app.mqtt_manager().variables().updateFromJson(obj) == 0) {
    request->send(500, "text/plain", "no values updated");
  }
  s_app.config().write_config(s_app.mqtt_manager().variables());
  request->send(200, "text/plain", "ok");
}

}  // namespace

// This function is called once when code is started.
void setup() {
  // Register the graphical watering state display as one of the views the OLED display
  //  will rotate through.
  s_oled.addDisplayFn(draw_graphs);
  // Setup URL handlers in the web server.
  // Serve static files from the /config subdirectory in flash.
  s_app.web_server().serveStatic("/config/", LittleFS, "/");
  // Serve the root URL via the handleWebRoot() callback function.
  s_app.web_server().on("/test/status", statusJson);
  s_app.web_server().on("/test/config", configJson);
  s_app.web_server().on("/root", handleWebRoot);
  initSvelteStaticFiles(&s_app.web_server());
  s_app.web_server().on("/api/plants", HTTP_GET, apiGetPlants);
  s_app.web_server().on("/api/wifi", HTTP_GET, apiGetWifi);
  s_app.web_server().on("/api/mqtt", HTTP_GET, apiGetMqtt);
  s_app.web_server().on("/api/moisture", HTTP_GET, apiGetMoisture);
  s_app.web_server().on("/api/status", HTTP_GET, apiGetStatus);

  {  // Add pump test json callback.
    AsyncCallbackJsonWebHandler* pumpTestHandler = new AsyncCallbackJsonWebHandler("/test/pump");
    pumpTestHandler->setMethod(HTTP_POST);
    pumpTestHandler->onRequest(
        [](AsyncWebServerRequest* request, JsonVariant json) { pumpTest(request, json); });
    s_app.web_server().addHandler(pumpTestHandler);
  }

  for (int id = 1; id <= static_cast<int>(s_plants.size()); id++) {
    char path[80];
    snprintf(path, sizeof(path), "/api/plants/%d", id);
    AsyncCallbackJsonWebHandler* api_handler = new AsyncCallbackJsonWebHandler(path);
    api_handler->setMethod(HTTP_PUT);
    api_handler->onRequest([id](AsyncWebServerRequest* request, JsonVariant& json) {
      putApiPlant(id, request, json);
    });
    s_app.web_server().addHandler(api_handler);
  }

  {  // Add WiFi callback
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/wifi");
    handler->setMethod(HTTP_PUT);
    handler->onRequest(
        [](AsyncWebServerRequest* request, JsonVariant json) { putWifiConfig(request, json); });
    s_app.web_server().addHandler(handler);
  }
  {  // Add Mqtt callback
    AsyncCallbackJsonWebHandler* handler = new AsyncCallbackJsonWebHandler("/api/mqtt");
    handler->setMethod(HTTP_PUT);
    handler->onRequest(
        [](AsyncWebServerRequest* request, JsonVariant json) { putMqttConfig(request, json); });
    s_app.web_server().addHandler(handler);
  }

  s_app.web_server().on("/api/restart", HTTP_POST, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "restarting");
    s_app.tasks().runIn(1000, []() { ESP.restart(); });
  });

  // Run the og3 application setup code.
  s_app.setup();
}

// This is called repeaedly when code is running.
void loop() { s_app.loop(); }
