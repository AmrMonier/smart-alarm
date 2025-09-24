#ifndef BUTTON_READER_H
#define BUTTON_READER_H

#include "esp_err.h"
#include "driver/gpio.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Opaque handle for a button instance.
 */
typedef struct button_t* button_handle_t;

/**
 * @brief Button event types.
 */
typedef enum {
    BUTTON_EVENT_PRESS,
    BUTTON_EVENT_RELEASE,
    BUTTON_EVENT_LONG_PRESS,
} button_event_t;

/**
 * @brief Callback function type for button events.
 *
 * @param handle The handle of the button that generated the event.
 * @param event The type of event that occurred.
 * @param user_data User data provided during registration.
 */
typedef void (*button_event_cb_t)(button_handle_t handle, button_event_t event, void* user_data);

/**
 * @brief Configuration for a button instance.
 */
typedef struct {
    gpio_num_t gpio_num;            /*!< GPIO pin number for the button. */
    bool active_level;              /*!< Logic level when the button is pressed (0 for active-low, 1 for active-high). */
    uint32_t long_press_ms;         /*!< Time in milliseconds to trigger a long-press event. Set to 0 to disable. */
    void* user_data;                /*!< User data to be passed to the callback function. */
} button_config_t;

/**
 * @brief Creates a new button instance.
 *
 * @param config Pointer to the button configuration.
 * @return A handle to the new button instance, or NULL on failure.
 */
button_handle_t button_create(const button_config_t* config);

/**
 * @brief Deletes a button instance and frees its resources.
 *
 * @param handle The handle of the button to delete.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t button_delete(button_handle_t handle);

/**
 * @brief Registers a callback function for button events.
 *
 * @param handle The handle of the button.
 * @param cb The callback function to register.
 * @return ESP_OK on success, or an error code on failure.
 */
esp_err_t button_register_callback(button_handle_t handle, button_event_cb_t cb);

#endif // BUTTON_READER_H