# ESP32 BLE Mouse Jiggler

A mouse jiggler firmware for ESP32 boards (ESP32-C3, ESP32-S3, ESP32, etc.).  
It emulates a Bluetooth HID mouse that periodically moves the cursor to prevent the computer from entering sleep / screensaver mode.  
Includes **battery simulation**, **randomised timing**, **idle pauses**, and **deep sleep** to behave like a real wireless mouse.

## How it works

The device advertises itself as a **Logitech MX Master 4** Bluetooth mouse.  

- **Connected:** moves the mouse in a small square pattern (1 px right, left, down, up). Each movement is spaced **~10 s apart ±10 s** (0–20 s delay) so the pattern is slow and natural.  
  After **each individual move** there's a **~10 % chance to pause for 10–100 s** (simulating a user reading or watching something).

- **Battery simulation:** on first boot it picks a random battery level between **60–100 %**, then drops **1 % on a non-linear discharge curve** (floor at **3 %**). The level is reported over BLE so the host OS can read it.

  | Level      | Drain rate       |
  |------------|------------------|
  | 100–80 %   | 1 % / 6 h       |
  | 80–10 %    | 1 % / 4 h       |
  | 30–10 %    | 1 % / 2 h       |
  | 10–3 %     | 1 % / 1 h       |

  Random **±1 % fluctuations** occur every ~2 h to mimic noisy battery readings.

- **Persistence:** battery level survives deep sleep and short power cycles (stored in RTC memory). If the saved level is **below 15 %** on boot, a fresh random value is chosen instead.
- **Disconnected:** after **5 minutes** without a connection the device enters **deep sleep for 30 seconds**, then wakes up and advertises again. This saves power when not in use.

## Hardware

- **Board:** Any ESP32 with BLE (ESP32-C3, ESP32-S3, ESP32, etc.)
- **Framework:** Arduino (via PlatformIO)

## Build & Flash

This project uses [PlatformIO](https://platformio.org/). Three environments are predefined in `platformio.ini`:

### For ESP32-C3 (e.g., ESP32-C3 Zero, ESP32-C3-DevKitM-1)

```bash
pio run --environment esp32-c3-zero
pio run --target upload --environment esp32-c3-zero
```

### For ESP32-S3 (e.g., ESP32-S3-DevKitC-1)

```bash
pio run --environment esp32-s3-devkit
pio run --target upload --environment esp32-s3-devkit
```

### For original ESP32 (e.g., ESP32 Dev Module)

```bash
pio run --environment esp32dev
pio run --target upload --environment esp32dev
```

### For other ESP32 boards

Add a new `[env:your-board]` section to `platformio.ini` – just copy one of the existing environments and change the `board` line.  
Most ESP32 variants with BLE will work without code changes.

```bash
# List all available environments
pio env list

# Monitor serial output (replace with your environment name)
pio device monitor --environment esp32-c3-zero
```

## Configuration

Edit `platformio.ini` to change the board or monitor speed.  
The device name and manufacturer are set in `src/main.cpp` – you can customise:

```cpp
BleMouse bleMouse("MX Master 4", "Logitech", 100);
```

Key constants at the top of `main.cpp`:

| Constant               | Default   | Description                         |
|------------------------|-----------|-------------------------------------|
| `DISCONNECT_TIMEOUT`   | 300 000 ms (5 min) | Idle before deep sleep   |
| `DEEP_SLEEP_US`        | 30 s      | Wake interval when sleeping         |
| `FLUCTUATION_INTERVAL` | 2 h       | Interval between ±1 % jitters      |
| Battery floor          | 3 %       | Never drain below this              |
| Movement interval      | ~10 s     | Average delay between moves (±10 s) |
| Idle pause             | 10 %      | Chance to pause 10–100 s after each move |

The drain rate is **dynamic** – see the table above.

## Dependencies

- [ESP32 BLE Mouse](https://github.com/T-vK/ESP32-BLE-Mouse) library (automatically fetched by PlatformIO)

## License

MIT
