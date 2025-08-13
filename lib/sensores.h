#ifndef SENSORES_H
#define SENSORES_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C Port and Pins
#define I2C_PORT_SHARED i2c0
#define SDA_PIN_SHARED 0
#define SCL_PIN_SHARED 1

// GY-33 Sensor Definitions
#define GY33_I2C_ADDR 0x29
#define ENABLE_REG 0x80
#define ATIME_REG 0x81
#define CONTROL_REG 0x8F
#define CDATA_REG 0x94
#define RDATA_REG 0x96
#define GDATA_REG 0x98
#define BDATA_REG 0x9A

// BH1750 Sensor Definitions
#define BH1750_I2C_ADDR 0x23
#define _POWER_ON_C 0x01
#define _CONT_HRES_C 0x10

// Function prototypes for BH1750
void bh1750_power_on();
uint16_t bh1750_read_measurement();

// Function prototypes for GY-33
void gy33_init();
void gy33_read_color(uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c);

#endif // SENSORES_H