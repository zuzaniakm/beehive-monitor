# Beehive Monitor

An ESP32-based IoT device for monitoring bee colony weight, in-hive temperature, and relative humidity with Bluetooth-based configuration. Data is logged locally to an SD card and uploaded wirelessly to a cloud database for remote viewing.

## Features

- **Weight monitoring**: four load cells (up to 200 kg combined) read via an HX711 amplifier/ADC
- **Temperature & humidity**: DHT22 sensor placed inside the hive
- **Wireless data upload**: sends readings to [ThingSpeak](https://thingspeak.com/) over WiFi
- **Local backup**: every reading is also appended to a CSV file on a microSD card
- **Email alerts**: automatic notifications when temperature, humidity, or weight moves outside normal bounds or changes abruptly
- **Bluetooth configuration**: set WiFi credentials, email recipient, scan interval, and scale calibration without reflashing the device
- **Low power**: the ESP32 spends most of its time in deep sleep, only waking to sample and transmit data

## Hardware

| Component | Part |
|---|---|
| Microcontroller | WeMos LOLIN32 (ESP32) |
| Temperature/humidity sensor | DHT22 (AM2302) |
| Load cells | SEN-10245 (x4, 50 kg each) |
| ADC/amplifier | HX711 |
| Storage | microSD card module (SPI) |
| Power | 5000 mAh Li-Po battery |

## Wiring

| Vývojová doska / ESP32 pin | Peripheral |
|---|---|
| GPIO 21 | HX711 SCK |
| GPIO 22 | HX711 DT |
| GPIO 33 | DHT22 DATA |
| GPIO 5 | SD card CS |
| GPIO 18 | SD card CLK |
| GPIO 19 | SD card MISO |
| GPIO 23 | SD card MOSI |
| GPIO 34 | Reset/config button |

The four load cells are wired together into a Wheatstone bridge configuration and connected to the HX711 module.

## Repository Structure

```
beehive_monitor.ino     # main sketch (setup/loop, deep sleep control)
BeehiveMonitor.h        # core class tying sensors, WiFi, DB, SD, and email together
Wifi.h                  # WiFi connection management
Database.h              # ThingSpeak upload wrapper
SDCardController.h      # SD card read/write helpers
EmailAlert.h            # SMTP alert emails with HTML formatting
Config.h                # pin assignments and thresholds 
```

## Configuration

You'll also need a ThingSpeak channel ID and write API key, passed into the `BeehiveMonitor` constructor in the main sketch.

## Dependencies (Arduino libraries)

- [DHT sensor library](https://github.com/adafruit/DHT-sensor-library) (Adafruit)
- [HX711](https://github.com/RobTillaart/HX711) (Rob Tillaart)
- [ThingSpeak Arduino library](https://github.com/mathworks/thingspeak-arduino)
- [EMailSender](https://github.com/xreef/EMailSender)

## First-time Setup

On first boot (or after a factory reset), the device enters a 2-minute Bluetooth configuration window. Connect using any serial Bluetooth terminal app and send the following commands:

| Command | Description |
|---|---|
| `wifi <ssid> <password>` | Set WiFi credentials |
| `email <address>` | Set alert recipient email |
| `id <number>` | Set hive ID (for multi-hive ThingSpeak channels) |
| `delay <minutes>` | Set measurement interval (minimum 15) |
| `tare` | Zero the scale |
| `calibrate <grams>` | Calibrate using a known reference weight |
| `offset <value>` | Manually set scale offset |
| `scale <factor>` | Manually set scale calibration factor |
| `clear` | Wipe all stored settings |
| `ready` | Finish setup and start normal operation |

Once WiFi and email are set, sending `ready` exits configuration mode and the device begins its normal sense → upload → save → sleep cycle.

## How It Works

1. On wake, the device loads saved settings from flash (WiFi credentials, calibration, last readings).
2. It reads temperature, humidity, and weight.
3. Readings are compared against configured thresholds and against the previous reading; out-of-range or sharply changed values trigger an email alert.
4. Data is uploaded to ThingSpeak (if WiFi is available) and appended to `data.csv` on the SD card with a timestamp synced via NTP.
5. WiFi is turned off and the device goes into deep sleep until the next scheduled reading.
