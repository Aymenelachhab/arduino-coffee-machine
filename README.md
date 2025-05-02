# ☕ Arduino Coffee / Milk / Tea Dispenser

[![Build](https://img.shields.io/github/actions/workflow/status/Aymenelachhab/arduino-coffee-machine/ci.yml?branch=main)](https://github.com/Aymenelachhab/arduino-coffee-machine/actions)

Secure drink machine that **authenticates users via RFID card _or_ keypad code**, then
lets them pour **coffee, milk, tea** or custom mixes, plus selectable sugar dose.
Inspired by our embedded‑electronics semester project. :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3}

<img src="docs/lcd_menu.gif" width="500" alt="LCD menu demo">

## Hardware overview

| Part | Qty | Note |
|------|-----|------|
| Arduino Uno (or Nano) | 1 | 16 MHz, 5 V |
| MFRC522 RFID reader | 1 | SPI |
| 4×4 membrane keypad | 1 | Digital |
| I2C 16×2 LCD | 1 | For menu UI |
| HC‑SR04 ultrasonic sensor | 1 | Cup detection |
| SG90 servo | 1 | Sugar gate |
| 12 V peristaltic pump | 3 | Coffee / Milk / Tea |
| 2‑channel relay module | 2 | Pumps & heater control |
| 12 V PSU | 1 | Shared for pumps |

*Full BOM in* **`hardware/bom.csv`**.

## Wiring diagram

<p align="center">
  <img src="docs/wiring_diagram.png" width="600">
</p>

## Firmware features

* Double‑factor access (`RFID_UID` list **or** keypad PIN).
* LCD menu → drink selection → sugar grams (servo angle).
* Glass presence check with HC‑SR04; abort if no cup.
* Pumps run via relay for calibrated duration (per 100 mL).
* Buzzer feedback & timeout auto‑logout.

## Getting started

```bash
# 1. Install libraries (Arduino IDE → Library Manager)
#    - MFRC522 by GithubCommunity
#    - Keypad by Mark Stanley
#    - NewLiquidCrystal

# 2. Open firmware/coffee_machine.ino
# 3. Upload to Arduino @ 16 MHz / 5V
