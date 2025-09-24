#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ds1307.h"
#include "lcd_i2c.h"
#include "button_reader.h"
#include "rotary_encoder.h"

static const char *TAG = "APP_MAIN";

// --- Hardware Configuration ---
#define I2C_MASTER_SCL_IO   22
#define I2C_MASTER_SDA_IO   21
#define I2C_MASTER_NUM      I2C_NUM_0

#define LCD_COLS 16
#define LCD_ROWS 4

#define BUTTON_A_GPIO GPIO_NUM_25
#define BUTTON_B_GPIO GPIO_NUM_26
#define BUTTON_C_GPIO GPIO_NUM_27

#define ROTARY_CLK_GPIO GPIO_NUM_19
#define ROTARY_DT_GPIO  GPIO_NUM_18
#define ROTARY_SW_GPIO  GPIO_NUM_23

// --- Button Context ---
typedef struct {
    char label;
} button_context_t;

// --- Global Handles & State ---
static SemaphoreHandle_t lcd_mutex;
static volatile int32_t encoder_count = 0;
static volatile char g_current_button_pressed = ' ';

// --- Tasks and Callbacks ---

void on_rotation_event(rotary_encoder_handle_t handle, const rotary_encoder_event_t* event, void* user_data) {
    if (*event == ROTARY_ENCODER_EVENT_CLOCKWISE) {
        encoder_count++;
    } else {
        encoder_count--;
    }
    ESP_LOGI(TAG, "Encoder count: %ld", encoder_count);
}

void on_sw_button_event(button_handle_t handle, button_event_t event, void* user_data) {
    if (event == BUTTON_EVENT_PRESS) {
        ESP_LOGI(TAG, "Rotary switch pressed, resetting count.");
        encoder_count = 0;
    }
}

void on_general_button_event(button_handle_t handle, button_event_t event, void* user_data) {
    char button_label = *(char*)user_data;
    if (event == BUTTON_EVENT_PRESS) {
        ESP_LOGI(TAG, "Button %c pressed.", button_label);
        g_current_button_pressed = button_label;
    } else if (event == BUTTON_EVENT_RELEASE) {
        ESP_LOGI(TAG, "Button %c released.", button_label);
        if (g_current_button_pressed == button_label) {
            g_current_button_pressed = ' '; // Clear if this was the last button pressed
        }
    }
}

void read_time_task(void *pvParameters) {
    char time_str[16];
    char date_str[16];
    char button_str[16];
    char encoder_str[16];

    static char prev_date_str[16] = {0};
    static char prev_button_str[16] = {0};
    static char prev_encoder_str[16] = {0};

    while (1) {
        if (xSemaphoreTake(lcd_mutex, portMAX_DELAY) == pdTRUE) {
            // Display Time on Line 1 (always update as seconds change frequently)
            rtc_time_t time;
            if (ds1307_get_time(&time) == ESP_OK) {
                sprintf(time_str, "%02d:%02d:%02d", time.hours, time.minutes, time.seconds);
                lcd_i2c_set_cursor(0, 0);
                lcd_i2c_send_string("                "); // Clear line
                lcd_i2c_set_cursor(0, 0);
                lcd_i2c_send_string(time_str);

                // Display Date on Line 2 (update only if changed)
                sprintf(date_str, "%02d/%02d/%04d", time.date, time.month, time.year + 2000);
                if (strcmp(date_str, prev_date_str) != 0) {
                    lcd_i2c_set_cursor(1, 0);
                    lcd_i2c_send_string("                "); // Clear line
                    lcd_i2c_set_cursor(1, 0);
                    lcd_i2c_send_string(date_str);
                    strcpy(prev_date_str, date_str);
                }
            }

            // Display Button Status on Line 3 (update only if changed)
            char current_button_display[16];
            if (g_current_button_pressed != ' ') {
                sprintf(current_button_display, "Button: %c", g_current_button_pressed);
            } else {
                sprintf(current_button_display, "Button: None");
            }
            if (strcmp(current_button_display, prev_button_str) != 0) {
                lcd_i2c_set_cursor(2, 0);
                lcd_i2c_send_string("                "); // Clear line
                lcd_i2c_set_cursor(2, 0);
                lcd_i2c_send_string(current_button_display);
                strcpy(prev_button_str, current_button_display);
            }

            // Display Encoder Count on Line 4 (update only if changed)
            sprintf(encoder_str, "Encoder: %ld", encoder_count);
            if (strcmp(encoder_str, prev_encoder_str) != 0) {
                lcd_i2c_set_cursor(3, 0);
                lcd_i2c_send_string("                "); // Clear line
                lcd_i2c_set_cursor(3, 0);
                lcd_i2c_send_string(encoder_str);
                strcpy(prev_encoder_str, encoder_str);
            }

            xSemaphoreGive(lcd_mutex);
        }
        vTaskDelay(500 / portTICK_PERIOD_MS); // Update display twice a second
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Initializing application...");

    lcd_mutex = xSemaphoreCreateMutex();

    // 1. Configure and initialize I2C bus for RTC
    ds1307_config_t ds1307_conf = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_pin = I2C_MASTER_SDA_IO,
        .scl_pin = I2C_MASTER_SCL_IO,
    };
    esp_err_t err = ds1307_init(&ds1307_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize DS1307: %s", esp_err_to_name(err));
        return;
    }

    // 2. Configure and initialize LCD
    lcd_i2c_config_t lcd_conf = {
        .i2c_port = I2C_MASTER_NUM,
        .i2c_address = LCD_I2C_DEFAULT_ADDRESS,
        .cols = LCD_COLS,
        .rows = LCD_ROWS,
    };
    err = lcd_i2c_init(&lcd_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize LCD: %s", esp_err_to_name(err));
        return;
    }

    // 3. Initialize Rotary Encoder
    rotary_encoder_config_t rotary_conf = {
        .clk_pin = ROTARY_CLK_GPIO,
        .dt_pin = ROTARY_DT_GPIO,
        .queue_size = 8,
    };
    rotary_encoder_handle_t rotary_handle = rotary_encoder_create(&rotary_conf);
    rotary_encoder_register_callback(rotary_handle, on_rotation_event, NULL);

    // 4. Initialize Rotary Encoder Switch Button
    button_config_t sw_btn_conf = {
        .gpio_num = ROTARY_SW_GPIO,
        .active_level = 0, // Assuming pull-up, active low
        .long_press_ms = 0,
        .user_data = NULL
    };
    button_handle_t sw_btn = button_create(&sw_btn_conf);
    button_register_callback(sw_btn, on_sw_button_event);

    // 5. Initialize Button A
    static char btn_a_label = 'A';
    button_config_t btn_a_conf = {
        .gpio_num = BUTTON_A_GPIO,
        .active_level = 0,
        .long_press_ms = 0,
        .user_data = &btn_a_label
    };
    button_handle_t btn_a = button_create(&btn_a_conf);
    button_register_callback(btn_a, on_general_button_event);

    // 6. Initialize Button B
    static char btn_b_label = 'B';
    button_config_t btn_b_conf = {
        .gpio_num = BUTTON_B_GPIO,
        .active_level = 0,
        .long_press_ms = 0,
        .user_data = &btn_b_label
    };
    button_handle_t btn_b = button_create(&btn_b_conf);
    button_register_callback(btn_b, on_general_button_event);

    // 7. Initialize Button C
    static char btn_c_label = 'C';
    button_config_t btn_c_conf = {
        .gpio_num = BUTTON_C_GPIO,
        .active_level = 0,
        .long_press_ms = 0,
        .user_data = &btn_c_label
    };
    button_handle_t btn_c = button_create(&btn_c_conf);
    button_register_callback(btn_c, on_general_button_event);

    ESP_LOGI(TAG, "All components initialized. Starting display task.");
    if (xSemaphoreTake(lcd_mutex, portMAX_DELAY) == pdTRUE) {
        lcd_i2c_clear();
        lcd_i2c_set_cursor(0, 2);
        lcd_i2c_send_string("Clock Ready");
        xSemaphoreGive(lcd_mutex);
    }
    xTaskCreate(read_time_task, "read_time_task", 2048, NULL, 5, NULL);
}