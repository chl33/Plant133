// Copyright (c) 2025 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/kernel_filter.h>
#include <og3/mapped_analog_sensor.h>
#include <og3/variable.h>

namespace og3 {

// A wrapper for the capacitative moisture sensor.
// A kernel-based filter is used to smooth noise in the readings.
// There is a minor experimental correction for temperature, because
//  the moisture sensor reads higher when temperature increases
class MoistureSensor {
 public:
  MoistureSensor(const char* name, uint8_t pin, const char* raw_description,
                 const char* description, ModuleSystem* module_system, VariableGroup& cfg_vg,
                 VariableGroup& vg);

  // -- Parameters for temperature compensation.
  // Set the current temperatures
  void setTempC(float tempC) { m_tempC = tempC; }
  // Set the temperature for which there is no correction.
  void setReferenceTempC(float tempC) { m_reference_tempC = tempC; }
  // Set the adjustment to % moisture for each delta-C compared to the reference temperature.
  void setDeltaPercentPerDegC(float delta) { m_delta_percent_per_degC = delta; }

  // Read the current temperature, and add the reading to the kernel filter using the
  //  current uptime value.
  void read(long nowMsec);

  // Set the sigmal value for the moisture reading filter.
  // This value is in seconds.
  void setSigma(float sigma) { m_filter.setSigma(sigma); }

  // Raw ADC counts of the latest moisture sensor reading.
  unsigned rawCounts() const { return m_mapped_adc.raw_counts(); }
  // Latest moisture sensor reading.
  float unfilteredValue() const { return m_mapped_adc.value(); }
  // Value of the moisture level filter after the latest reading.
  float filteredValue() const { return m_filter.value(); }
  // Whether the latest moisture level reading failed.
  bool readingIsFailed() const { return m_mapped_adc.readingIsFailed(); }

  const KernelFilter& filter() const { return m_filter; }
  const MappedAnalogSensor& adc() const { return m_mapped_adc; }
  MappedAnalogSensor& adc() { return m_mapped_adc; }

 private:
  MappedAnalogSensor m_mapped_adc;
  String m_filtered_name;
  KernelFilter m_filter;
  String m_delta_percent_name;
  FloatVariable m_delta_percent_per_degC;
  float m_tempC = 20.0f;
  float m_reference_tempC = 20.0f;
};

}  // namespace og3
