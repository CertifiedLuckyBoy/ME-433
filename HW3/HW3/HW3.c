#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
// This example will use I2C0 on GPIO8 (SDA) and GPIO9 (SCL) running at 400KHz.
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

#define HEARTBEAT_PIN 15

// MCP23008 with A0, A1, and A2 tied to GND has 7-bit I2C address 0x20.
#define MCP23008_ADDR 0x20
#define IODIR 0x00
#define GPPU 0x06
#define GPIO 0x09
#define OLAT 0x0A

#define EXPANDER_BUTTON_PIN 0
#define EXPANDER_LED_PIN 7
#define I2C_TIMEOUT_US 5000

static int mcp23008_write_register(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write_timeout_us(I2C_PORT, address, data, 2, false, I2C_TIMEOUT_US);
}

static int mcp23008_read_register(uint8_t address, uint8_t reg, uint8_t *value) {
    int result = i2c_write_timeout_us(I2C_PORT, address, &reg, 1, true, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    return i2c_read_timeout_us(I2C_PORT, address, value, 1, false, I2C_TIMEOUT_US);
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

    // GP7 is output, GP0-GP6 are inputs.
    mcp23008_write_register(MCP23008_ADDR, IODIR, 0x7F);
    mcp23008_write_register(MCP23008_ADDR, GPPU, 1 << EXPANDER_BUTTON_PIN);

    bool heartbeat_on = false;
    absolute_time_t next_heartbeat = make_timeout_time_ms(250);

    while (true) {
        uint8_t gpio_value = 0;
        int result = mcp23008_read_register(MCP23008_ADDR, GPIO, &gpio_value);

        if (result >= 0) {
            bool button_pressed = ((gpio_value & (1 << EXPANDER_BUTTON_PIN)) == 0);
            uint8_t led_value = button_pressed ? (1 << EXPANDER_LED_PIN) : 0x00;
            mcp23008_write_register(MCP23008_ADDR, OLAT, led_value);
        } else {
            printf("MCP23008 read failed\n");
        }

        if (absolute_time_diff_us(get_absolute_time(), next_heartbeat) <= 0) {
            heartbeat_on = !heartbeat_on;
            gpio_put(HEARTBEAT_PIN, heartbeat_on);
            next_heartbeat = make_timeout_time_ms(250);
        }

        sleep_ms(10);
    }
}
