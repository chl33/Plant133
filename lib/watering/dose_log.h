#include <og3/constants.h>
#include <og3/module_system.h>
#include <og3/ring_buffer.h>
#include <og3/util.h>
#include <og3/variable.h>

#include <algorithm>

namespace og3 {

// Track the total number of watering doses in a day.
// This is done to avoid over-watering in case something goes wrong such as problems
//  reading the moisture level of the soil.
class DoseLog {
 public:
  DoseLog(VariableGroup& vg, VariableGroup& cfg_vg, ModuleSystem* module_system);

  // Return the total number of doses in the last 24 hours.
  unsigned dose_count() const { return m_dose_count.value(); }

  // When the pump has run m_max_doses in either a watering cycle or
  //  a 24 hour period is reached, pause watering for 12 hours.
  bool shouldPauseWatering() const;

  // Call this each time the watering state machine is updated to add a watering-dose
  //  entry when watering starts, and to expire watering doses after 24 hours.
  void update(bool is_watering);
  // Call this is increment the count of pump doses in the current watering cycle.
  // This should only be called if is_watering.
  void addDose();

  // This registers callbacks for Home Assistant MQTT auto-discovery of variables.
  void addHADiscovery(class HADiscovery* had);

 private:
  Logger* log() { return m_module_system->log(); }

  // The maximum number of doses to allow in a cycle/day before watering should be paused.
  Variable<unsigned> m_max_doses_per_cycle;
  // Number of doses in the current watering cycle.
  Variable<unsigned> m_doses_this_cycle;
  // Number of doses in the last 24 hours.
  Variable<unsigned> m_dose_count;
  bool m_watering = false;

  // Doses in watering cycles in the last 24 hours.
  struct Dose {
    Dose() { this->secs = esp_timer_get_time() / kUsecInSec; }
    uint64_t secs = 0;
    int doses_this_cycle = 0;
    int dose_count = 0;
  };
  RingQueue<Dose, 16> m_dose_record;
  ModuleSystem* m_module_system;  // Used to access the logger.
};

}  // namespace og3
