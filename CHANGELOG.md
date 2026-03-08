# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.7] - 2026-03-08

### Added
- **Web UI Enhancements**: Added real-time "Online/Offline" status indicator in the Navbar and plant-specific status dots in the sidebar.
- **Improved Sidebar**: Sidebar buttons now show MQTT connection status and visual indicators for whether a plant is enabled.
- **Centered Board Name**: Redesigned the Navbar layout to center the device board name for better aesthetics.

### Changed
- **Modernized Framework**: Upgraded to **og3 v0.5.0** and its extension libraries (**og3x-oled**, **og3x-shtc3**).
- **PsychicHttp Integration**: Switched to the modernized Request/Response networking model provided by PsychicHttp 2.1.1 on ESP32.
- **Moisture Filtering Logic**: The state machine now pauses filtered moisture updates when sensor readings are outside the calibrated calibration range.

### Fixed
- **LDF Mode**: Set `lib_ldf_mode = deep+` to ensure all transitive dependencies (e.g., Adafruit BusIO) are correctly resolved across all hardware environments.
