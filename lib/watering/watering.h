#pragma once
// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/blink_led.h>
#include <og3/ha_app.h>
#include <og3/ha_dependencies.h>
#include <og3/logger.h>
#include <og3/relay.h>

#include "dose_log.h"
#include "moisture_sensor.h"
#include "reservoir_check.h"

namespace og3 {

// Watering manages the state machine for watering a plant.
class Watering : public Module {
 public:
  // The set of possible states in the state machine.
  enum State {
    // Check moisture level, reservior level during watering cycle.
    kStateEval,

    // Start the pump and run for a fixed amount of time.
    kStateDose,

    // Turn pump off at end of dose.
    kStateEndOfDose,

    // After moisture level reaches maximum level, wait for it to reach minimum
    //  moisture level and also wait for minimum time between watering cycles, then
    //  go back to kStateEval.
    kStateWaitForNextCycle,

    // After too many does in a cycle, pause for 12 hours.
    kStateWateringPaused,

    // State for when pump is disabled.
    kStateDisabled,

    // State for when testing a single pump cycle.
    kStatePumpTest,

    // Test state.
    kStateTest,
  };

  class StateVariable : public EnumStrVariable<State> {
   public:
    StateVariable(const char* name_, const State value, const char* description_, unsigned flags_,
                  VariableGroup& group)
        : EnumStrVariable<State>(name_, value, description_, kStateTest, s_state_names, flags_,
                                 group) {}
    String string() const override;
    bool fromString(const String& value) override;

    StateVariable& operator=(const State value) {
      m_value = value;
      setFailed(false);
      return *this;
    }
  };

  static const char* s_state_names[];

  Watering(unsigned index, const char* name, uint8_t moisture_pin, uint8_t mode_led,
           uint8_t pump_ctl_pin, HAApp* app);

  const char* stateName() const { return s_state_names[m_state.value()]; }

  // +1 during watering, -1 waiting for next cycle, 0 if disabled.
  int direction() const;

  bool isEnabled() const { return m_watering_enabled.value(); }
  float moisturePercent() const { return m_moisture.filteredValue(); }
  float maxTarget() const { return m_max_moisture_target.value(); }
  float minTarget() const { return m_min_moisture_target.value(); }

  const String& plantName() const { return m_plant_name.value(); }

  void setPumpEnable(bool enable);
  void setReservoirCheckEnable(bool enable) { m_reservoir_check_enabled = enable; }
  bool reservoirCheckEnabled() const { return m_reservoir_check_enabled.value(); }
  void testPump() { setState(kStatePumpTest, 100, "test pump"); }
  void test() { setState(kStateTest, 100, "full test"); }
  State state() const { return static_cast<State>(m_state.value()); }
  void add_html_status_button(String* body) const {
    add_html_button(body, plantName().c_str(), statusUrl());
  }
  bool isReservoirEmpty() const { return !m_reservoir_check->haveWater(); }

  const VariableGroup& variables() const { return m_vg; }
  const VariableGroup& configVariables() const { return m_cfg_vg; }

  Relay& relay() { return m_pump; }

  void loop();

  void getApiPlants(JsonObject json) const;
  bool putApiPlants(JsonObject json);

 protected:
  // This method performs the work of the state machine.
  // Before the method exits, it updates the current state and then schedules
  //  this to get called again after a given time interval.
  // The first call to this is scheduled in the constructor.
  void setState(State state, unsigned msec, const char* msg);

 private:
  void _fullTest();
  const char* statusUrl() const { return m_status_url.c_str(); }
  const char* configUrl() const { return m_config_url.c_str(); }
  const char* pumpTestUrl() const { return m_pump_test_url.c_str(); }
  const char* varname(const char* name, std::string* str);

  void handleStatusRequest(AsyncWebServerRequest* request);
  void handleConfigRequest(AsyncWebServerRequest* request);

  HAApp* const m_app;
  HADependenciesArray<3> m_dependencies;
  const unsigned m_index;
  VariableGroup m_cfg_vg;
  VariableGroup m_vg;
  const String m_status_url;
  const String m_config_url;
  const String m_pump_test_url;
  String m_html;

  std::string m_device_id;
  std::string m_moisture_varname;
  std::string m_pump_varname;
  std::string m_watering_varname;
  std::string m_sec_dose_varname;

  ReservoirCheck* m_reservoir_check = nullptr;
  ConfigInterface* m_config = nullptr;
  MoistureSensor m_moisture;
  Relay m_pump;
  BlinkLed m_mode_led;
  DoseLog m_dose_log;

  unsigned long m_next_update_msec = 0;
  Variable<String> m_plant_name;
  FloatVariable m_max_moisture_target;
  FloatVariable m_min_moisture_target;
  FloatVariable m_pump_dose_msec;
  FloatVariable m_between_doses_sec;
  StateVariable m_state;
  FloatVariable m_sec_since_dose;
  BoolVariable m_watering_enabled;
  BoolVariable m_reservoir_check_enabled;
};

}  // namespace og3
