# Lampe LED — ESP8266 (WeMos D1 Mini)

Lampe d'ambiance à 11 LEDs WS2812B adressables, contrôlée par un bouton physique.
Firmware embarqué, aucune dépendance réseau (pas de Wi-Fi utilisé).

## Matériel

| Composant | Détail |
|---|---|
| MCU | WeMos D1 Mini (ESP8266, CH340 USB-serial) |
| LEDs | 11× WS2812B (3 fils, 5V) |
| Bouton | Tactile 2 pins, momentary NO |
| Alimentation LEDs | Chargeur 5V séparé (USB-C femelle) |
| Alimentation MCU | USB (sert aussi au flash) |

### Câblage

| LED strip | Source |
|---|---|
| 5V (VCC) | Chargeur 5V séparé |
| GND | GND (masse commune obligatoire) |
| DIN | D5 (GPIO14) via 330Ω |

| Bouton | Source |
|---|---|
| Pin 1 | D1 (GPIO5) |
| Pin 2 | GND |

Pull-up interne activé (`INPUT_PULLUP`) — pas de résistance externe.
Active-low : appui = LOW.

## Features

### Modes d'éclairage (cycle au double-appui)

| # | Mode | Couleur |
|---|---|---|
| 0 | Blanc chaud | (255, 110, 20) — ambre/orangé |
| 1 | Blanc | (255, 230, 200) — neutre/froid |
| 2 | RGB random | Couleur HSV saturée aléatoire, nouvelle couleur toutes les 10s |

### Contrôles (bouton)

| Gesture | Action |
|---|---|
| Appui long (>500ms) | ON / OFF |
| 1 appui court | Cycle intensité : faible → moyen → max |
| 2 appuis courts (<400ms d'écart) | Cycle mode : blanc chaud → blanc → RGB |

### Niveaux d'intensité

| Niveau | Valeur (0-255) |
|---|---|
| Faible | 40 |
| Moyen | 80 |
| Max | 255 |

### Feedback visuel (LED 0)

| Événement | Couleur | Durée |
|---|---|---|
| Changement d'intensité | Rouge (full power) | 300ms |
| Changement de mode | Bleu (full power) | 300ms |

Les autres LEDs restent allumées pendant le feedback (compensation de luminosité).

### Transitions

| Type | Durée | Easing |
|---|---|---|
| Changement de mode (couleur) | 5s | Smootherstep quintic, 60 FPS |
| Changement d'intensité | 2s | Smootherstep quintic |
| Démarrage (ON) | 2s | Fade-in luminosité 0 → niveau actuel |
| RGB : hold entre couleurs | 10s | — |

**Smootherstep quintic** : `t³(t(t·6−15)+10)` — vitesse ET accélération nulles aux deux extrémités → transition perçue comme fluide.

Le blend couleur est fait en **float** (précision totale) pour éviter le jitter à basse luminosité.

### Anti-bounce

Fenêtre d'ignore de 80ms après chaque relâchement du bouton (le bounce mécanique fait 5-20ms).

## Build & Flash

```bash
# Prérequis: PlatformIO dans un venv
python3 -m venv ~/.platformio-venv
~/.platformio-venv/bin/pip install platformio

# Build
~/.platformio-venv/bin/pio run

# Flash (port /dev/ttyUSB0)
~/.platformio-venv/bin/pio run -t upload

# Monitor serial (115200 baud)
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

### Config PlatformIO (`platformio.ini`)

- Board: `d1_mini`
- Framework: `arduino`
- Lib: `fastled/FastLED@^3.10.1`
- Port: `/dev/ttyUSB0`

> **Note** : FastLED 3.6.0 crash sur ce board. Toujours utiliser ≥3.10.1.

## Budget puissance

| Config | Consommation |
|---|---|
| 11 LEDs full white + ESP | ~810mA @ 5V |
| 11 LEDs au max (255/255) | ~810mA @ 5V |
| Chargeur 5V/1A | ~14 LEDs max (full) |
| Chargeur 5V/2A | ~30 LEDs max (full) |

## Structure

```
lampe-led/
├── platformio.ini
├── src/
│   └── main.cpp      # Firmware complet
├── include/
├── lib/
└── test/
```
