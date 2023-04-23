# Readme - Leaf_Logger

This code is an Arduino program designed to log GPS and OBD data from a vehicle. It uses an ESP32 microcontroller, a GPS module, and an OBD2 scanner to collect the information, and stores it on an SD card in CSV format. The collected data includes GPS coordinates, altitude, speed, and various vehicle parameters.

## Hardware

### Components
- ESP32 Microcontroller
- GPS Module (TX and RX pins)
- OBD2 Scanner (Bluetooth ELM327)
- SD Card Module
- LED (built-in)

### Pinout
- GPS_RX_PIN: 4
- GPS_TX_PIN: 2
- LED_BUILTIN: 14

## Code Description

The code initializes the required libraries and sets up the necessary variables to store the GPS and OBD data. It uses BluetoothSerial to communicate with the OBD2 scanner and HardwareSerial for the GPS module. The CSV file is created on the SD card to store the data. In the main loop, GPS data is read and stored into variables, and OBD data is queried and saved.

## Adjustments

### Logging Intervals

The `T` constant is used as a delay between the OBD readings. It is currently set to 50ms. You can adjust this value to modify the interval between OBD queries. Increasing the value will reduce the frequency of OBD data collection, while decreasing it will increase the frequency.

### OBD Parameters

The code currently reads the following OBD parameters:
- Ambient temperature
- Speed
- Odometer
- Range
- State of Health (SOH)
- State of Charge (SOC)
- HX
- Ahr
- HV Battery Current 1
- HV Battery Current 2
- HV Battery Voltage

You can add or remove OBD parameters by modifying the `loop()` function and the associated OBD functions.

### GPS Data

The code collects the following GPS data:
- Time
- Latitude
- Longitude
- Altitude
- Speed
- Satellites

If you want to collect additional GPS data, you can add new variables and read the relevant data from the `gps` object of the TinyGPS++ library.

### CSV Columns

The CSV file columns are defined in the `CsvCol` constant. If you modify the logged parameters, make sure to update this constant accordingly, so that the CSV file has the correct headers.

## Usage

1. Connect the hardware components according to the pinout and component list.
2. Upload the code to the ESP32.
3. The program will start logging GPS and OBD data automatically.
4. The LED will blink to indicate the status of the program (e.g., error, acquiring GPS data).
5. Retrieve the SD card and access the CSV file to view the logged data.
