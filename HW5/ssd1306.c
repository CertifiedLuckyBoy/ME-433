// Based on the Adafruit and SparkFun SSD1306 libraries, adapted for ME433.

#include <stdint.h>
#include <string.h>
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

#define SSD1306_ADDRESS 0x3C
#define SSD1306_I2C_PORT i2c0
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 32
#define SSD1306_PAGES (SSD1306_HEIGHT / 8)
#define SSD1306_BUFFER_SIZE (1 + SSD1306_WIDTH * SSD1306_HEIGHT / 8)

static unsigned char ssd1306_buffer[SSD1306_BUFFER_SIZE];

void ssd1306_command(unsigned char c) {
    uint8_t buf[2] = {0x00, c};
    i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_ADDRESS, buf, 2, false);
}

void ssd1306_setup(void) {
    ssd1306_buffer[0] = 0x40;
    sleep_ms(20);

    ssd1306_command(SSD1306_DISPLAYOFF);
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);
    ssd1306_command(0x80);
    ssd1306_command(SSD1306_SETMULTIPLEX);
    ssd1306_command(0x1F);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_SETSTARTLINE);
    ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
    ssd1306_command(SSD1306_CHARGEPUMP);
    ssd1306_command(0x14);
    ssd1306_command(SSD1306_MEMORYMODE);
    ssd1306_command(0x00);
    ssd1306_command(SSD1306_SEGREMAP | 0x01);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);
    ssd1306_command(0x02);
    ssd1306_command(SSD1306_SETCONTRAST);
    ssd1306_command(0x8F);
    ssd1306_command(SSD1306_SETPRECHARGE);
    ssd1306_command(0xF1);
    ssd1306_command(SSD1306_SETVCOMDETECT);
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);
    ssd1306_command(SSD1306_NORMALDISPLAY);
    ssd1306_command(SSD1306_DISPLAYON);

    ssd1306_clear();
    ssd1306_update();
}

void ssd1306_update(void) {
    ssd1306_command(SSD1306_PAGEADDR);
    ssd1306_command(0);
    ssd1306_command(SSD1306_PAGES - 1);
    ssd1306_command(SSD1306_COLUMNADDR);
    ssd1306_command(0);
    ssd1306_command(SSD1306_WIDTH - 1);

    i2c_write_blocking(SSD1306_I2C_PORT, SSD1306_ADDRESS, ssd1306_buffer,
                       SSD1306_BUFFER_SIZE, false);
}

void ssd1306_drawPixel(int x, int y, unsigned char color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) {
        return;
    }

    if (color) {
        ssd1306_buffer[1 + x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
    } else {
        ssd1306_buffer[1 + x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
    }
}

void ssd1306_clear(void) {
    memset(ssd1306_buffer + 1, 0, SSD1306_BUFFER_SIZE - 1);
    ssd1306_buffer[0] = 0x40;
}
