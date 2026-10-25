# Plant133 Development Mandates

This is a firmware application project for the Plant133 watering system.

## Coding Standards
- **Naming**: Follow `og3` conventions: `camelCase` methods, `snake_case` locals, `m_` prefix for private members.
- **Framework**: Strictly use the `og3` application framework classes (`HAApp`, `WifiManager`, etc.).

## Hardware Architecture
- **Sensor Drivers**: Prefer using `og3x-` extension libraries for climate and OLED display.
- **Pins**: Hardware pin mappings are versioned via `BOARD_V20` and `BOARD_V13` macros. Ensure changes are verified against v1.2, v1.3 (ESP32), and v2.0+ (ESP32-S3) board layouts.

## Web Interface
- **Svelte UI**: The frontend is built with Svelte. Always run `./build-svelte.sh` after making changes to the `svelte/` directory to regenerate the C++ assets.
- **API Consistency**: Ensure JSON API endpoints in `main.cpp` remain in sync with the data-fetching logic in `svelte/src/App.svelte`.

## Quality
- **CHANGELOG**: Document all version updates in `CHANGELOG.md`.
- **Release Verification**: Before releasing, verify that `./util/check.sh` passes and the project compiles for both `node32s` (`usb_node32s` / `wifi_node32s`) and `esp32s3` (`usb_s3` / `wifi_s3`) environments.
