# Reflash Artifacts

These files were exported from the local build outputs on 2026-08-07 without rebuilding.

## Maintenance Rule

- Update this folder after every new build export.
- Keep only the latest artifact set in this folder.
- When refreshing artifacts, replace older versioned firmware, LittleFS, and ELF files instead of keeping multiple releases here.

## Files

- `TDSTMPSensor-1.0.40-firmware.bin`
- `TDSTMPSensor-1.0.40-littlefs.bin`
- `SHA256SUMS.txt`

## Expected board

- Board: `Wemos D1 Mini`
- MCU: `ESP8266`
- Set `--upload-port` to the serial port assigned to your board.

## Reflash firmware only

```bash
~/.platformio/penv/bin/pio run -t nobuild -t upload --upload-port /dev/cu.usbserial-XXXX
```

That command uses the current `.pio` build output already present in the project and does not rebuild.

## Reflash LittleFS only

```bash
~/.platformio/penv/bin/pio run -t nobuild -t uploadfs --upload-port /dev/cu.usbserial-XXXX
```

That also reuses the existing built image already present in `.pio`.

## Reuse exported binaries manually

If you want to flash from the exported files in this folder rather than from `.pio`, use an ESP8266 flashing tool and write:

- firmware image at `0x00000000`
- LittleFS image at `0x00200000`

The flash layout matches `eagle.flash.4m2m.ld` with LittleFS enabled.
