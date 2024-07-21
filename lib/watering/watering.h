// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/blink_led.h>
#include <og3/ha_app.h>
#include <og3/ha_dependencies.h>
#include <og3/logger.h>
#include <og3/oled_display_ring.h>
#include <og3/relay.h>

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

    // State for when pump is disabled.
    kStateDisabled,

    // State for when testing a single pump cycle.
    kStatePumpTest,

    // Test state.
    kStateTest,
  };

  class StateVariable : public EnumVariable<State> {
   public:
    StateVariable(const char* name_, const State value, const char* units_,
                  const char* description_, unsigned flags_, VariableGroup* group)
        : EnumVariable<State>(name_, value, units_, description_, flags_, group) {}
    String string() const override;
    bool fromString(const String& value) override;

    StateVariable& operator=(const State value) {
      m_value = value;
      setFailed(false);
      return *this;
    }
  };

  static const char* s_state_names[];

  Watering(const char* name,
	   uint8_t moisture_pin, uint8_t mode_led, uint8_t pump_ctl_pin,
           HAApp* app);

  const char* stateName() const { return s_state_names[m_state.value()]; }

  void setPumpEnable(bool enable);
  void setReservoirCheckEnable(bool enable) { m_reservoir_check_enabled = enable; }
  bool reservoirCheckEnabled() const { return m_reservoir_check_enabled.value(); }
  void testPump() { setState(kStatePumpTest, 100, "test pump"); }
  void test() { setState(kStateTest, 100, "full test"); }
  State state() const { return static_cast<State>(m_state.value()); }
  void add_html_status_button(String* body) const { add_html_button(body, name(), statusUrl()); }
  bool isReservoirEmpty() const {
    return !m_reservoir_check->haveWater();
  }

  const VariableGroup& variables() const { return m_vg; }

  void loop();

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
  void handleStatusRequest(AsyncWebServerRequest* request);
  void handleConfigRequest(AsyncWebServerRequest* request);

  HAApp* const m_app;
  HADependenciesArray<3> m_dependencies;
  VariableGroup m_cfg_vg;
  VariableGroup m_vg;
  const String m_status_url;
  const String m_config_url;
  const String m_pump_test_url;
  String m_html;
  ReservoirCheck* m_reservoir_check = nullptr;
  OledDisplayRing* m_oled = nullptr;
  ConfigInterface* m_config = nullptr;
  MoistureSensor m_moisture;
  Relay m_pump;
  BlinkLed m_mode_led;

  unsigned long m_next_update_msec = 0;
  FloatVariable m_max_moisture_target;
  FloatVariable m_min_moisture_target;
  FloatVariable m_pump_dose_msec;
  StateVariable m_state;
  BoolVariable m_watering_enabled;
  BoolVariable m_reservoir_check_enabled;
  FloatVariable m_sec_since_dose;
};

}  // namespace og3
