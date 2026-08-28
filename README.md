# TDS Sensor

An ESP8266-based total dissolved solids (TDS) and temperature sensor for a Home Assistant installation. The firmware reads a DFRobot SEN0244 analog TDS sensor, applies temperature compensation from a DS18B20 probe, and publishes readings through MQTT.

## Features

- TDS measurement in ppm through an analog input
- DS18B20 temperature measurement and temperature compensation
- WiFiManager onboarding
- MQTT state publishing and Home Assistant MQTT discovery
- Browser-based configuration and sensor calibration
- OTA firmware updates
- LittleFS configuration and sensor log storage
- Factory reset input on the controller board

## Hardware

- Wemos D1 Mini (ESP8266)
- DFRobot SEN0244 analog TDS sensor
- DFRobot DS18B20 temperature probe
- 5 V supply for the controller board
- Optional 12 V supply and 12-24 V to 5 V buck converter

The complete parts list and enclosure credits are in [`docs/BOM.md`](docs/BOM.md).

## Wiring

| Controller pin | Connection |
| --- | --- |
| `A0` | SEN0244 analog TDS output |
| `D2` / `GPIO4` | DS18B20 one-wire data |
| `D5` / `GPIO14` | Factory reset input, jumper to GND at boot |
| `5V` and `GND` | Sensor and controller power connections as appropriate |

See [`docs/hardware-pinout.md`](docs/hardware-pinout.md) for the board assumptions and electrical notes.

## 3D-printable enclosure

The repository includes the current 3MF files for the enclosure and sensor plate:

- [`Box.3mf`](Enclosure%20and%20mount/Box.3mf)
- [`Sensor Plate.3mf`](Enclosure%20and%20mount/Sensor%20Plate.3mf)

The parts list identifies the board holders, capacitor holder, connector holder, cable-tie holder, and standoffs used by the assembly. Confirm dimensions and clearances against your exact components before printing.

## Build with PlatformIO

### Requirements

- VS Code with the PlatformIO extension, or PlatformIO CLI
- A Wemos D1 Mini connected by USB
- The dependencies listed in [`platformio.ini`](platformio.ini)

From the project directory:

```bash
pio run
```

The build target is `WEMOS_D1_Mini`. The pre-build script automatically increments the local build number and updates `include/fw_version_auto.h`.

## Upload firmware

Replace `<PORT>` with the serial port assigned to your board:

```bash
pio run -t upload --upload-port <PORT>
pio run -t uploadfs --upload-port <PORT>
```

The first command uploads the firmware. The second uploads the LittleFS web interface and filesystem data. For the exported release images, see [`artifacts/reflash/FLASHING.md`](artifacts/reflash/FLASHING.md).

## First setup

1. Upload the firmware and LittleFS image.
2. Power-cycle the controller.
3. Join the WiFiManager access point if the device has not been configured yet.
4. Configure Wi-Fi, MQTT, time zone, and OTA credentials in the web interface.
5. Confirm that the device publishes MQTT state and appears through Home Assistant discovery.
6. Calibrate the TDS reading using a known reference solution.

The MQTT topic contract is documented in [`docs/mqtt-contract-v1.md`](docs/mqtt-contract-v1.md).

## Security note

The firmware includes example default MQTT and OTA credentials for initial development. Change them before deploying the device on a network that is not fully trusted. Do not commit a device-specific configuration file, Wi-Fi password, MQTT password, or OTA password to this repository.

## Repository layout

```text
src/TDSTMPSensor/       Firmware source
data/web/               Web interface files stored in LittleFS
docs/                   Wiring, BOM, and MQTT documentation
Enclosure and mount/    3D-printable 3MF files
artifacts/reflash/      Exported firmware and LittleFS images
platformio.ini          PlatformIO project configuration
```

## License

No license has been selected for this project yet. Add a `LICENSE` file before accepting or requesting contributions if you want to define reuse terms.
