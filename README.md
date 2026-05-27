# Bongo Cat

An interactive Bongo Cat animation running on an ESP32-C3 with an SH1106G OLED display. Press the left and right buttons to make the cat tap its paws — hearts float up from each paw and the tap count is shown at the top of the screen.

## Hardware Requirements

| Component | Spec |
|-----------|------|
| Microcontroller | ESP32-C3 DevKitM-1 |
| Display | SH1106G 128×64 OLED (I2C) |
| Buttons | Left × 1, Right × 1 |

### Wiring

| Signal | GPIO |
|--------|------|
| Left button | GPIO 1 |
| Right button | GPIO 2 |
| OLED SDA | GPIO 3 |
| OLED SCL | GPIO 4 |

> Buttons use internal pull-ups (`INPUT_PULLUP`) and are active-low (connect to GND).

## Software

- **Platform**: [PlatformIO](https://platformio.org/)
- **Framework**: Arduino
- **Library**: [Adafruit SH110X](https://github.com/adafruit/Adafruit_SH110x) `^2.1.14`

## Features

- **Idle animation**: When no button is pressed, the cat cycles through 5 breathing frames (200 ms per frame).
- **Tap animation**: Pressing the left, right, or both buttons triggers the corresponding paw-tap animation.
- **Heart animation**: Each tap spawns a heart from the corresponding paw that fades out over several frames (ring buffer holds up to 15 simultaneous hearts).
- **Tap counter**: The cumulative tap count is displayed in the top-left corner of the screen.
- **Deep sleep**: The device enters deep sleep after 60 seconds of inactivity.

## State Machine

```
          ┌──────────────────────┐
          ▼                      │ timeout
        IDLE ──── button ─────► TAP
          ▲                      │
          │                      │ button released
          └──────── PREP ◄───────┘
```

| State | Description |
|-------|-------------|
| `IDLE` | Plays idle animation and polls for button input |
| `PREP` | Transition cooldown after button release (1600 ms); returns to TAP if pressed again |
| `TAP` | Plays tap animation and spawns hearts |

## Project Structure

```
bongo_cat/
├── platformio.ini          # PlatformIO configuration
├── include/
│   ├── cat.h               # Cat and heart bitmap data (PROGMEM)
│   └── heartQueue.h        # HeartQueue class declaration
├── src/
│   ├── main.cpp            # Main logic: state machine and rendering
│   └── heartQueue.cpp      # Ring-buffer heart queue implementation
└── test/
    └── README
```

## Build & Flash

```bash
# Requires PlatformIO CLI

# Build
pio run

# Upload
pio run --target upload

# Open serial monitor (115200 baud)
pio device monitor
```
