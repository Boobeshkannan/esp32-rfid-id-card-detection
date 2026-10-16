# ESP32 RFID-Based ID Card Detection

An ESP32-based Smart Gate prototype that uses an MFRC522 (RC522) RFID reader and two IR sensors to detect card IDs, identify movement direction, and maintain student entry and exit counts.

## Project Overview

This project demonstrates a basic student entry/exit tracking system using RFID and infrared sensors. After scanning an RFID card, the person must cross two IR sensors within a specified time. The order in which the sensors are triggered determines the movement direction.

The system displays the card UID, detected direction, timestamp, and entry/exit counts through the Serial Monitor.

## Features

- RFID card UID detection using the MFRC522 (RC522) reader
- ESP32-based sensor interfacing
- Direction detection using two IR sensors
- Entry and exit counting based on sensor sequence
- Basic sensor debounce logic
- Timeout handling for card scans and sensor crossings
- Serial Monitor output for student records and crossing events
- Optional Wi-Fi and NTP time synchronization
- Uptime-based timestamp fallback when network time is unavailable

## Hardware Components

- ESP32 development board
- MFRC522 (RC522) RFID reader
- Compatible RFID cards or tags
- Two IR obstacle/beam sensors
- Jumper wires
- Breadboard or suitable mounting arrangement
- USB power supply for the ESP32

## Pin Connections

### RC522 to ESP32

| RC522 Pin | ESP32 Pin |
|---|---|
| SDA / SS | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| 3.3V | 3.3V |
| GND | GND |

### IR Sensors to ESP32

| IR Sensor | ESP32 Pin |
|---|---|
| IR Sensor 1 OUT | GPIO 32 |
| IR Sensor 2 OUT | GPIO 33 |
| VCC and GND | According to the sensor module specifications |

**Note:** The pin assignments above are based on the current source code. Verify the physical wiring before powering the circuit. The RC522 should use a compatible 3.3V supply and logic levels.

## Working Principle

1. The RC522 detects an RFID card and reads its UID.
2. The UID is stored temporarily as the pending card ID.
3. The person must cross the two IR sensors within 5 seconds of scanning the card.
4. The system detects the order in which the sensors become blocked.
5. If IR Sensor 1 is triggered before IR Sensor 2, the system registers an EXIT.
6. If IR Sensor 2 is triggered before IR Sensor 1, the system registers an ENTRY.
7. The system updates the corresponding count and displays the UID, direction, timestamp, and counts in the Serial Monitor.

## Direction Detection

| Sensor Sequence | Detected Direction |
|---|---|
| IR1 → IR2 | EXIT |
| IR2 → IR1 | ENTRY |

The direction labels depend on the physical arrangement of the sensors and the walking direction.

## Software Requirements

- Arduino IDE
- ESP32 board support package
- MFRC522 library compatible with the Arduino environment
- SPI library
- Wi-Fi and time libraries provided by the ESP32 Arduino core

## Setup and Usage

1. Install the ESP32 board support package in Arduino IDE.
2. Install the MFRC522 library.
3. Connect the RC522 and both IR sensors according to the pin tables.
4. Open `rfid_based_idcard_detection.ino` in Arduino IDE.
5. Select the appropriate ESP32 board and COM port.
6. Optionally configure `WIFI_SSID` and `WIFI_PASSWORD` in the source code to enable network time synchronization.
7. Upload the sketch to the ESP32.
8. Open Serial Monitor at **115200 baud**.
9. Scan an RFID card and cross the two sensors within the configured time.
10. Observe the detected direction, timestamp, and entry/exit counts.

## Current Implementation and Limitations

- Student records are maintained in a RAM-based table with a maximum capacity of 100 records.
- Entry and exit counts are not permanently stored and will reset when the ESP32 restarts or loses power.
- The system associates a sensor crossing with the most recently pending RFID card; it is intended as a basic prototype, not a complete multi-person access-control system.
- Card UIDs are used as identifiers; an authorized student database and additional authentication are not implemented.
- Accurate clock timestamps require successful network time synchronization. Otherwise, the system displays an uptime-based timestamp.
- Sensor polarity and physical placement must match the configuration in the code.
- The system does not include a web dashboard, cloud database, or automatic attendance report.

## Future Improvements

- Add an authorized student database.
- Store attendance and movement records in non-volatile memory or a database.
- Improve handling of overlapping card scans and multiple people.
- Add an LCD or OLED display for local status information.
- Add a web dashboard for viewing attendance records.
- Improve access-control security and event validation.

## Project Status

Working prototype with RFID card reading, two-sensor direction detection, and Serial Monitor-based entry/exit tracking.

## Learning Outcomes

- ESP32 programming using Arduino IDE
- RFID interfacing with the MFRC522 module
- Digital sensor interfacing and event detection
- Basic direction-detection logic
- Timing, debounce, and timeout handling
- Serial debugging and embedded system integration

## Author

**Boobeshkannan**

GitHub: [Boobeshkannan](https://github.com/Boobeshkannan)
