# ESP32-S3 RFID & Fingerprint Student Attendance System

An IoT-based student attendance system using **ESP32-S3**, **RFID RC522**, and the **AS608 fingerprint sensor**. The system provides two-factor authentication, a local web management interface, cloud data storage through Google Sheets, SMS notifications, LCD status display, and Vietnamese voice feedback.

## Key Features

- Two-factor attendance verification using RFID card and fingerprint
- Student registration through a local Web Server
- Stores RFID UID and fingerprint ID in ESP32-S3 flash memory
- Uploads attendance records to Google Sheets through HTTPS
- Sends SMS notifications using a 4G LTE module
- Displays student information and system status on an LCD
- Provides Vietnamese voice notifications through an I2S audio amplifier
- Supports custom schematic and PCB design

## System Workflow

### Student Registration

1. The administrator opens the ESP32-S3 Web Server.
2. Student name and ID are entered through the registration form.
3. The student scans a new RFID card.
4. The student registers a fingerprint using the AS608 sensor.
5. RFID UID, fingerprint ID, and student information are linked and stored.
6. The LCD, web interface, and speaker confirm successful registration.

### Attendance Verification

1. The student scans an RFID card.
2. The system checks whether the card UID is registered.
3. The student places a finger on the AS608 sensor.
4. The system verifies that the fingerprint matches the registered RFID card.
5. After successful verification:
   - Student name, ID, and timestamp are uploaded to Google Sheets.
   - An SMS notification is sent through the 4G module.
   - The LCD displays the attendance result.
   - The speaker plays a Vietnamese confirmation message.

## Hardware

- ESP32-S3 development board
- MFRC522 RFID reader
- AS608 fingerprint sensor
- A7670C 4G LTE module
- LCD 20x4 with I2C interface
- MAX98357A I2S audio amplifier
- Speaker
- LM2596 buck converter
- Custom PCB and power supply circuit

## Communication Interfaces

| Module | Interface | ESP32-S3 GPIO |
|---|---|---|
| RFID RC522 | SPI | SCK 12, MISO 13, MOSI 11, SS 10 |
| AS608 fingerprint sensor | UART | GPIO 4 and GPIO 5 |
| LCD 20x4 | I2C | SDA 2, SCL 1 |
| MAX98357A audio module | I2S | BCLK 39, LRC 38, DIN 40 |
| A7670C 4G module | UART | GPIO 15 and GPIO 16 |

> Verify the pin configuration in the firmware before connecting the hardware.

## Software and Technologies

- C/C++
- Arduino IDE
- ESP32-S3 Arduino Framework
- HTML, CSS, and JavaScript
- ESPAsyncWebServer
- Google Apps Script
- Google Sheets API
- HTTPS and HTTP GET/POST
- AT commands for the 4G LTE module
- SPI, UART, I2C, and I2S

## Experimental Results

The prototype was evaluated using student registration, two-factor authentication, cloud synchronization, SMS notification, and voice-feedback tests.

- More than **97% fingerprint recognition rate** under normal conditions
- Google Sheets synchronization delay: approximately **1–2 seconds**
- SMS notification delay: approximately **3–5 seconds**
- Stable operation during simultaneous Wi-Fi, cloud, audio, and mobile communication tasks
## Setup

1. Install the ESP32 board package in Arduino IDE.
2. Install the required RFID, fingerprint, LCD, asynchronous web server, and audio libraries.
3. Open `StudentAttendanceSystem.ino`.
4. Configure:
   - Wi-Fi SSID and password
   - Google Apps Script URL
   - SMS recipient information
   - GPIO assignments
5. Connect the hardware according to the schematic.
6. Select the correct ESP32-S3 board and COM port.
7. Compile and upload the firmware.
8. Open Serial Monitor to obtain the Web Server IP address.
9. Access the IP address from a phone or computer connected to the same network.

## Security Notice

Before publishing the source code, remove or replace:

- Wi-Fi credentials
- Google Apps Script deployment URL
- API keys and tokens
- Phone numbers
- Real student names, IDs, RFID UIDs, and fingerprint information

Use a separate configuration file or placeholder values for public repositories.

## Limitations

- Cloud synchronization depends on a stable Internet connection.
- SMS delivery depends on mobile network quality.
- The AS608 sensor has limited fingerprint storage capacity.
- The system currently uses RFID and fingerprint verification only.

## Future Improvements

- Add offline attendance storage and automatic synchronization
- Develop a centralized database and administrator dashboard
- Encrypt locally stored student data
- Add role-based web authentication
- Improve PCB size, enclosure design, and power protection
- Support multiple classrooms and attendance devices
- Add over-the-air firmware updates
<img width="846" height="901" alt="image" src="https://github.com/user-attachments/assets/e83a2eab-a1c2-4c67-bac8-29221fc60cb9" />
<img width="795" height="909" alt="image" src="https://github.com/user-attachments/assets/8a06fa41-135a-474e-ba62-6daac8e0becd" />
<img width="759" height="788" alt="image" src="https://github.com/user-attachments/assets/617f75f2-00b5-4c6d-ba7f-6e9433bfb91a" />
