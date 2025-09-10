# GEMINI.md

## Project Overview

This is a simple ESP32 project using the ESP-IDF framework and PlatformIO. The main application logic is in `src/main.c`, which currently just logs a "Hello Monier" message.

* **Purpose:** A starting point for an ESP32-based clock device.
* **Main Technologies:** C, ESP-IDF, PlatformIO
* **Hardware:** ESP32 development board (`esp32dev`)
* **Architecture:** Standard PlatformIO project structure with `src`, `lib`, `include`, and `test` directories.

## Building and Running

This project is managed by PlatformIO. Here are the common commands:

* **Build:**

    ```bash
    platformio run
    ```

* **Upload to device:**

    ```bash
    platformio run --target upload
    ```

* **Monitor serial output:**

    ```bash
    platformio device monitor
    ```

* **Clean build files:**

    ```bash
    platformio run --target clean
    ```

## Development Conventions

* **Project Structure:** The project follows the standard PlatformIO directory structure.
  * `src/`: Main application code
  * `include/`: Project header files
  * `lib/`: Project-specific (private) libraries
  * `test/`: Unit tests
* **Dependencies:** Libraries are managed by the PlatformIO Library Manager. Add dependencies to `platformio.ini`.
* **Configuration:** The project is configured via the `platformio.ini` file.
