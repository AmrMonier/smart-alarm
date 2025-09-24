#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include "driver/gpio.h"
#include "esp_err.h"

/**
 * @brief Opaque handle for a rotary encoder instance.
 */
typedef struct rotary_encoder_t* rotary_encoder_handle_t;

/**
 * @brief Enumeration of rotary encoder events.
 */
typedef enum {
    ROTARY_ENCODER_EVENT_COUNTER_CLOCKWISE,
    ROTARY_ENCODER_EVENT_CLOCKWISE,
} rotary_encoder_event_t;

/**
 * @brief Configuration structure for a rotary encoder.
 */
typedef struct {
    gpio_num_t clk_pin;
    gpio_num_t dt_pin;
    uint32_t queue_size;
} rotary_encoder_config_t;

/**
 * @brief Callback function type for rotary encoder events.
 *
 * @param handle The handle of the rotary encoder that generated the event.
 * @param event The type of event that occurred.
 * @param user_data User-provided data associated with the encoder.
 */
typedef void (*rotary_encoder_callback_t)(rotary_encoder_handle_t handle, const rotary_encoder_event_t* event, void* user_data);

/**
 * @brief Creates a new rotary encoder instance.
 *
 * @param config Pointer to the rotary encoder configuration structure.
 * @return A handle to the created encoder instance, or NULL if creation fails.
 */
rotary_encoder_handle_t rotary_encoder_create(const rotary_encoder_config_t *config);

/**
 * @brief Registers a callback function for rotary encoder events.
 *
 * @param handle The handle of the rotary encoder.
 * @param callback The callback function to register.
 * @param user_data User data to be passed to the callback.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t rotary_encoder_register_callback(rotary_encoder_handle_t handle, rotary_encoder_callback_t callback, void* user_data);

/**
 * @brief Deletes a rotary encoder instance and frees its resources.
 *
 * @param handle The handle of the encoder to delete.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t rotary_encoder_delete(rotary_encoder_handle_t handle);

#endif // ROTARY_ENCODER_H
