# ESP32 TFT OBD-II Multi-Gauge Display v1.5

![Project Image](https://raw.githubusercontent.com/Olme-xH/OBDII-v1.5/main/doc/gauge_photo.jpg)
*Note: You would need to replace the image link above with an actual image of your project.*

This repository contains the source code for an ESP32-based OBD-II multi-gauge display that uses a TFT screen to show real-time vehicle data. The project is built using the `TFT_eSPI` library and communicates with an ELM327 Bluetooth adapter.

## Project Versions

This repository contains several versions of the project, each located in its own directory. Please choose the one that best fits your needs:

* **[OBDII_Testing_Final](./OBDII_Testing_Final/)**: **Recommended for most users.** This is the final, stable version intended for in-car use. It is cleaned of all simulation and debugging code.
* **[OBDII_Testing](./OBDII_Testing/)**: The development version. This includes a special feature that allows you to start the device in **Simulation Mode** by holding down a button on startup, making it easy to test and develop without being connected to a car.
* **[OBDII_v1.5](./OBDII_v1.5/)**: The original stable release of the project. This version uses a compile-time flag to switch between simulation and car modes.

## Features

* **Multi-Mode Display:** Cycle through different screen layouts to view various data points.
* **Real-Time Data:** Displays RPM, MPH, MPG, Average MPG, Engine Load, Fuel Level, and more.
* **Automatic Dimming:** Uses a photoresistor (LDR) to automatically adjust backlight brightness for day and night driving.
* **Dual MPG Averages:** Calculates two separate MPG averages with different capping logic based on trip duration.
* **Runtime Simulation Mode (in Testing version):** Easily switch to a mode that generates fake data for testing purposes.

## Hardware Required

* ESP32 Development Board
* ILI9488 TFT Display (or other `TFT_eSPI` compatible display)
* ELM327 Bluetooth OBD-II Adapter
* PNP Transistor (e.g., BC327, 2N3906) for backlight control
* Resistors (1kΩ, 100Ω, 1Ω)
* Photoresistor (LDR)
* Momentary push button

## License

This project is licensed under the Apache-2.0 License. See the [LICENSE](./LICENSE) file for details.