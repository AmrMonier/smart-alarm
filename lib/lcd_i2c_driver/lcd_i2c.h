#ifndef LCD_I2C_H
#define LCD_I2C_H

#include <stdint.h>
#include "driver/i2c.h"

#define LCD_I2C_DEFAULT_ADDRESS 0x27

typedef struct {
    i2c_port_t i2c_port;
    uint8_t i2c_address;
    uint8_t cols;
    uint8_t rows;
} lcd_i2c_config_t;

esp_err_t lcd_i2c_init(const lcd_i2c_config_t *config);
void lcd_i2c_clear(void);
void lcd_i2c_set_cursor(uint8_t row, uint8_t col);
void lcd_i2c_send_string(const char *str);

#endif // LCD_I2C_H
