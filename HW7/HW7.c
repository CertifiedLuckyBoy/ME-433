#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
// MCP4912 official 14-pin pinout:
// Pin 3 CS  <- GP17
// Pin 4 SCK <- GP18
// Pin 5 SDI <- GP19
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

#define SPI_BAUD_HZ       (1000 * 1000)
#define UPDATE_RATE_HZ    1000
#define SINE_HZ           2
#define TRIANGLE_HZ       1
#define DAC_MAX_CODE      1023
#define DAC_CHANNEL_A     0
#define DAC_CHANNEL_B     1
#define DAC_BUF_BIT       0
#define PI                3.14159265358979323846f

static inline void cs_select(uint cs_pin)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect(uint cs_pin)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop");
}

static uint16_t dac_command(uint channel, uint16_t voltage)
{
    if (voltage > DAC_MAX_CODE) {
        voltage = DAC_MAX_CODE;
    }

    // MCP4912 command: A/B, BUF, GA=1, SHDN=1, then the 10-bit value left-aligned.
    return ((channel & 0x1) << 15) |
           (DAC_BUF_BIT << 14) |
           (1 << 13) |
           (1 << 12) |
           (voltage << 2);
}

static void dac_write(uint channel, uint16_t voltage)
{
    uint16_t command = dac_command(channel, voltage);
    uint8_t data[2] = {command >> 8, command & 0xff};

    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_CS);
}

static uint16_t sine_code(uint phase)
{
    const uint sine_samples = UPDATE_RATE_HZ / SINE_HZ;
    float angle = 2.0f * PI * (float)phase / (float)sine_samples;
    return (uint16_t)(511.5f + 511.5f * sinf(angle) + 0.5f);
}

static uint16_t triangle_code(uint phase)
{
    const uint triangle_samples = UPDATE_RATE_HZ / TRIANGLE_HZ;
    const uint half_cycle = triangle_samples / 2;

    if (phase < half_cycle) {
        return (uint16_t)((phase * DAC_MAX_CODE) / (half_cycle - 1));
    }

    return (uint16_t)(((triangle_samples - 1 - phase) * DAC_MAX_CODE) / (half_cycle - 1));
}

int main()
{
    stdio_init_all();

    spi_init(SPI_PORT, SPI_BAUD_HZ);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    uint sine_phase = 0;
    uint triangle_phase = 0;
    const uint sine_samples = UPDATE_RATE_HZ / SINE_HZ;
    const uint triangle_samples = UPDATE_RATE_HZ / TRIANGLE_HZ;
    absolute_time_t next_update = get_absolute_time();

    while (true) {
        dac_write(DAC_CHANNEL_A, sine_code(sine_phase));
        dac_write(DAC_CHANNEL_B, triangle_code(triangle_phase));

        sine_phase = (sine_phase + 1) % sine_samples;
        triangle_phase = (triangle_phase + 1) % triangle_samples;

        next_update = delayed_by_us(next_update, 1000000 / UPDATE_RATE_HZ);
        sleep_until(next_update);
    }
}
