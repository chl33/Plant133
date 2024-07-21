// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include "moisture_sensor.h"

#include <og3/constants.h>
#include <og3/units.h>

#include "watering_constants.h"

namespace og3 {
namespace {
constexpr unsigned kCfgSet = VariableBase::Flags::kConfig | VariableBase::Flags::kSettable;
}

MoistureSensor::MoistureSensor(const char* name, uint8_t pin, const char* raw_description,
                               const char* description, ModuleSystem* module_system_,
                               VariableGroup* cfg_vg, VariableGroup* vg)
    : m_mapped_adc(
          {
              .name = name,
              .pin = pin,
              .units = units::kPercentage,
              .raw_description = raw_description,
              .description = description,
              .raw_var_flags = 0,
              .mapped_var_flags = 0,
              .config_flags = kCfgSet,
              .default_in_min = kNoMoistureCounts,
              .default_in_max = kFullMoistureCounts,
              .default_out_min = 0.0f,
              .default_out_max = 100.0f,
              .config_decimals = 0,
              .decimals = 1,
              .valid_in_min = 350,
              .valid_in_max = 1 << 12,
          },
          module_system_, cfg_vg, vg),
      m_filtered_name(String(name) + "_filtered"),
      m_filter(
          {
              .name = m_filtered_name.c_str(),
              .units = units::kPercentage,
              .description = "filtered moisture",
              .var_flags = 0,
              .sigma = kKernelWateringSec,
              .decimals = 1,
              .size = KernelFilter::kDefaultNumSamples,
          },
          module_system_, vg),
      m_delta_percent_name(String(name) + "_delta_per_deg"),
      m_delta_percent_per_degC(m_delta_percent_name.c_str(), 0.075, "", "moisture per degC",
                               VariableBase::kSettable | VariableBase::kConfig, 3, cfg_vg) {}

void MoistureSensor::read(long nowMsec) {
  const float val = m_mapped_adc.read();  // TODO(chrishl): check is reasonable value
  // We noticed that the moisture sensor reading is dependent on temperature, so try to compensate
  //  here.
  const float delta_temp = m_reference_tempC - m_tempC;
  const float delta_moisture = m_delta_percent_per_degC.value() * delta_temp;
  const float adjustedValue = val + delta_moisture;
  const float secs = 1e-3 * nowMsec;
  m_filter.addSample(secs, adjustedValue);
}

}  // namespace og3
