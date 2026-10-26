# ESP32-C3 BLE Mouse Jiggler

A simple mouse jiggler firmware for ESP32-C3 boards (e.g., ESP32-C3 Zero).  
It emulates a Bluetooth HID mouse that periodically moves the cursor a few pixels to prevent the computer from entering sleep / screensaver mode.

## How it works

The device advertises itself as a **Logitech MX Master 4** Bluetooth mouse.  
Once connected, it moves the mouse pointer in a small square pattern (1 pixel right, left, down, up) every few seconds.  
If the host disconnects, it waits 5 seconds and then starts advertising again.

## Hardware

- **Board:** ESP32-C3 (ESP32-C3-DevKitM-1 or compatible, e.g., ESP32-C3 Zero)
- **Framework:** Arduino (via PlatformIO)

## Build & Flash

This project uses [PlatformIO](https://platformio.org/).

```bash
# Install dependencies and build
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

## Configuration

Edit `platformio.ini` to change the board or monitor speed.  
The device name and manufacturer are set in `src/main.cpp` – you can customise:

```cpp
BleMouse bleMouse("MX Master 4", "Logitech", 100);
```

## Dependencies

- [ESP32 BLE Mouse](https://github.com/T-vK/ESP32-BLE-Mouse) library (automatically fetched by PlatformIO)

## License

MIT
