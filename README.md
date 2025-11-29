# Plant133

This is a full distribution of the Plant133 plant-watering device. This device can monitor and water up to 4 plants at a time. This repository hosts the firmware, PCBA design, and OpenSCAD code for all components.

A writeup about this project is at "[Plant1337: "Water 4 plants](https://selectiveappeal.org/posts/plant1337/)."

![Plant133 device](images/plant1337_deployed_trim-1400x600.webp)

## Features

*   **Multi-Plant Support**: Monitors and independently waters up to 4 plants.
*   **Sensors**:
    *   Capacitive soil moisture sensing for each plant.
    *   SHT3x temperature and humidity sensor for environmental monitoring.
    *   Reservoir water level float switch detection.
*   **User Interface**:
    *   **OLED Screen**: Rotates through status screens showing IP address, moisture levels, and environment data.
    *   **Web Dashboard**: A modern Svelte-based responsive web interface for real-time monitoring and configuration.
*   **Integration**:
    *   **MQTT**: Full support for Home Assistant auto-discovery and state reporting.
*   **Safety**:
    *   **Watchdog Timer**: Hardware watchdog protects against system hangs.
    *   **Dose Limiting**: Prevents over-watering by limiting the maximum number of pump cycles per day.
    *   **Reservoir Check**: Prevents pump damage by detecting low water levels.

## Usage

Please see the [instructions](instructions.md) for how to assemble and set up a Plant133 device hardware.

## Development

### Prerequisites
*   [PlatformIO](https://platformio.org/) (Core or VSCode extension)
*   [Node.js](https://nodejs.org/) (for building the web interface)

### Building the Web Interface
The web interface is built with Svelte and must be compiled before uploading to the ESP32's filesystem.

```bash
cd svelte
npm install
npm run build
```

This generates the static HTML/CSS/JS files in `data/static/`.

### Building and Flashing Firmware

1.  **Configure Secrets**: Copy `secrets.ini.example` to `secrets.ini` and set your WiFi credentials, OTA password, and MQTT details.
2.  **Build Firmware**:
    ```bash
    pio run
    ```
3.  **Upload Filesystem** (contains the web interface):
    ```bash
    pio run -t uploadfs
    ```
4.  **Upload Firmware**:
    ```bash
    pio run -t upload
    ```

## API Reference

The device exposes a JSON API for integration and control:

*   `GET /api/status`: Returns system status (temp, humidity, water level).
*   `GET /api/plants`: Returns configuration for all plants.
*   `GET /api/moisture`: Returns current moisture readings.
*   `PUT /api/plants/{id}`: Update configuration for a specific plant.
*   `POST /test/pump`: Run a pump for a specific duration (JSON body: `{ "pumpId": 1, "duration": 1000 }`).
*   `POST /api/restart`: Restart the device.

## Software Libraries

This project is built using a custom C++ framework for ESP devices:
- [og3](https://github.com/chl33/og3): Application framework.
- [og3x-oled](https://github.com/chl33/og3x-oled): OLED screen support.
- [og3x-shtc3](https://github.com/chl33/og3x-shtc3): SHTC3 sensor support.
- [svelteesp32](https://github.com/BCsabaEngine/svelteesp32): Web interface integration.

## Hardware

### PCBA

The full [KiCAD](https://www.kicad.org/) project for the printed circuit board is in the [KiCAD](KiCAD/) subdirectory.

![Plant1337 PCBA rendered by KiCAD](images/plant1337-board-kicad.png)

### 3D Printed Components

Components are designed in [OpenSCAD](https://openscad.org/). Source code is in the [scad](scad/) subdirectory.

#### Project box
Housing for the PCBA ([scad/box/](scad/box/)).
![EBox design in OpenSCAD](images/scad-plant133-ebox.png)

#### Reservoir Hanger
Mounts the box to a water reservoir ([scad/ebox_hook](scad/ebox_hook)).
![Reservoir hanger](images/ebox-reservoir-hook.png)

#### Reservoir Insert
Holds pumps and water level float ([scad/reservoir_insert](scad/reservoir_insert)).
![Reservoir insert with 2 pumps](images/scad-insert-2-pumps.png)

#### Moisture Sensor Cap
Holds the watering tube at the sensor location ([scad/moisture_sensor_cap](scad/moisture_sensor_cap)).
![Moisture sensor cap top](images/scad-moisture-sensor-cap-4.png)

#### Watering Spike
Alternative watering method using a soil spike ([scad/spike](scad/spike)).
![Watering spike half](images/scad-spike.png)
