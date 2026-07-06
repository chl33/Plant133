// Copyright (c) 2026 Chris Lee and contributors.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#pragma once

#include <og3/blink_led.h>
#include <og3/ha_app.h>
#include <og3/module.h>
#include <og3/relay.h>
#include <og3/variable.h>

#include "dose_log.h"
#include "moisture_sensor.h"
#include "reservoir_check.h"

namespace og3 {

class ConfigInterface;
class MqttManager;
class HADiscovery;

/** @brief Watering manages the state machine for watering a plant. */
class Watering : public Module {
 public:
  /** @brief The set of possible states in the state machine. */
  enum State {
    /** @brief Check moisture level, reservior level during watering cycle. */
    kStateEval = 0,

    /** @brief Start the pump and run for a fixed amount of time. */
    kStateDose,

    /** @brief Turn pump off at end of dose. */
    kStateEndOfDose,

    /** @brief After moisture level reaches maximum level, wait for it to reach minimum
     *  moisture level and also wait for minimum time between watering cycles, then
     *  go back to kStateEval.
     */
    kStateWaitForNextCycle,

    /** @brief After too many does in a cycle, pause for 12 hours. */
    kStateWateringPaused,

    /** @brief State for when pump is disabled. */
    kStateDisabled,

    /** @brief State for when testing a single pump cycle. */
    kStatePumpTest,

    /** @brief Test state. */
    kStateTest,
  };

  /** @brief Variable type for tracking the current watering state. */
  class StateVariable : public EnumStrVariable<State> {
   public:
    StateVariable(const char* name_, State value, const char* description, unsigned flags,
                  VariableGroup& vg)
        : EnumStrVariable<State>(name_, value, description, State::kStateTest, s_state_names, flags,
                                 vg) {}

    String string() const final { return s_state_names[static_cast<int>(m_value)]; }

    bool fromString(const String& value) final {
      for (int i = 0; i <= kStateTest; i++) {
        if (0 == strcmp(value.c_str(), s_state_names[i])) {
          m_value = static_cast<State>(i);
          return true;
        }
      }
      return false;
    }

    StateVariable& operator=(State val) {
      m_value = static_cast<int>(val);
      setFailed(false);
      return *this;
    }
  };

  static const char* s_state_names[];

  /**
   * @brief Constructs a Watering module.
   * @param index The index of this plant (0-3).
   * @param name Unique name identifying this plant.
   * @param moisture_pin Hardware ADC pin for moisture sensing.
   * @param mode_led Pin for the status LED.
   * @param pump_ctl_pin Pin for the pump relay control.
   * @param app Pointer to the main application.
   */
  Watering(unsigned index, const char* name, uint8_t moisture_pin, uint8_t mode_led,
           uint8_t pump_ctl_pin, HAApp* app);

  /** @brief Main loop update function for the state machine. */
  void loop();

  /** @return The current state of the machine. */
  State state() const { return static_cast<State>(m_state.value()); }
  /** @return The human-readable state name. */
  const char* stateName() const { return s_state_names[static_cast<int>(m_state.value())]; }
  /** @return The user-assigned name for this plant. */
  const String& plantName() const { return m_plant_name.value(); }
  /** @return Current filtered moisture percentage. */
  float moisturePercent() const { return m_moisture.filteredValue(); }
  /** @return Current raw ADC counts from the sensor. */
  unsigned rawMoisture() const { return m_moisture.rawCounts(); }
  /** @return Minimum moisture threshold. */
  float minTarget() const { return m_min_moisture_target.value(); }
  /** @return Maximum moisture threshold. */
  float maxTarget() const { return m_max_moisture_target.value(); }
  /** @return true if automatic watering is enabled. */
  bool isEnabled() const { return m_watering_enabled.value(); }
  /** @return true if the associated reservoir is detected as empty. */
  bool isReservoirEmpty() const { return m_reservoir_check && !m_reservoir_check->haveWater(); }
  /** @return true if reservoir checks are enabled for this plant. */
  bool reservoirCheckEnabled() const { return m_reservoir_check_enabled.value(); }

  /** @brief Manually enable or disable watering. */
  void setPumpEnable(bool enable);
  /** @brief Triggers a manual pump test dose. */
  void testPump() { setState(kStatePumpTest, 100, "start pump test"); }
  /** @brief Triggers a full hardware test. */
  void test() { setState(kStateTest, 100, "full test"); }

  const char* statusUrl() const { return m_status_url.c_str(); }
  const char* configUrl() const { return m_config_url.c_str(); }
  const char* pumpTestUrl() const { return m_pump_test_url.c_str(); }

  void add_html_status_button(String* body) const {
    add_html_button(body, plantName().c_str(), statusUrl());
  }

  /** @brief Populates a JSON object with plant status for the API. */
  void getApiPlants(JsonObject json) const;
  /** @brief Updates plant configuration from a JSON object. */
  bool putApiPlants(JsonObject json);

  /** @return +1 during watering, -1 waiting for next cycle, 0 if disabled. */
  int direction() const;

  /** @return Reference to the status variable group. */
  const VariableGroup& variables() const { return m_vg; }
  /** @return Reference to the configuration variable group. */
  const VariableGroup& configVariables() const { return m_cfg_vg; }
  /** @return Reference to the pump relay. */
  Relay& relay() { return m_pump; }
  /** @return Reference to the dose log. */
  const DoseLog& doseLog() const { return m_dose_log; }

 protected:
  /**
   * @brief This method performs the work of the state machine.
   *
   * Before the method exits, it updates the current state and then schedules
   * this to get called again after a given time interval.
   * @param state The next state.
   * @param msec Time delay before next evaluation.
   * @param msg Descriptive message for logging the transition.
   */
  void setState(State state, unsigned msec, const char* msg);

 private:
  void _fullTest();
  NetHandlerStatus handleStatusRequest(NetRequest* request, NetResponse* response);
  NetHandlerStatus handleConfigRequest(NetRequest* request, NetResponse* response);

  std::string varname(const char* elname);

  HAApp* m_app;
  const unsigned m_index;
  VariableGroup m_cfg_vg;
  VariableGroup m_vg;
  const String m_status_url;
  const String m_config_url;
  const String m_pump_test_url;
  String m_html;

  MoistureSensor m_moisture;
  Relay m_pump;
  BlinkLed m_mode_led;
  DoseLog m_dose_log;

  Variable<String> m_plant_name;
  FloatVariable m_max_moisture_target;
  FloatVariable m_min_moisture_target;
  FloatVariable m_pump_dose_msec;
  FloatVariable m_between_doses_sec;
  StateVariable m_state;
  FloatVariable m_sec_since_dose;
  BoolVariable m_watering_enabled;
  BoolVariable m_reservoir_check_enabled;

  unsigned long m_next_update_msec = 0;

  ConfigInterface* m_config = nullptr;
  ReservoirCheck* m_reservoir_check = nullptr;
  MqttManager* m_mqtt_manager = nullptr;
  HADiscovery* m_ha_discovery = nullptr;
  OledDisplayRing* m_oled = nullptr;
};

}  // namespace og3
