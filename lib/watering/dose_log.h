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
  DoseLog(VariableGroup& vg, VariableGroup& cfg_vg);

  // Return the total number of doses in the last 24 hours.
  unsigned dose_count() const { return m_dose_count.value(); }

  // When the pump has run m_max_doses in either a watering cycle or
  //  a 24 hour period is reached, pause watering for 12 hours.
  bool shouldPauseWatering() const;

  // Call this when a new watering cycle has started.
  void startWatering();
  // Call this is increment the count of pump doses in the current watering cycle.
  void addDose();
  // Call this when the current watering cycle has ended.
  void stopWatering();
  // Call this periodically to time-out watering data which is more that 24 hours old.
  void update();

  // This registers callbacks for Home Assistant MQTT auto-discovery of variables.
  void addHADiscovery(class HADiscovery* had);

 private:
  // The maximum number of doses to allow in a cycle/day before watering should be paused.
  Variable<unsigned> m_max_doses_per_cycle;
  // Number of doses in the current watering cycle.
  Variable<unsigned> m_doses_this_cycle;
  // Number of doses in the last 24 hours.
  Variable<unsigned> m_dose_count;
  bool m_watering = false;

  // Doses in watering cycles in the last 24 hours.
  struct Dose {
    Dose() {}
    explicit Dose(unsigned long millis) { this->millis = millis; }
    unsigned long millis = 0;
    int doses_this_cycle = 0;
    int dose_count = 0;
  };
  RingQueue<Dose, 16> m_dose_record;
};

}  // namespace og3
