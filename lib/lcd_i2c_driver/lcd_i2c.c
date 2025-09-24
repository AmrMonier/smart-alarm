#include "lcd_i2c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Commands
#define LCD_CMD_CLEAR_DISPLAY 0x01
#define LCD_CMD_RETURN_HOME 0x02
#define LCD_CMD_ENTRY_MODE_SET 0x04
#define LCD_CMD_DISPLAY_CONTROL 0x08
#define LCD_CMD_FUNCTION_SET 0x20
#define LCD_CMD_SET_DDRAM_ADDR 0x80

// Flags for entry mode
#define LCD_FLAG_ENTRY_RIGHT 0x00
#define LCD_FLAG_ENTRY_LEFT 0x02
#define LCD_FLAG_ENTRY_SHIFT_INCREMENT 0x01
#define LCD_FLAG_ENTRY_SHIFT_DECREMENT 0x00

// Flags for display on/off control
#define LCD_FLAG_DISPLAY_ON 0x04
#define LCD_FLAG_DISPLAY_OFF 0x00
#define LCD_FLAG_CURSOR_ON 0x02
#define LCD_FLAG_CURSOR_OFF 0x00
#define LCD_FLAG_BLINK_ON 0x01
#define LCD_FLAG_BLINK_OFF 0x00

// Flags for function set
#define LCD_FLAG_8BIT_MODE 0x10
#define LCD_FLAG_4BIT_MODE 0x00
#define LCD_FLAG_2LINE 0x08
#define LCD_FLAG_1LINE 0x00
#define LCD_FLAG_5x10DOTS 0x04
#define LCD_FLAG_5x8DOTS 0x00

// Control bits
#define LCD_BIT_RS (1 << 0) // Register select
#define LCD_BIT_RW (1 << 1) // Read/Write
#define LCD_BIT_E (1 << 2)  // Enable
#define LCD_BACKLIGHT (1 << 3)

static const char *TAG = "LCD_I2C";

// Driver state
static i2c_port_t g_i2c_port;
static uint8_t g_i2c_address;
static uint8_t g_cols;
static uint8_t g_rows;

// Static functions
static esp_err_t lcd_send_nibble(uint8_t nibble, uint8_t flags);
static esp_err_t lcd_send_byte(uint8_t byte, uint8_t flags);
static esp_err_t lcd_write_i2c(uint8_t data);

esp_err_t lcd_i2c_init(const lcd_i2c_config_t *config) {
    g_i2c_port = config->i2c_port;
    g_i2c_address = config->i2c_address;
    g_cols = config->cols;
    g_rows = config->rows;

    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for >40ms after power-on

    // Put LCD into 4-bit mode
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x03, 0);
    vTaskDelay(pdMS_TO_TICKS(5));
    lcd_send_nibble(0x02, 0); // Set 4-bit interface

    // Configure LCD
    lcd_send_byte(LCD_CMD_FUNCTION_SET | LCD_FLAG_4BIT_MODE | LCD_FLAG_2LINE | LCD_FLAG_5x8DOTS, 0);
    lcd_send_byte(LCD_CMD_DISPLAY_CONTROL | LCD_FLAG_DISPLAY_ON | LCD_FLAG_CURSOR_OFF | LCD_FLAG_BLINK_OFF, 0);
    lcd_i2c_clear();
    lcd_send_byte(LCD_CMD_ENTRY_MODE_SET | LCD_FLAG_ENTRY_LEFT | LCD_FLAG_ENTRY_SHIFT_DECREMENT, 0);

    ESP_LOGI(TAG, "LCD initialized successfully");
    return ESP_OK;
}

void lcd_i2c_clear(void) {
    lcd_send_byte(LCD_CMD_CLEAR_DISPLAY, 0);
    vTaskDelay(pdMS_TO_TICKS(5)); // this command takes a long time
}

void lcd_i2c_set_cursor(uint8_t row, uint8_t col) {
    uint8_t row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row >= g_rows) {
        row = g_rows - 1;
    }
    lcd_send_byte(LCD_CMD_SET_DDRAM_ADDR | (col + row_offsets[row]), 0);
}

void lcd_i2c_send_string(const char *str) {
    while (*str) {
        lcd_send_byte((uint8_t)(*str), LCD_BIT_RS);
        str++;
    }
}

static esp_err_t lcd_write_i2c(uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (g_i2c_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(g_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return err;
}

static esp_err_t lcd_send_nibble(uint8_t nibble, uint8_t flags) {
    uint8_t data = (nibble << 4) | flags | LCD_BACKLIGHT;

    // Set data lines and pull E high
    lcd_write_i2c(data | LCD_BIT_E);
    vTaskDelay(pdMS_TO_TICKS(1)); // Enable pulse width

    // Pull E low
    lcd_write_i2c(data & ~LCD_BIT_E);
    vTaskDelay(pdMS_TO_TICKS(1)); // Hold time

    return ESP_OK;
}

static esp_err_t lcd_send_byte(uint8_t byte, uint8_t flags) {
    uint8_t high_nibble = byte >> 4;
    uint8_t low_nibble = byte & 0x0F;

    lcd_send_nibble(high_nibble, flags);
    lcd_send_nibble(low_nibble, flags);

    return ESP_OK;
}
