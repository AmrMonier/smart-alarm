#include "buzzer_driver.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static gpio_num_t buzzer_gpio_pin;
static TaskHandle_t buzzer_task_handle = NULL;
static QueueHandle_t buzzer_queue = NULL;

// The command sent to the buzzer task.
typedef struct {
    buzzer_note_t note;
    bool stop; // A flag to signal stop, instead of a separate command type
} buzzer_cmd_t;

static volatile bool is_playing = false;
static buzzer_note_t s_default_note;

static void buzzer_task(void *pvParameters) {
    buzzer_cmd_t cmd;

    while (1) {
        // Wait for a command
        if (xQueueReceive(buzzer_queue, &cmd, portMAX_DELAY)) {
            if (cmd.stop) {
                is_playing = false;
                gpio_set_level(buzzer_gpio_pin, 0);
                continue;
            }

            is_playing = true;
            for (int i = 0; i < cmd.note.repeat_count; i++) {
                // Check for a stop command before starting the next beep
                if (uxQueueMessagesWaiting(buzzer_queue) > 0) {
                    if (xQueueReceive(buzzer_queue, &cmd, 0) && cmd.stop) {
                        goto stop_playing_note;
                    }
                }

                // Play duration
                if (cmd.note.play_duration_ms > 0) {
                    gpio_set_level(buzzer_gpio_pin, 1);
                    vTaskDelay(cmd.note.play_duration_ms / portTICK_PERIOD_MS);
                }

                // Pause duration
                if (cmd.note.pause_duration_ms > 0) {
                    gpio_set_level(buzzer_gpio_pin, 0);
                    vTaskDelay(cmd.note.pause_duration_ms / portTICK_PERIOD_MS);
                }
            }
        stop_playing_note:
            is_playing = false;
            gpio_set_level(buzzer_gpio_pin, 0); // Ensure buzzer is off
        }
    }
}

void buzzer_init(gpio_num_t gpio_pin, const buzzer_note_t* default_note) {
    buzzer_gpio_pin = gpio_pin;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << buzzer_gpio_pin),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);

    if (default_note != NULL) {
        s_default_note = *default_note;
    } else {
        // Hardcoded default values
        s_default_note.play_duration_ms = 100;
        s_default_note.pause_duration_ms = 50;
        s_default_note.repeat_count = 30;
    }

    buzzer_queue = xQueueCreate(1, sizeof(buzzer_cmd_t));
    xTaskCreate(buzzer_task, "buzzer_task", 2048, NULL, 5, &buzzer_task_handle);
}

void buzzer_play_note(buzzer_note_t note) {
    if (buzzer_task_handle == NULL) {
        return;
    }
    // Stop any currently playing note before starting a new one.
    buzzer_stop();
    buzzer_cmd_t cmd = {
        .note = note,
        .stop = false
    };
    xQueueSend(buzzer_queue, &cmd, 0);
}

void buzzer_play_default_note(void) {
    buzzer_play_note(s_default_note);
}

void buzzer_stop(void) {
    if (buzzer_task_handle == NULL || !is_playing) {
        return;
    }
    buzzer_cmd_t cmd = { .stop = true };
    xQueueSendToFront(buzzer_queue, &cmd, 0);
}

bool buzzer_is_playing(void) {
    return is_playing;
}
