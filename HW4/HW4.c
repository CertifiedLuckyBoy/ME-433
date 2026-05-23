#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define HEARTBEAT_PIN 15
#define ADC0_PIN 26
#define ADC0_INPUT 0

void drawChar(uint8_t x, uint8_t y, char c)
{
    if (c < 0x20 || c > 0x7f) {
        return;
    }

    for (int i = 0; i < 5; i++) {
        uint8_t line = ASCII[c - 0x20][i];
        for (int j = 0; j < 8; j++) {
            ssd1306_drawPixel(x + i, y + j, line & 0x1);
            line >>= 1;
        }
    }
}

void drawString(uint8_t x, uint8_t y, const char* message)
{
    while (*message) {
        drawChar(x, y, *message++);
        x += 6; // 5 pixels for the character and 1 pixel for spacing
    }
}   

int main()
{
    stdio_init_all();

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400*1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // For more examples of I2C use see https://github.com/raspberrypi/pico-examples/tree/master/i2c

    gpio_init(HEARTBEAT_PIN);
    gpio_set_dir(HEARTBEAT_PIN, GPIO_OUT);

    ssd1306_setup();

    adc_init();
    adc_gpio_init(ADC0_PIN);
    adc_select_input(ADC0_INPUT);

    bool on = false;
    uint64_t next_heartbeat_us = to_us_since_boot(get_absolute_time());
    uint32_t last_frame_time_us = 1;

    while (true) {
        uint64_t start_time_us = to_us_since_boot(get_absolute_time());

        if (start_time_us >= next_heartbeat_us) {
            on = !on;
            gpio_put(HEARTBEAT_PIN, on);
            next_heartbeat_us = start_time_us + 500000;
        }

        uint16_t raw = adc_read();
        uint32_t millivolts = raw * 3300 / 4095;
        uint32_t fps = 1000000 / last_frame_time_us;

        char voltage_message[32];
        char raw_message[32];
        char fps_message[32];

        sprintf(voltage_message, "ADC0: %d.%03d V", millivolts / 1000, millivolts % 1000);
        sprintf(raw_message, "raw: %4d", raw);
        sprintf(fps_message, "fps: %lu", fps);

        ssd1306_clear();
        drawString(0, 0, "ME433 HW4");
        drawString(0, 8, voltage_message);
        drawString(0, 16, raw_message);
        drawString(0, 24, fps_message);
        ssd1306_update();

        uint64_t end_time_us = to_us_since_boot(get_absolute_time());
        last_frame_time_us = end_time_us - start_time_us;
        if (last_frame_time_us == 0) {
            last_frame_time_us = 1;
        }
    }
}
