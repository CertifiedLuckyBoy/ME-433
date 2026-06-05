t#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define SPI_PORT spi0
#define PIN_MISO   16
#define PIN_DAC_CS 17
#define PIN_SCK    18
#define PIN_MOSI   19
#define PIN_RAM_CS 20

#define SPI_BAUD_HZ       (1000 * 1000)
#define NUM_SAMPLES       1000
#define SAMPLE_DELAY_US   1000
#define SINE_AMPLITUDE_V  1.65f
#define SINE_OFFSET_V     1.65f
#define DAC_VREF_V        3.30f
#define DAC_MAX_CODE      1023
#define PI                3.14159265358979323846f

#define RAM_CMD_READ      0x03
#define RAM_CMD_WRITE     0x02
#define RAM_CMD_WRSR      0x01
#define RAM_MODE_SEQ      0x40

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

static uint16_t dac_command(uint16_t code)
{
    if (code > DAC_MAX_CODE) {
        code = DAC_MAX_CODE;
    }

    // MCP4912 channel A, Vref unbuffered, gain=1x, output enabled.
    return (1 << 13) | (1 << 12) | (code << 2);
}

static uint16_t voltage_to_dac_code(float voltage)
{
    if (voltage < 0.0f) {
        voltage = 0.0f;
    } else if (voltage > DAC_VREF_V) {
        voltage = DAC_VREF_V;
    }

    return (uint16_t)((voltage / DAC_VREF_V) * DAC_MAX_CODE + 0.5f);
}

static void dac_write_bytes(const uint8_t data[2])
{
    cs_select(PIN_DAC_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_DAC_CS);
}

static void ram_write_status(uint8_t status)
{
    uint8_t data[2] = {RAM_CMD_WRSR, status};

    cs_select(PIN_RAM_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_RAM_CS);
}

static void ram_write_sine_table(void)
{
    uint8_t header[3] = {RAM_CMD_WRITE, 0x00, 0x00};

    cs_select(PIN_RAM_CS);
    spi_write_blocking(SPI_PORT, header, 3);

    for (uint i = 0; i < NUM_SAMPLES; i++) {
        float angle = 2.0f * PI * (float)i / (float)NUM_SAMPLES;
        float voltage = SINE_OFFSET_V + SINE_AMPLITUDE_V * sinf(angle);
        uint16_t command = dac_command(voltage_to_dac_code(voltage));
        uint8_t data[2] = {command >> 8, command & 0xff};
        spi_write_blocking(SPI_PORT, data, 2);
    }

    cs_deselect(PIN_RAM_CS);
}

static void ram_read_bytes(uint16_t address, uint8_t *data, size_t length)
{
    uint8_t header[3] = {
        RAM_CMD_READ,
        (uint8_t)(address >> 8),
        (uint8_t)(address & 0xff)
    };

    cs_select(PIN_RAM_CS);
    spi_write_blocking(SPI_PORT, header, 3);
    spi_read_blocking(SPI_PORT, 0, data, length);
    cs_deselect(PIN_RAM_CS);
}

int main()
{
    stdio_init_all();

    spi_init(SPI_PORT, SPI_BAUD_HZ);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);

    gpio_set_function(PIN_DAC_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_DAC_CS, GPIO_OUT);
    gpio_put(PIN_DAC_CS, 1);

    gpio_set_function(PIN_RAM_CS, GPIO_FUNC_SIO);
    gpio_set_dir(PIN_RAM_CS, GPIO_OUT);
    gpio_put(PIN_RAM_CS, 1);

    ram_write_status(RAM_MODE_SEQ);
    ram_write_sine_table();

    uint16_t address = 0;
    absolute_time_t next_update = get_absolute_time();

    while (true) {
        uint8_t dac_data[2];
        ram_read_bytes(address, dac_data, sizeof(dac_data));
        dac_write_bytes(dac_data);

        address += 2;
        if (address >= NUM_SAMPLES * 2) {
            address = 0;
        }

        next_update = delayed_by_us(next_update, SAMPLE_DELAY_US);
        sleep_until(next_update);
    }
}
