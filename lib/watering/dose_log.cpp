#include "dose_log.h"

#include <og3/constants.h>
#include <og3/ha_discovery.h>

#include <algorithm>

#include "watering_constants.h"

namespace og3 {
namespace {
constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;
}

DoseLog::DoseLog(VariableGroup& vg, VariableGroup& cfg_vg)
    : m_max_doses_per_cycle("max_doses_per_cycle", kMaxDosesPerCycle, "", "maximum doses per cycle",
                            kCfgSet, cfg_vg),
      m_doses_this_cycle("doses_this_cycle", 0, "", "doses this cycle", 0, vg),
      m_dose_count("doses_today", 0, "", "doses in the past day", 0, vg) {}

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

void DoseLog::startWatering() {
  m_dose_record.pushBack(Dose(millis()));
  m_watering = true;
}
void DoseLog::addDose() {
  m_dose_record.back().dose_count += 1;
  m_dose_count = m_dose_count.value() + 1;
  m_doses_this_cycle = m_doses_this_cycle.value() + 1;
}
void DoseLog::stopWatering() {
  m_watering = false;
  m_doses_this_cycle = 0;
}

void DoseLog::update() {
  if (m_watering) {
    return;
  }
  const unsigned long now = millis();
  const unsigned long one_day_ago =
      std::max(static_cast<long>(0), static_cast<long>(now) - kMsecInHour * 24);
  while (!m_dose_record.empty()) {
    const auto front = m_dose_record.front();
    if (!isBefore(front.millis, one_day_ago)) {
      break;
    }
    m_dose_count = std::max(0, static_cast<int>(m_dose_count.value()) - front.dose_count);
    m_dose_record.popFront();
  }
}

}  // namespace og3
