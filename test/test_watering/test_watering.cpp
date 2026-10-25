// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <ArduinoFake.h>
#include <og3/ha_app.h>
#include <og3/logger.h>
#include <og3/module_system.h>
#include <og3/oled_display_ring.h>
#include <unity.h>
#include <watering.h>

#include <cstdio>

#include "reservoir_check.h"
#include "watering_constants.h"

using namespace fakeit;

void setUp() {
  ArduinoFakeReset();
  std::remove("plant1.json");
}

void tearDown() { std::remove("plant1.json"); }

void test_watering_cycle() {
  constexpr uint8_t kMoisturePin = 32;
  constexpr uint8_t kModeLED = 17;
  constexpr uint8_t kWaterPin = 23;
  constexpr uint8_t kPumpCtlPin = 18;

  unsigned long now_ms = 0;
  When(Method(ArduinoFake(), millis)).AlwaysDo([&now_ms]() { return now_ms; });
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalRead)).AlwaysReturn(HIGH);  // reservoir has water
  // 2500 counts is ~28% moisture (between 2900=0% and 1470=100%), below 70% minTarget
  When(Method(ArduinoFake(), analogRead)).AlwaysReturn(2500);

  og3::HAApp app(
      og3::HAApp::Options("Chris Lee", "Plantl337",
                          og3::WifiApp::Options()
                              .withSoftwareName("PlantL33 test")
                              .withDefaultDeviceName("plant133")
                              .withApp(og3::App::Options().withLogType(og3::App::LogType::kNone))));
  og3::OledDisplayRing oled(&app.module_system(), "Plantl337", 5000);
  og3::ReservoirCheck reservoir(kWaterPin, &app);
  og3::Watering watering(0, "plant1", kMoisturePin, kModeLED, kPumpCtlPin, &app);

  app.setup();

  // Initially watering is disabled by default config, so first loop transitions to kStateDisabled.
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateDisabled, watering.state());
  TEST_ASSERT_FALSE(watering.isEnabled());

  // Enable watering; state transitions to kStateWaitForNextCycle.
  watering.setPumpEnable(true);
  TEST_ASSERT_TRUE(watering.isEnabled());
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateWaitForNextCycle, watering.state());

  // Advance past initial wait and between-doses interval (15 minutes).
  now_ms = (og3::kPumpOffSec + 10) * og3::kMsecInSec;
  watering.loop();
  // Reading is ~28% (< 70% minTarget), so it transitions to kStateEval.
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEval, watering.state());

  // Next update (1ms later): evaluates moisture & reservoir, transitions to kStateDose.
  now_ms += 2000;
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateDose, watering.state());

  // Next update: turns pump ON, increments dose count, transitions to kStateEndOfDose.
  now_ms += 2000;
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEndOfDose, watering.state());
  TEST_ASSERT_TRUE(watering.relay().isOn());
  TEST_ASSERT_EQUAL_UINT(1, watering.doseLog().doseCount());

  // After pump dose duration (3000ms), turns pump OFF and returns to kStateEval.
  now_ms += 3000;
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEval, watering.state());
  TEST_ASSERT_FALSE(watering.relay().isOn());

  // Simulate wet soil (1600 counts -> ~91% moisture, above 80% maxTarget).
  When(Method(ArduinoFake(), analogRead)).AlwaysReturn(1600);
  for (int i = 0; i < 20; ++i) {
    now_ms += 60 * og3::kMsecInSec;
    watering.loop();
  }
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateWaitForNextCycle, watering.state());
}

void test_dose_limit_pauses_watering() {
  constexpr uint8_t kMoisturePin = 32;
  constexpr uint8_t kModeLED = 17;
  constexpr uint8_t kWaterPin = 23;
  constexpr uint8_t kPumpCtlPin = 18;

  unsigned long now_ms = 0;
  When(Method(ArduinoFake(), millis)).AlwaysDo([&now_ms]() { return now_ms; });
  When(Method(ArduinoFake(), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();
  When(Method(ArduinoFake(), digitalRead)).AlwaysReturn(HIGH);
  When(Method(ArduinoFake(), analogRead)).AlwaysReturn(2500);  // ~28% moisture (dry)

  og3::HAApp app(
      og3::HAApp::Options("Chris Lee", "Plantl337",
                          og3::WifiApp::Options()
                              .withSoftwareName("PlantL33 test")
                              .withDefaultDeviceName("plant133")
                              .withApp(og3::App::Options().withLogType(og3::App::LogType::kNone))));
  og3::OledDisplayRing oled(&app.module_system(), "Plantl337", 5000);
  og3::ReservoirCheck reservoir(kWaterPin, &app);
  og3::Watering watering(0, "plant1", kMoisturePin, kModeLED, kPumpCtlPin, &app);

  app.setup();

  // Configure maxDosesPerCycle = 2 via JSON API.
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  watering.getApiPlants(obj);
  obj["maxDosesPerCycle"] = 2;
  obj["enabled"] = true;
  TEST_ASSERT_TRUE(watering.putApiPlants(obj));

  // Transition from kStateWaitForNextCycle -> kStateEval.
  now_ms = (og3::kPumpOffSec + 10) * og3::kMsecInSec;
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEval, watering.state());

  // Run 2 full doses.
  for (int dose = 1; dose <= 2; ++dose) {
    now_ms += (og3::kPumpOffSec + 5) * og3::kMsecInSec;
    watering.loop();  // kStateEval -> kStateDose
    TEST_ASSERT_EQUAL_INT(og3::Watering::kStateDose, watering.state());

    now_ms += 2000;
    watering.loop();  // kStateDose -> kStateEndOfDose
    TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEndOfDose, watering.state());

    now_ms += 3000;
    watering.loop();  // kStateEndOfDose -> kStateEval
    TEST_ASSERT_EQUAL_INT(og3::Watering::kStateEval, watering.state());
    TEST_ASSERT_EQUAL_UINT(dose, watering.doseLog().doseCount());
  }

  // Next evaluation should pause watering since 2 doses reached maxDosesPerCycle (2).
  now_ms += (og3::kPumpOffSec + 5) * og3::kMsecInSec;
  watering.loop();
  TEST_ASSERT_EQUAL_INT(og3::Watering::kStateWateringPaused, watering.state());
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test_watering_cycle);
  RUN_TEST(test_dose_limit_pauses_watering);
  return UNITY_END();
}

// For native platform.
int main() { return runUnityTests(); }

// For arduino framework
void setup() {}
void loop() {}

// For ESP-IDF framework
void app_main() { runUnityTests(); }
