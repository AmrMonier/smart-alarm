#include "rotary_encoder.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

static const char *TAG = "ROTARY_ENCODER";

// State machine table for decoding
const int8_t KNOB_STATES[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

/**
 * @brief Internal structure for a rotary encoder instance.
 */
typedef struct rotary_encoder_t {
    rotary_encoder_config_t config;
    QueueHandle_t event_queue;
    rotary_encoder_callback_t callback;
    void* user_data;
    uint8_t last_state;
} rotary_encoder_t;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
    rotary_encoder_handle_t handle = (rotary_encoder_handle_t)arg;

    uint8_t new_clk = gpio_get_level(handle->config.clk_pin);
    uint8_t new_dt = gpio_get_level(handle->config.dt_pin);
    uint8_t new_state = (new_clk << 1) | new_dt;

    handle->last_state = (handle->last_state << 2) & 0x0F;
    handle->last_state |= new_state;

    int8_t state = KNOB_STATES[handle->last_state];

    if (state != 0) {
        rotary_encoder_event_t event = (state == 1) ? ROTARY_ENCODER_EVENT_CLOCKWISE : ROTARY_ENCODER_EVENT_COUNTER_CLOCKWISE;
        xQueueSendFromISR(handle->event_queue, &event, NULL);
    }
}

static void encoder_task(void* arg) {
    rotary_encoder_handle_t handle = (rotary_encoder_handle_t)arg;
    rotary_encoder_event_t event;

    while (1) {
        if (xQueueReceive(handle->event_queue, &event, portMAX_DELAY)) {
            if (handle->callback) {
                handle->callback(handle, &event, handle->user_data);
            }
        }
    }
}

rotary_encoder_handle_t rotary_encoder_create(const rotary_encoder_config_t *config) {
    rotary_encoder_handle_t handle = calloc(1, sizeof(rotary_encoder_t));
    if (!handle) {
        ESP_LOGE(TAG, "Failed to allocate memory for handle");
        return NULL;
    }

    handle->config = *config;

    handle->event_queue = xQueueCreate(config->queue_size, sizeof(rotary_encoder_event_t));
    if (!handle->event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        free(handle);
        return NULL;
    }

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << config->clk_pin) | (1ULL << config->dt_pin);
    io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

    // Install ISR service if not already installed
    gpio_install_isr_service(0);

    gpio_isr_handler_add(config->clk_pin, gpio_isr_handler, handle);
    gpio_isr_handler_add(config->dt_pin, gpio_isr_handler, handle);

    xTaskCreate(encoder_task, "encoder_task", 2048, handle, 5, NULL);

    ESP_LOGI(TAG, "Rotary encoder created for CLK:%d, DT:%d", config->clk_pin, config->dt_pin);
    return handle;
}

esp_err_t rotary_encoder_register_callback(rotary_encoder_handle_t handle, rotary_encoder_callback_t callback, void* user_data) {
    if (!handle) return ESP_ERR_INVALID_ARG;
    handle->callback = callback;
    handle->user_data = user_data;
    return ESP_OK;
}

esp_err_t rotary_encoder_delete(rotary_encoder_handle_t handle) {
    if (!handle) return ESP_ERR_INVALID_ARG;
    // TODO: Implement deletion logic (remove ISRs, delete task, delete queue, free handle)
    return ESP_OK;
}
