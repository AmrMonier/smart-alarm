#ifndef BUZZER_DRIVER_H
#define BUZZER_DRIVER_H

#include "driver/gpio.h"
#include <stdbool.h>
#include <stddef.h>

// Represents a musical note with play and pause durations, and a repeat count.
typedef struct {
    uint32_t play_duration_ms;
    uint32_t pause_duration_ms;
    int repeat_count;
} buzzer_note_t;

/**
 * @brief Initializes the buzzer driver.
 *
 * @param gpio_pin The GPIO pin connected to the buzzer.
 * @param default_note Optional: A pointer to a note to set as the default. If NULL, a hardcoded default is used.
 */
void buzzer_init(gpio_num_t gpio_pin, const buzzer_note_t* default_note);

/**
 * @brief Plays an arbitrary note.
 *
 * @param note The note to play.
 */
void buzzer_play_note(buzzer_note_t note);

/**
 * @brief Plays the default note configured during initialization.
 */
void buzzer_play_default_note(void);

/**
 * @brief Stops any currently playing note or pattern.
 */
void buzzer_stop(void);

/**
 * @brief Checks if the buzzer is currently playing.
 *
 * @return True if the buzzer is active, false otherwise.
 */
bool buzzer_is_playing(void);

#endif // BUZZER_DRIVER_H
