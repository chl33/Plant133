# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-10-25

### Added
- **Hardware Board v2.x & ESP32-S3 Support**: Added PlatformIO configurations (`esp32s3`, `usb_s3`, `wifi_s3`), `BOARD_V20` pin mappings for `ESP32-S3-WROOM-1-N8R2` with USB-C, updated KiCAD schematics/PCB layouts (v2.0–v2.2 with boot/reset push-buttons and improved flyback diode placement), and BOM (`KiCAD/Plant133.csv`).
- **3D-Printed Enclosure & Accessories**:
  - Updated EBox (`scad/box/`) for Board v2.1+ and added OLED support bracket (`OLEDSupport.scad`) and board test fixture (`fixture.scad`).
  - Added reservoir cover/cap (`scad/reservoir_cover/`) with cutouts for the EBox, tubing, and wires.
  - Updated reservoir insert (`scad/reservoir_insert/`) and extracted reusable vitamin models (`vertical_pump.scad`, `float.scad`).
  - Updated solder stencil fixture and added stencil edge-support frame (`scad/stencil_fixture/`).
- **Adafruit IO & SSL/TLS MQTT Support**: Added NTP time synchronization (`og3::Ntp`), SSL/TLS with optional certificate verification, and Adafruit IO MQTT mode with feed filtering/rate-limiting in both firmware and the Svelte MQTT configuration UI.
- **System Status Versioning**: Exposed `software` and `hardware` version fields in `GET /api/status`.
- **Native Unit Tests & Dependabot**: Ported unit tests (`test/test_watering/test_watering.cpp`) to `og3` under `[env:native]`, enabled `pio test -e native` in `util/check.sh`, and added GitHub Actions Dependabot configuration.

### Changed
- **Framework Upgrade**: Upgraded to **og3 v0.7.0**, **og3x-oled v0.6.1**, and **og3x-shtc3 v0.6.0**, linking modules by pointer via `require(...)` and making `DoseLog` an `og3::Module`.
- **Web UI & API Alignment**: Updated Svelte WiFi and MQTT configuration pages to use `og3` variable keys (`essId`, `wifiPassword`, `hostAddr`, `authUser`, `authPassword`) and updated `build-svelte.sh` to use `--etag=always`.
- **MQTT Optimization**: Avoided sending duplicate reservoir status updates per plant.

### Fixed
- **Plant API Validation**: Updated `Watering::putApiPlants` to accept floating-point values for moisture targets and pump timing parameters and return the validation result.

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
