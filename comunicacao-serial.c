#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/font.h"

// Defines
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BTN_A 5
#define BTN_B 6
#define LED_G 11
#define LED_B 12

// Variáveis Globais
bool cor = false;
static volatile uint32_t last_time = 0;
ssd1306_t ssd;

// Prototipação das Funções
void setup();
void setupDisplay();
void gpio_irq_handler(uint gpio, uint32_t events);

int main()
{
    // Configuração inicial dos GPIOs, interrupções e Display
    stdio_init_all();
    setupDisplay();
    setup();
    

    while (true) {
        if(stdio_usb_connected()){
            char c;
            if(scanf("%c", &c) == 1){
                ssd1306_fill(&ssd, cor); // Limpa o display
                ssd1306_draw_char(&ssd, c, 48, 16, 4);
            }
            if (c >= '0' || c <= '9'){
                // implementar função que 'printa o número na matriz de LEDs
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
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
            ssd1306_draw_string(&ssd, "LED VERDE", 8, 10);
            ssd1306_draw_string(&ssd, "ALTERNADO", 8, 30);
            printf("LED verde alternado!\n");
        }
        else if(gpio == BTN_B){
            gpio_put(LED_B, !gpio_get(LED_B));
            ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo
            ssd1306_draw_string(&ssd, "LED AZUL", 8, 10);
            ssd1306_draw_string(&ssd, "ALTERNADO", 8, 30);
            printf("LED azul alternado!\n");
        }
        ssd1306_send_data(&ssd);
    }
}