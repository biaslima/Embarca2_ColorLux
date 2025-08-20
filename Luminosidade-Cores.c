#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/bootrom.h"

#include "lib/ssd1306.h"
#include "lib/sensores.h"
#include "lib/font.h" 
#include "lib/ws2812b.h"

// --- Constantes ---
#define MAX_LUX 1000 // Valor máximo de lux para o cálculo de intensidade (ajuste conforme necessário)

// --- Pinos ---
#define BTN_BOOTSEL_PIN 6
const uint RED_PIN = 13;
const uint GREEN_PIN = 11;
const uint BLUE_PIN = 12;

// --- Display I2C ---
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define DISPLAY_ADDR 0x3C

// --- Buzzer ---
// Constantes para configuração do buzzer por PWM
// Frequência de aproximadamente 440 Hz
const uint BUZZER_PIN = 21;
const uint16_t PERIOD = 59609; // WRAP
const float DIVCLK = 16.0; // Divisor inteiro
static uint slice_21;
const uint16_t dc_values[] = {PERIOD * 0.3, 0}; // Duty Cycle de 30% e 0%

// --- Funções Auxiliares ---

// Função para configurar um pino para PWM
void init_pwm_pin(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    
    // Usa uma configuração padrão, o divisor de clock pode ser ajustado se necessário
    pwm_config config = pwm_get_default_config();
    pwm_init(slice_num, &config, true);
}

// Handler de interrupção para o botão BOOTSEL
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BTN_BOOTSEL_PIN) {
        reset_usb_boot(0, 0);
    }
}

// Inicializa o buzzer com as configurações de PWM
void init_buzzer(uint gpio_pin, float clkdiv, uint16_t period) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio_pin);
    pwm_set_clkdiv(slice, clkdiv);
    pwm_set_wrap(slice, period);
    pwm_set_gpio_level(gpio_pin, 0); // Inicia desligado
    pwm_set_enabled(slice, true);
}

// --- Função Principal ---
int main() {
    stdio_init_all();
    sleep_ms(2000); // Pausa para inicializar o monitor serial

    // --- Matriz de LEDs WS2812B ---
    ws2812b_t *ws = init_ws2812b(pio0, WS2812B_PIN);

    // --- Botão BOOTSEL ---
    gpio_init(BTN_BOOTSEL_PIN);
    gpio_set_dir(BTN_BOOTSEL_PIN, GPIO_IN);
    gpio_pull_up(BTN_BOOTSEL_PIN);
    gpio_set_irq_enabled_with_callback(BTN_BOOTSEL_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // --- I2C dos Sensores ---
    i2c_init(I2C_PORT_SHARED, 400 * 1000);
    gpio_set_function(SDA_PIN_SHARED, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN_SHARED, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN_SHARED);
    gpio_pull_up(SCL_PIN_SHARED);

    // --- Display OLED SSD1306 ---
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);                    
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);                    
    gpio_pull_up(I2C_SDA_DISP);                                        
    gpio_pull_up(I2C_SCL_DISP);                                        
    ssd1306_t ssd;                                                     
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, DISPLAY_ADDR, I2C_PORT_DISP); 
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);                                              
    ssd1306_send_data(&ssd);   

    // --- Buzzer ---
    init_buzzer(BUZZER_PIN, DIVCLK, PERIOD);

    // --- Sensores ---
    bh1750_power_on();
    printf("BH1750 inicializado.\n");
    gy33_init();
    printf("GY-33 inicializado.\n");

    // --- LED RGB com PWM ---
    init_pwm_pin(RED_PIN);
    init_pwm_pin(GREEN_PIN);
    init_pwm_pin(BLUE_PIN);

    // --- Buffers para strings do display ---
    char str_lux[10];
    char str_red[5];
    char str_green[5];
    char str_blue[5];

    // --- Loop Principal ---
    while (1) {
        // Leitura dos sensores
        uint16_t r, g, b, c;
        gy33_read_color(&r, &g, &b, &c);
        uint16_t lux = bh1750_read_measurement();

        printf("Cor: R=%d, G=%d, B=%d, C=%d | Luminosidade: %d lux\n", r, g, b, c, lux);

        // --- Lógica de Controle dos LEDs ---
        
        // 1. Normaliza os valores de cor (0-255)
        // Encontra o valor máximo entre r, g, b para manter a proporção da cor
        uint16_t max_color = (r > g) ? r : g;
        max_color = (max_color > b) ? max_color : b;
        
        uint8_t r_norm = 0, g_norm = 0, b_norm = 0;
        if (max_color > 0) {
            r_norm = (uint8_t)((r * 255.0f) / max_color);
            g_norm = (uint8_t)((g * 255.0f) / max_color);
            b_norm = (uint8_t)((b * 255.0f) / max_color);
        }

        // 2. Calcula a intensidade baseada na luminosidade (0.0 a 1.0)
        float intensity = (lux >= MAX_LUX) ? 1.0f : (float)lux / MAX_LUX;
        
        // 3. Aplica a intensidade aos valores de cor normalizados
        uint8_t final_r = (uint8_t)(r_norm * intensity);
        uint8_t final_g = (uint8_t)(g_norm * intensity);
        uint8_t final_b = (uint8_t)(b_norm * intensity);

        // 4. Atualiza o LED RGB com PWM
        // O nível do PWM é ao quadrado para uma percepção de brilho mais linear (correção de gamma)
        pwm_set_gpio_level(RED_PIN, final_r * final_r);
        pwm_set_gpio_level(GREEN_PIN, final_g * final_g);
        pwm_set_gpio_level(BLUE_PIN, final_b * final_b);

        // 5. Atualiza a matriz de LEDs WS2812B com a nova função
        // Passa os valores de cor e intensidade diretamente
        ws2812b_draw_rgb(ws, ZERO_GLYPH, final_r, final_g, final_b);

        // 6. Emite o alerta do buzzer, caso a luminosidade esteja muito baixa ou a cor vermelha seja identificada em maior parte
        if(lux < 1 || (r > g && r > b))
            pwm_set_gpio_level(BUZZER_PIN, dc_values[0]);
        
        else
            pwm_set_gpio_level(BUZZER_PIN, dc_values[1]);
        
        // --- Atualização do Display ---
        sprintf(str_red, "R:%d", r);
        sprintf(str_green, "G:%d", g);
        sprintf(str_blue, "B:%d", b);
        sprintf(str_lux, "Lux:%d", lux);

        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "CEPEDI TIC37", 8, 6);
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);
        ssd1306_draw_string(&ssd, str_red, 14, 30);
        ssd1306_draw_string(&ssd, str_green, 14, 40);
        ssd1306_draw_string(&ssd, str_blue, 14, 50);
        ssd1306_draw_string(&ssd, str_lux, 60, 40);
        ssd1306_send_data(&ssd);
        sleep_ms(250); // Reduz o delay para uma resposta mais rápida
    }
    return 0;
}