// Copyright (c) 2024 Chris Lee and contibuters.
// Licensed under the MIT license. See LICENSE file in the project root for details.

#include <og3/units.h>

namespace og3 {

// When the reservoir level sensor goes low, this is the estimated number of seconds
//  left the pump can run before the water level is too low for the pump.
constexpr float kLowWaterSecsRemaining = 10.0f;
// At one point I thought maybe the moisture leval reading was unreliable for a
//  short time after the pump was run, so I introduced a minimum wait between the
//  the the pump was run and a reading was taken.
constexpr long kWaitBetweenPumpAndMoisureReadingMsec = kMsecInSec;
// < 5% means the sensor is probably not in soil.
constexpr float kMinPlausibleMoisture = 5.0f;
// Always wait 15 minutes between pump cycles.
constexpr long kPumpOffSec = 15 * kSecInMin;
// Wait 1 minute between checks in eval mode while waiting for moisture level to drop.
constexpr long kWaitForNextCycleMsec = 1 * kMsecInMin;

// Value of the moisture smoother kernel sigme value while in the watering state.
// When watering, smooth the moisture level over about 5 minutes.
constexpr float kKernelWateringSec = 5 * kSecInMin;
// Time constant of smoothing kernel used for soil moisture level between waterings.
// When not watering, smoothe the moisture level over about 20 minutes.
constexpr float kKernelNotWateringSec = 20 * kSecInMin;

// Default ADC reading at which to consider soil moisture to be 100%.
constexpr unsigned kFullMoistureCounts = 1470;
// Default ADC reading at which to consider soil moisture to be 0%.
constexpr unsigned kNoMoistureCounts = 2900;

constexpr unsigned kMaxDosesPerCycle = 5;

constexpr unsigned kWateringPauseSec = kSecInDay;

}  // namespace og3
