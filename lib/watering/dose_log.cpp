#include "dose_log.h"

#include <og3/constants.h>
#include <og3/ha_discovery.h>

#include <algorithm>

#include "watering_constants.h"

namespace og3 {
namespace {
constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;
}  // namespace

const char* DoseLog::varname(const char* elname, const VariableGroup& vg, std::string* str) {
  *str = std::string(vg.name()) + "_" + elname;
  return str->c_str();
}

DoseLog::DoseLog(VariableGroup& vg, VariableGroup& cfg_vg, ModuleSystem* module_system)
    : m_max_doses_per_cycle("max_doses_per_cycle", kMaxDosesPerCycle, "", "maximum doses per cycle",
                            kCfgSet, cfg_vg),
      m_doses_this_cycle(varname("doses_this_cycle", vg, &m_str_doses_this_cycle), 0, "",
                         "doses this cycle", 0, vg),
      m_dose_count(varname("doses_today", vg, &m_str_doses_today), 0, "", "doses in the past day",
                   0, vg),
      m_module_system(module_system) {}

void DoseLog::addHADiscovery(HADiscovery* had) {
  had->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
    return had->addMeas(json, m_doses_this_cycle, ha::device_type::kSensor, nullptr);
  });
  had->addDiscoveryCallback([this](HADiscovery* had, JsonDocument* json) {
    return had->addMeas(json, m_dose_count, ha::device_type::kSensor, nullptr);
  });
}

bool DoseLog::shouldPauseWatering() const {
  if (m_doses_this_cycle.value() >= m_max_doses_per_cycle.value()) {
    return true;
  }
  if (m_dose_count.value() >= m_max_doses_per_cycle.value()) {
    return true;
  }
  return false;
}

void DoseLog::addDose() {
  m_dose_record.back().dose_count += 1;
  m_dose_count = m_dose_count.value() + 1;
  m_doses_this_cycle = m_doses_this_cycle.value() + 1;
}

void DoseLog::update(bool is_watering) {
  if (m_watering != is_watering) {
    if (is_watering) {
      m_dose_record.pushBack({});
    } else {
      m_doses_this_cycle = 0;
    }
    m_watering = is_watering;
  }

  if (!m_watering && !m_dose_record.empty()) {
    const int64_t now_secs = esp_timer_get_time() / kUsecInSec;
    const int64_t one_day_ago = std::max(static_cast<int64_t>(0), now_secs - kWateringPauseSec);
    while (!m_dose_record.empty()) {
      const auto front = m_dose_record.front();
      if (front.secs > one_day_ago) {
        break;  // While dose record is not yet a day old, don't remove it.
      }
      log()->logf("Popping dose record (%zu left): %lld sec > %lld.", m_dose_record.size() - 1,
                  front.secs, one_day_ago);
      m_dose_count = std::max(0, static_cast<int>(m_dose_count.value()) - front.dose_count);
      m_dose_record.popFront();
    }
  }
}

}  // namespace og3
