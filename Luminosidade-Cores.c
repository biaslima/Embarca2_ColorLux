#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pico/bootrom.h"

// Libraries
#include "lib/ssd1306.h"
#include "lib/sensores.h"
#include "lib/font.h" 

// Display Definitions (assuming it uses the shared I2C)
#define endereco 0x3C
ssd1306_t ssd;

// BOOTSEL Button Definition
#define botaoB 6

// --- LED RGB PINS ---
const uint BTN_A_PIN = 5;
const uint RED_PIN = 13;
const uint GREEN_PIN = 11;
const uint BLUE_PIN = 12;

// Display na I2C
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

// --- Control Variables ---
volatile int led_state = 0;
volatile uint32_t last_press_time = 0;

// --- GPIO Interrupt Handler ---
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == botaoB) {
        reset_usb_boot(0, 0);
    } else if (gpio == BTN_A_PIN) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_press_time > 250) {
            last_press_time = current_time;
            led_state++;
            if (led_state > 3) {
                led_state = 0;
            }
        }
    }
}

int main() {
    // --- BOOTSEL Setup ---
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    sleep_ms(2000);

    // --- I2C0 Initialization (for all devices) ---
    i2c_init(I2C_PORT_SHARED, 400 * 1000);
    gpio_set_function(SDA_PIN_SHARED, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_SHARED, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_SHARED);
    gpio_pull_up(SCL_PIN_SHARED);

    // SSD1306 Display Setup
    i2c_init(I2C_PORT_DISP, 400 * 1000);

    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);                    
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);                    
    gpio_pull_up(I2C_SDA_DISP);                                        
    gpio_pull_up(I2C_SCL_DISP);                                        
    ssd1306_t ssd;                                                     
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP); 
    ssd1306_config(&ssd);                                              
    ssd1306_send_data(&ssd);   

    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Sensors Setup
    bh1750_power_on();
    printf("BH1750 initialized.\n");
    gy33_init();
    printf("GY-33 initialized.\n");

    // --- LED RGB GPIO Setup ---
    gpio_init(RED_PIN);
    gpio_init(GREEN_PIN);
    gpio_init(BLUE_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    // --- Button A GPIO and Interrupt Setup ---
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // --- Main Loop ---
    char str_lux[10];
    char str_red[5];
    char str_green[5];
    char str_blue[5];

    while (1) {
        // Read GY-33
        uint16_t r, g, b, c;
        gy33_read_color(&r, &g, &b, &c);
        
        // Read BH1750
        uint16_t lux = bh1750_read_measurement();

        // Print to console
        printf("Cor detectada - R: %d, G: %d, B: %d, Clear: %d | Lux = %d\n", r, g, b, c, lux);

        // Convert to strings for display
        sprintf(str_red, "R:%d", r);
        sprintf(str_green, "G:%d", g);
        sprintf(str_blue, "B:%d", b);
        sprintf(str_lux, "Lux:%d", lux);

        // Update Display
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "CEPEDI TIC37", 8, 6);
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
        ssd1306_draw_string(&ssd, str_red, 14, 30);
        ssd1306_draw_string(&ssd, str_green, 14, 40);
        ssd1306_draw_string(&ssd, str_blue, 14, 50);
        ssd1306_draw_string(&ssd, str_lux, 60, 40);
        ssd1306_send_data(&ssd);

        // Update LED based on button press
        if (led_state == 0) {
            gpio_put(RED_PIN, 1);
            gpio_put(GREEN_PIN, 0);
            gpio_put(BLUE_PIN, 0);
        } else if (led_state == 1) {
            gpio_put(RED_PIN, 1);
            gpio_put(GREEN_PIN, 1);
            gpio_put(BLUE_PIN, 0);
        } else if (led_state == 2) {
            gpio_put(RED_PIN, 0);
            gpio_put(GREEN_PIN, 1);
            gpio_put(BLUE_PIN, 0);
        } else {
            gpio_put(RED_PIN, 0);
            gpio_put(GREEN_PIN, 0);
            gpio_put(BLUE_PIN, 1);
        }

        sleep_ms(500);
    }
    return 0;
}