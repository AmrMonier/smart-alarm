# GEMINI.md

## Project Overview

This project is an ESP32-based clock that utilizes a DS1307 Real-Time Clock (RTC) module. It is built using the ESP-IDF framework and PlatformIO.

*   **Purpose:** A functional digital clock with the ability to maintain time even when powered off, thanks to the DS1307 RTC with a backup battery.
*   **Main Technologies:** C, ESP-IDF, PlatformIO
*   **Hardware:**
    *   ESP32 development board (`esp32dev`)
    *   DS1307 RTC module connected via I2C (SDA: GPIO21, SCL: GPIO22)
*   **Core Functionality:**
    *   Initializes the DS1307 RTC.
    *   If the RTC is not running, it sets the time to a placeholder value (the current compile time).
    *   Periodically reads the time from the RTC and logs it to the serial monitor.

## Building and Running

This project is managed by PlatformIO. Here are the common commands:

*   **Build:**
    ```bash
    ~/.platformio/penv/bin/pio run
    ```
*   **Upload to device:**
    ```bash
    ~/.platformio/penv/bin/pio run --target upload
    ```
*   **Monitor serial output:**
    ```bash
    ~/.platformio/penv/bin/pio device monitor
    ```
*   **Clean build files:**
    ```bash
    ~/.platformio/penv/bin/pio run --target clean
    ```

## Development Conventions

*   **Project Structure:** The project follows the standard PlatformIO directory structure.
    *   `src/`: Main application code.
    *   `include/`: Project header files.
    *   `lib/`: Project-specific (private) libraries.
        *   `ds1307_driver/`: A custom driver for the DS1307 RTC.
    *   `test/`: Unit tests.
*   **Dependencies:** Libraries are managed by the PlatformIO Library Manager.
*   **Configuration:** The project is configured via `platformio.ini`.

## DS1307 Driver

A custom driver for the DS1307 is located in `lib/ds1307_driver`. It provides the following functions:

*   `ds1307_init()`: Initializes the I2C communication.
*   `ds1307_set_time()`: Sets the time on the RTC.
*   `ds1307_get_time()`: Reads the time from the RTC.
*   `ds1307_is_running()`: Checks if the RTC oscillator is running.
*   `ds1307_reset()`: Resets the RTC to its initial state, as if the battery was removed. This is useful for testing the time initialization logic. To use it, uncomment the call to this function in `src/main.c`.
