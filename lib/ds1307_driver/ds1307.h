#ifndef DS1307_H
#define DS1307_H

#include <time.h>
#include "driver/i2c.h"
#include <stdbool.h>

#define DS1307_I2C_ADDRESS 0x68

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
} rtc_time_t;

esp_err_t ds1307_init(i2c_port_t i2c_port, int sda_pin, int scl_pin);
esp_err_t ds1307_set_time(i2c_port_t i2c_port, rtc_time_t *time);
esp_err_t ds1307_get_time(i2c_port_t i2c_port, rtc_time_t *time);
esp_err_t ds1307_is_running(i2c_port_t i2c_port, bool *is_running);
esp_err_t ds1307_reset(i2c_port_t i2c_port);

#endif // DS1307_H
