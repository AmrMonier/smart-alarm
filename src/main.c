
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ds1307.h"

// Tag for ESP_LOGI
static const char *TAG = "MY_APP";

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM I2C_NUM_0

void app_main()
{
    ESP_LOGI(TAG, "ESP32 application started");

    // Initialize I2C
    if (ds1307_init(I2C_MASTER_NUM, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO) != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return;
    }
    ESP_LOGI(TAG, "I2C initialized successfully");

    // Reset the RTC
    // if (ds1307_reset(I2C_MASTER_NUM) != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to reset RTC");
    // } else {
    //     ESP_LOGI(TAG, "RTC reset successfully");
    // }

    // Check if RTC is running
    bool is_running = false;
    if (ds1307_is_running(I2C_MASTER_NUM, &is_running) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to check RTC status");
    } else {
        if (!is_running) {
            ESP_LOGI(TAG, "RTC is not running, setting time.");
            // Set time
            rtc_time_t time_to_set = {
                .seconds = 0,
                .minutes = 03,
                .hours = 0,
                .day = 5, // Wednesday
                .date = 11,
                .month = 9,
                .year = 25 // 2025
            };

            if (ds1307_set_time(I2C_MASTER_NUM, &time_to_set) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to set time");
            } else {
                ESP_LOGI(TAG, "Time set successfully");
            }
        } else {
            ESP_LOGI(TAG, "RTC is already running.");
        }
    }

    // Read time periodically
    while (1) {
        rtc_time_t current_time;
        if (ds1307_get_time(I2C_MASTER_NUM, &current_time) == ESP_OK) {
            ESP_LOGI(TAG, "Current time: %02d:%02d:%02d", current_time.hours, current_time.minutes, current_time.seconds);
        } else {
            ESP_LOGE(TAG, "Failed to get time");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
