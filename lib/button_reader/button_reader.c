#include "button_reader.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "BUTTON_READER";

// Debounce configuration
#define DEBOUNCE_TICKS pdMS_TO_TICKS(50)

// Main polling task configuration
#define POLLING_TASK_STACK_SIZE 2048
#define POLLING_TASK_PRIORITY 5
#define POLLING_INTERVAL_MS 20

/**
 * @brief Internal structure for a button instance.
 */
struct button_t {
    button_config_t config;
    button_event_cb_t callback;
    bool current_state;
    bool last_state;
    TimerHandle_t long_press_timer;
    struct button_t* next;
};

// --- Private Module State ---
static button_handle_t button_list_head = NULL;
static TaskHandle_t polling_task_handle = NULL;

// --- Forward Declarations ---
static void polling_task(void* arg);
static void long_press_timer_callback(TimerHandle_t xTimer);

// --- Public API Implementation ---

button_handle_t button_create(const button_config_t* config) {
    if (config == NULL) {
        return NULL;
    }

    // 1. Allocate memory for the new button
    button_handle_t new_button = (button_handle_t)malloc(sizeof(struct button_t));
    if (new_button == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for new button");
        return NULL;
    }
    memset(new_button, 0, sizeof(struct button_t));
    new_button->config = *config;

    // 2. Configure GPIO pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << config->gpio_num),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = config->active_level == 0 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = config->active_level == 0 ? GPIO_PULLDOWN_DISABLE : GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    // 3. Initialize button state
    new_button->last_state = gpio_get_level(config->gpio_num) == config->active_level;
    new_button->current_state = new_button->last_state;

    // 4. Create long-press timer if needed
    if (config->long_press_ms > 0) {
        new_button->long_press_timer = xTimerCreate(
            "long_press_tmr",
            pdMS_TO_TICKS(config->long_press_ms),
            pdFALSE, // One-shot timer
            (void*)new_button,
            long_press_timer_callback
        );
        if (new_button->long_press_timer == NULL) {
            ESP_LOGE(TAG, "Failed to create long press timer for GPIO %d", config->gpio_num);
            free(new_button);
            return NULL;
        }
    }

    // 5. Add to the linked list
    new_button->next = button_list_head;
    button_list_head = new_button;

    // 6. Start the polling task if it's not already running
    if (polling_task_handle == NULL) {
        xTaskCreate(polling_task, "button_poll_task", POLLING_TASK_STACK_SIZE, NULL, POLLING_TASK_PRIORITY, &polling_task_handle);
    }

    ESP_LOGI(TAG, "Button created for GPIO %d", config->gpio_num);
    return new_button;
}

esp_err_t button_delete(button_handle_t handle) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Find and remove the button from the list
    button_handle_t* current = &button_list_head;
    while (*current != NULL && *current != handle) {
        current = &(*current)->next;
    }

    if (*current == handle) {
        *current = handle->next; // Unlink
        if (handle->long_press_timer) {
            xTimerDelete(handle->long_press_timer, portMAX_DELAY);
        }
        free(handle);
        ESP_LOGI(TAG, "Button deleted.");

        // If the list is now empty, stop the polling task
        if (button_list_head == NULL && polling_task_handle != NULL) {
            vTaskDelete(polling_task_handle);
            polling_task_handle = NULL;
        }
        return ESP_OK;
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t button_register_callback(button_handle_t handle, button_event_cb_t cb) {
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    handle->callback = cb;
    return ESP_OK;
}

// --- Private Functions ---

static void long_press_timer_callback(TimerHandle_t xTimer) {
    button_handle_t button = (button_handle_t)pvTimerGetTimerID(xTimer);
    if (button && button->callback && button->current_state) {
        button->callback(button, BUTTON_EVENT_LONG_PRESS, button->config.user_data);
    }
}

static void polling_task(void* arg) {
    TickType_t last_poll_time = xTaskGetTickCount();

    while (1) {
        for (button_handle_t b = button_list_head; b != NULL; b = b->next) {
            b->last_state = b->current_state;
            b->current_state = gpio_get_level(b->config.gpio_num) == b->config.active_level;

            // --- State Change Detection ---
            if (b->current_state != b->last_state) {
                // State has changed, wait for debounce period
                vTaskDelay(DEBOUNCE_TICKS);
                // Read again after debounce
                b->current_state = gpio_get_level(b->config.gpio_num) == b->config.active_level;

                if (b->current_state != b->last_state) { // Debounced change
                    if (b->current_state) {
                        // --- Press Event ---
                        if (b->callback) {
                            b->callback(b, BUTTON_EVENT_PRESS, b->config.user_data);
                        }
                        // Start long press timer if applicable
                        if (b->long_press_timer) {
                            xTimerStart(b->long_press_timer, 0);
                        }
                    } else {
                        // --- Release Event ---
                        if (b->callback) {
                            b->callback(b, BUTTON_EVENT_RELEASE, b->config.user_data);
                        }
                        // Stop long press timer if it's running
                        if (b->long_press_timer) {
                            xTimerStop(b->long_press_timer, 0);
                        }
                    }
                }
            }
        }

        vTaskDelayUntil(&last_poll_time, pdMS_TO_TICKS(POLLING_INTERVAL_MS));
    }
}