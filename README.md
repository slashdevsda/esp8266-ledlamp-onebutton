# LED Lamp — ESP8266 (WeMos D1 Mini)

Ambient lamp with 11 addressable WS2812B LEDs, controlled by a single physical button.
Self-contained firmware, no network dependency (Wi-Fi unused).

> **Disclaimer**
> This project was written entirely by an AI agent (Hermes Agent, by Nous Research)
> during an interactive session with the owner. The code has **not been reviewed by
> any human**. It worked as intended on the author's setup, but use it at your own
> risk — no warranty of any kind, and no liability for bricked boards, burnt strips
> or other mishaps. Read the code before flashing it.

## Hardware

| Component | Detail |
|---|---|
| MCU | WeMos D1 Mini (ESP8266, CH340 USB-serial) |
| LEDs | 11× WS2812B (3-wire, 5V) |
| Button | 2-pin tactile switch, momentary NO |
| LED power | Separate 5V charger (USB-C female connector) |
| MCU power | USB (also used for flashing) |

### Wiring

| LED strip | Source |
|---|---|
| 5V (VCC) | Separate 5V supply |
| GND | GND (common ground mandatory) |
| DIN | D5 (GPIO14) via 330Ω |

| Button | Source |
|---|---|
| Pin 1 | D1 (GPIO5) |
| Pin 2 | GND |

Internal pull-up enabled (`INPUT_PULLUP`) — no external resistor needed.
Active-low: pressed = LOW.

## Features

### Lighting modes (cycled by double-press)

| # | Mode | Color |
|---|---|---|
| 0 | Warm white | (255, 110, 20) — amber/orange |
| 1 | White | (255, 230, 200) — neutral/cool |
| 2 | Random RGB | Random saturated HSV color, new color every 10s |

### Controls (button)

| Gesture | Action |
|---|---|
| Long press (>500ms) | ON / OFF |
| 1 short press | Cycle intensity: low → medium → max |
| 2 short presses (<400ms apart) | Cycle mode: warm white → white → RGB |

### Intensity levels

| Level | Value (0-255) |
|---|---|
| Low | 40 |
| Medium | 80 |
| Max | 255 |

### Visual feedback (LED 0)

| Event | Color | Duration |
|---|---|---|
| Intensity change | Red (full power) | 300ms |
| Mode change | Blue (full power) | 300ms |

The other LEDs stay lit during feedback (brightness compensation).

### Transitions

| Type | Duration | Easing |
|---|---|---|
| Mode change (color) | 5s | Quintic smootherstep, 60 FPS |
| Intensity change | 2s | Quintic smootherstep |
| Startup (ON) | 2s | Brightness fade-in 0 → current level |
| RGB: hold between colors | 10s | — |

**Quintic smootherstep**: `t³(t(t·6−15)+10)` — zero velocity AND zero acceleration at both endpoints → perceived as fluid.

The color blend is done in **float** (full precision) to avoid jitter at low brightness.

### Debounce

80ms ignore window after each button release (mechanical bounce lasts 5-20ms).

## Build & Flash

```bash
# Prerequisite: PlatformIO in a venv
python3 -m venv ~/.platformio-venv
~/.platformio-venv/bin/pip install platformio

# Build
~/.platformio-venv/bin/pio run

# Flash (port /dev/ttyUSB0)
~/.platformio-venv/bin/pio run -t upload

# Serial monitor (115200 baud)
~/.platformio-venv/bin/python -c "
import serial, time
s = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
end = time.time() + 5
while time.time() < end:
    d = s.readline()
    if d: print(d.decode().strip())
s.close()
"
```

### PlatformIO config (`platformio.ini`)

- Board: `d1_mini`
- Framework: `arduino`
- Lib: `fastled/FastLED@^3.10.1`
- Port: `/dev/ttyUSB0`

> **Note**: FastLED 3.6.0 crashes on this board. Always use ≥3.10.1.

## Power budget

| Config | Draw |
|---|---|
| 11 LEDs full white + ESP | ~810mA @ 5V |
| 11 LEDs at max (255/255) | ~810mA @ 5V |
| 5V/1A charger | ~14 LEDs max (full) |
| 5V/2A charger | ~30 LEDs max (full) |

Idle (lamp off, no Wi-Fi): ~0.4-0.6W, roughly 1.30€/year at 24/7.

## Structure

```
lampe-led/
├── platformio.ini
├── src/
│   └── main.cpp      # Complete firmware
├── include/
├── lib/
└── test/
```
