#include "sensores.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// --- BH1750 Functions ---
void _bh1750_i2c_write_byte(uint8_t byte) {
    i2c_write_blocking(I2C_PORT_SHARED, BH1750_I2C_ADDR, &byte, 1, false);
}

void bh1750_power_on() {
    _bh1750_i2c_write_byte(_POWER_ON_C);
}

uint16_t bh1750_read_measurement() {
    _bh1750_i2c_write_byte(_CONT_HRES_C);
    sleep_ms(200);
    uint8_t buff[2];
    i2c_read_blocking(I2C_PORT_SHARED, BH1750_I2C_ADDR, buff, 2, false);
    return (((uint16_t)buff[0] << 8) | buff[1]) / 1.2;
}

// --- GY-33 Functions ---
void gy33_write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(I2C_PORT_SHARED, GY33_I2C_ADDR, buffer, 2, false);
}

uint16_t gy33_read_register(uint8_t reg) {
    uint8_t buffer[2];
    i2c_write_blocking(I2C_PORT_SHARED, GY33_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_PORT_SHARED, GY33_I2C_ADDR, buffer, 2, false);
    return (buffer[1] << 8) | buffer[0];
}

void gy33_init() {
    gy33_write_register(ENABLE_REG, 0x03);
    gy33_write_register(ATIME_REG, 0xF5);
    gy33_write_register(CONTROL_REG, 0x00);
}

void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c) {
    *c = gy33_read_register(CDATA_REG);
    *r = gy33_read_register(RDATA_REG);
    *g = gy33_read_register(GDATA_REG);
    *b = gy33_read_register(BDATA_REG);
}