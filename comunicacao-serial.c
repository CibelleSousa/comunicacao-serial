#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"
#include "serial.pio.h"

// Defines
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BTN_A 5
#define BTN_B 6
#define OUT_PIN 7
#define LED_G 11
#define LED_B 12

// Variáveis Globais
static volatile uint32_t last_time = 0;
ssd1306_t ssd;

// Frames
double contagem [10][25] = {
    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25},

     {0.25, 0.25, 0.25, 0.25, 0.25,
      0.00, 0.00, 0.25, 0.00, 0.00,
      0.00, 0.00, 0.25, 0.00, 0.25,
      0.00, 0.25, 0.25, 0.00, 0.00,
      0.00, 0.00, 0.25, 0.00, 0.00},
    
    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.00,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.00, 0.00, 0.00, 0.00,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25},

    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.00,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.00,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.00, 0.00, 0.00, 0.00,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.00,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25},

    {0.25, 0.00, 0.00, 0.00, 0.00,
     0.00, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25,
     0.25, 0.00, 0.00, 0.00, 0.25,
     0.25, 0.25, 0.25, 0.25, 0.25}
};

// Prototipação das Funções
void setup();
void setupDisplay();
void gpio_irq_handler(uint gpio, uint32_t events);
void exibir_numero(PIO pio, uint sm, uint8_t num);
uint32_t matrix_rgb(double r, double g, double b);

int main()
{
    // Configuração inicial dos GPIOs, interrupções e Display
    set_sys_clock_khz(128000, false);
    stdio_init_all();
    setupDisplay();
    setup();

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &serial_program);
    uint sm = pio_claim_unused_sm(pio, true);
    serial_program_init(pio, sm, offset, OUT_PIN);
    
    while (true) {
        if(stdio_usb_connected()){
            char c;
            if(scanf("%c", &c) == 1){
                ssd1306_fill(&ssd, false); // Limpa o display
                ssd1306_draw_char(&ssd, c, 48, 16, 4);
                if (c >= '0' && c <= '9'){
                    // Printa o número na matriz de LEDs
                    exibir_numero(pio, sm, (c - '0'));
                }
            }
        }
        ssd1306_send_data(&ssd); // Atualiza o display
        sleep_ms(50);
    }
}

void setup(){
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, false);

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);
    gpio_put(LED_B, false);
    
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

void setupDisplay(){
    // I2C Inicialização. Setando frequência a 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Seta a GPIO para I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Seta a GPIO para I2C
    gpio_pull_up(I2C_SDA); // Pull up the data line
    gpio_pull_up(I2C_SCL); // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display

    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);
}

void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (current_time - last_time > 200000){
        last_time = current_time;
        ssd1306_fill(&ssd, false); // Limpa o display
        if(gpio == BTN_A){
            gpio_put(LED_G, !gpio_get(LED_G));
            ssd1306_draw_string(&ssd, "LED VERDE", 8, 10);
            ssd1306_draw_string(&ssd, "ALTERNADO", 8, 30);
            printf("LED verde alternado!\n");
        }
        else if(gpio == BTN_B){
            gpio_put(LED_B, !gpio_get(LED_B));
            ssd1306_draw_string(&ssd, "LED AZUL", 8, 10);
            ssd1306_draw_string(&ssd, "ALTERNADO", 8, 30);
            printf("LED azul alternado!\n");
        }
        ssd1306_send_data(&ssd);
    }
}

uint32_t matrix_rgb(double r, double g, double b) {
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

void exibir_numero(PIO pio, uint sm, uint8_t num) {

    for (int i = 0; i < 25; i++) {
            uint32_t color = matrix_rgb(contagem[num][i], contagem[num][i], contagem[num][i]);
            pio_sm_put_blocking(pio, sm, color);
        }

}