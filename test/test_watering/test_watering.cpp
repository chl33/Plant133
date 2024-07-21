// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <ArduinoFake.h>
#include <cl3/logger.h>
#include <cl3/module.h>
#include <cl3/module_system.h>
#include <unity.h>
#include <watering.h>

#include <cstdio>
#include <string>
#include <vector>

namespace cl3 {

namespace {

class TestLogger : public Logger {
 public:
  virtual void log(const char* msg) final {
    m_output += msg;
    m_output += "\n";
  }
  bool check(const std::string& expected) {
    const bool ret = m_output == expected;
    if (!ret) {
      printf(
          "Expected '%s'\n"
          "but got  '%s'\n",
          expected.c_str(), m_output.c_str());
    }
    m_output.clear();
    return ret;
  }

 private:
  std::string m_output;
};

}  // namespace

}  // namespace cl3

void setUp() {}

void tearDown() {}

void test1() {
  cl3::TestLogger log;
  cl3::ModuleSystem depends(&log);
  og3::Tasks tasks(16, &depends);
  cl3::VariableGroup vg("test1");
  constexpr uint8_t kModeLED = 5;
  constexpr uint8_t kWaterPin = 6;
  constexpr uint8_t kPumpCtlPin = 7;

  using namespace fakeit;
  When(Method(ArduinoFake(), digitalWrite)).Return();
  When(Method(ArduinoFake(), millis)).Return(0, 10001, 20002, 30003, 40004);

  cl3::Watering watering(A0, kWaterPin, kModeLED, kPumpCtlPin, &tasks, &vg);

  Verify(Method(ArduinoFake(), millis)).Once();
  // Verify(Method(ArduinoFake(), digitalWrite)).Once();
  Verify(Method(ArduinoFake(), digitalWrite).Using(kPumpCtlPin, LOW)).Once();

  TEST_ASSERT_EQUAL_INT(cl3::Watering::kStateEval, watering.state());

  When(Method(ArduinoFake(), digitalRead)).Return(1);
  When(Method(ArduinoFake(), analogRead)).Return(730);       // low moisturef
  When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();  // pump off

  watering.loop();

  Verify(Method(ArduinoFake(), digitalRead)).Once();
  Verify(Method(ArduinoFake(), analogRead)).Once();
  // Verify(Method(ArduinoFake(), digitalWrite).Using(kPumpCtlPin, LOW)).Once();

  // TEST_ASSERT_EQUAL_INT(cl3::Watering::kStateWaitForNextCycle, watering.state());
}

int runUnityTests() {
  UNITY_BEGIN();
  RUN_TEST(test1);
  return UNITY_END();
}

// For native platform.
int main() { return runUnityTests(); }

// For arduion framework
void setup() {}
void loop() {}

// For ESP-IDF framework
void app_main() { runUnityTests(); }
