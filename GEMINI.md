# GEMINI.md

## Project Overview

This project is an ESP32-based clock that utilizes a DS1307 Real-Time Clock (RTC) module. It is built using the ESP-IDF framework and PlatformIO.

*   **Purpose:** A smart alarm clock with advanced features, built on an ESP32 and utilizing a DS1307 Real-Time Clock (RTC) module for timekeeping.
*   **Main Technologies:** C, ESP-IDF, PlatformIO
*   **Hardware:**
    *   ESP32 development board (`esp32dev`)
    *   DS1307 RTC module connected via I2C (SDA: GPIO21, SCL: GPIO22)
*   **Core Functionality:**
    *   Displays current date and time.
    *   Allows setting up to 5 distinct alarms.
    *   Each alarm can store a specific time and days of the week for recurring triggers.
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
        *   `button_reader/`: A custom driver for push buttons.
        *   `buzzer_driver/`: A custom driver for an active buzzer.
        *   `ds1307_driver/`: A custom driver for the DS1307 RTC.
        *   `lcd_i2c_driver/`: A custom driver for LCD I2C displays.
        *   `rotary_encoder_driver/`: A custom driver for rotary encoders.
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

## Buzzer Driver

A custom driver for an active buzzer is located in `lib/buzzer_driver`. It provides a non-blocking, task-based interface to play sequences of beeps.

*   `buzzer_init(gpio_pin, default_note)`: Initializes the buzzer driver on a specific GPIO pin. It can be configured with a default note, or `NULL` can be passed to use a hardcoded default.
*   `buzzer_play_note(note)`: Plays a note defined by a `buzzer_note_t` struct, which includes play duration, pause duration, and repeat count.
*   `buzzer_play_default_note()`: Plays the default note that was configured during initialization.
*   `buzzer_stop()`: Immediately stops any currently playing note.
*   `buzzer_is_playing()`: Returns `true` if the buzzer is currently active.
