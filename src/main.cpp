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

#include <algorithm>
#include <array>

#include "watering.h"

#define SW_VERSION "0.7.4"

namespace {

const char kManufacturer[] = "Chris Lee";
const char kModel[] = "Plantl337a";
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
constexpr uint8_t kPumpCtlPin[4] = {18, 5, 16, 19};
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
  for (const auto& plant : s_plants) {
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
  s_app.web_server().on("/", handleWebRoot);
  // Run the og3 application setup code.
  s_app.setup();
}

// This is called repeaedly when code is running.
void loop() { s_app.loop(); }
