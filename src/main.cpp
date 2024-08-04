// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <Arduino.h>
#include <LittleFS.h>
#include <og3/constants.h>
#include <og3/ha_app.h>
#include <og3/html_table.h>
#include <og3/oled_wifi_info.h>
#include <og3/shtc3.h>
#include <og3/units.h>
#include <og3/variable.h>

#include <array>

#include "watering.h"

#define SW_VERSION "0.7.0"

namespace {

const char kManufacturer[] = "Chris Lee";
const char kModel[] = "Plantl337a";
const char kSoftware[] = "PlantL33 " SW_VERSION;

// -- Hardware configuration,
constexpr uint8_t kWaterPin = 23;
constexpr uint8_t kModeLED = 17;

// gpio32: adc1_ch4 -> mois1
// gpio33: adc1_ch5 -> mois2
// gpio34: adc1_ch6 -> mois3
// gpio35: adc1_ch7 -> mois4
constexpr uint8_t kMoistureAnalogPin[4] = {32, 33, 34, 35};
constexpr uint8_t kPumpCtlPin[4] = {18, 5, 16, 19};
constexpr unsigned kOledSwitchMsec = 5000;

// og3 application definition
og3::HAApp s_app(
    og3::HAApp::Options(kManufacturer, kModel,
                        og3::WifiApp::Options()
                            .withSoftwareName(kSoftware)
                            .withDefaultDeviceName("plantl33")
                            .withOta(og3::OtaManager::Options(OTA_PASSWORD))
#if defined(LOG_UDP) && defined(LOG_UDP_ADDRESS)
                            .withApp(og3::App::Options().withLogType(og3::App::LogType::kUdp))
                            .withUdpLogHost(IPAddress(IPAddress(LOG_UDP_ADDRESS)))
#else
                            // kNone
                            .withApp(og3::App::Options().withLogType(og3::App::LogType::kSerial))
#endif
                            ));

// Have oled display IP address or AP status.
og3::OledWifiInfo wifi_infof(&s_app.tasks());

og3::VariableGroup s_climate_vg("plant133");
og3::Shtc3 s_shtc3("temperature", "humidity", &s_app.module_system(), "temperature", &s_climate_vg);

og3::ReservoirCheck s_reservoir(kWaterPin, &s_app);

// A periodic task to monitor temperature/humidity in the area of the device.
og3::PeriodicTaskScheduler climate_scheduler(
    10 * og3::kMsecInSec, og3::kMsecInMin,
    []() {
      s_shtc3.read();
      s_app.mqttSend(s_climate_vg);
    },
    &s_app.tasks());

og3::OledDisplayRing s_oled(&s_app.module_system(), kModel, kOledSwitchMsec, og3::Oled::kSixteenPt,
                            og3::Oled::Orientation::kDefault);

// Add the plant-watering system.
// This is defined in the app's lib/ subdirectory.
std::array<og3::Watering, 4> plants{{
    {0, "plant1", kMoistureAnalogPin[0], kModeLED, kPumpCtlPin[0], &s_app},
    {1, "plant2", kMoistureAnalogPin[1], kModeLED, kPumpCtlPin[1], &s_app},
    {2, "plant3", kMoistureAnalogPin[2], kModeLED, kPumpCtlPin[2], &s_app},
    {3, "plant4", kMoistureAnalogPin[3], kModeLED, kPumpCtlPin[3], &s_app},
}};

// Web interface buttons.
og3::WebButton s_button_wifi_config = s_app.createWifiConfigButton();
og3::WebButton s_button_mqtt_config = s_app.createMqttConfigButton();
og3::WebButton s_button_app_status = s_app.createAppStatusButton();
og3::WebButton s_button_restart = s_app.createRestartButton();

// Web callback for main device web page.
void handleWebRoot(AsyncWebServerRequest* request) {
  // The send of the web page happens asynchronously after this function exits, so we need to make
  // sure the storage for the page remains.  I don't know of a great way to handle the case where
  // multiple clients are being served data at once.
  static String s_body;
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
  for (const auto& plant : plants) {
    plant.add_html_status_button(&s_body);
  }
  // Add a button for configuring Wifi.
  s_button_wifi_config.add_button(&s_body);
  // Add a button for configuring MQTT.
  s_button_mqtt_config.add_button(&s_body);
  // Add a button for looking at app state.
  s_button_app_status.add_button(&s_body);
  // Add a button for rebooting the device.
  s_button_restart.add_button(&s_body);
  // Send the page back to the web client.
  og3::sendWrappedHTML(request, s_app.board_cname(), kSoftware, s_body.c_str());
}

constexpr int16_t kScreenWidth = 128;
constexpr int16_t kScreenHeight = 32;
constexpr int16_t kMargin = 3;

int16_t percent_to_y(float percent) {
  const int16_t y = (kScreenHeight - 2 * kMargin) * percent / 100 + kMargin;
  constexpr int16_t kMax = kScreenHeight - kMargin;
  constexpr int16_t kMin = kMargin;
  if (y < kMin) {
    return kMax;
  } else if (y >= kMax) {
    return kMin;
  }
  return kScreenHeight - 1 - y;
}

void draw_graphs() {
  s_oled.clear();
  auto& scr = s_oled.screen();
  auto line = [&scr](int16_t x, float percent, int16_t offset) {
    const int16_t y1 = percent_to_y(percent);
    const int16_t y2 = y1 + offset;
    // s_app.log().logf("x:%d y1:%d y2:%d", (int)x, (int)y1, (int)y2);
    scr.drawLine(x - 3, y2, x, y1);
    scr.drawLine(x, y1, x + 3, y2);
  };
  for (size_t i = 0; i < plants.size(); i++) {
    if (!plants[i].isEnabled()) {
      continue;
    }
    const int16_t x = kScreenWidth * (1 + 2 * i) / 8;
    scr.drawVerticalLine(x, kMargin, kScreenHeight - (2 * kMargin));
    line(x, plants[i].minTarget(), 0);
    line(x, plants[i].maxTarget(), 0);
    line(x, plants[i].moisturePercent(), 3 * plants[i].direction());
  }
  s_oled.screen().display();
}

}  // namespace

// This function is called once when code is started.
void setup() {
  s_oled.addDisplayFn(draw_graphs);
  // Setup URL handlers in the web server.
  s_app.web_server().serveStatic("/config/", LittleFS, "/");
  s_app.web_server().on("/", handleWebRoot);
  s_app.setup();
}

// This is called repeaedly when code is running.
void loop() { s_app.loop(); }
