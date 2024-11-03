#include <og3/ring_buffer.h>
#include <og3/util.h>
#include <og3/variable.h>

#include <algorithm>

namespace og3 {

// Track the total number of doses in a day.
class DoseLog {
 public:
  DoseLog(VariableGroup& vg, VariableGroup& cfg_vg);

  unsigned dose_count() const { return m_dose_count.value(); }

  bool shouldPauseWatering() const;

  void startWatering();
  void addDose();
  void stopWatering();
  void update();

  void addHADiscovery(class HADiscovery* had);

 private:
  // Doses in the last 24 hours.
  Variable<unsigned> m_max_doses_per_cycle;
  Variable<unsigned> m_doses_this_cycle;
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
