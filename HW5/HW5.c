#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

// HW5: MPU6050 accelerometer/gyro over I2C, displayed as an OLED tilt vector.
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_BAUDRATE_HZ (400 * 1000)
#define I2C_TIMEOUT_US 5000

#define MPU6050_ADDR 0x68
#define MPU6050_WHO_AM_I 0x75
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_ACCEL_XOUT_H 0x3B

#define OLED_WIDTH 128
#define OLED_HEIGHT 32
#define OLED_CENTER_X 64
#define OLED_CENTER_Y 16

#define ACCEL_COUNTS_PER_G 16384
#define X_PIXELS_PER_G 50
#define Y_PIXELS_PER_G 14
#define DISPLAY_X_SIGN -1
#define DISPLAY_Y_SIGN 1
#define OLED_UPDATE_DIVIDER 5

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t temp;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu6050_data_t;

static int int_abs(int value) {
    return value < 0 ? -value : value;
}

static int clamp_int(int value, int min_value, int max_value) {
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

static void draw_line(int x0, int y0, int x1, int y1) {
    int dx = int_abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -int_abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        ssd1306_drawPixel(x0, y0, 1);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

static void draw_oled_self_test(void) {
    ssd1306_clear();

    for (int x = 0; x < OLED_WIDTH; x++) {
        ssd1306_drawPixel(x, 0, 1);
        ssd1306_drawPixel(x, OLED_HEIGHT - 1, 1);
    }
    for (int y = 0; y < OLED_HEIGHT; y++) {
        ssd1306_drawPixel(0, y, 1);
        ssd1306_drawPixel(OLED_WIDTH - 1, y, 1);
    }

    draw_line(0, 0, OLED_WIDTH - 1, OLED_HEIGHT - 1);
    draw_line(0, OLED_HEIGHT - 1, OLED_WIDTH - 1, 0);
    ssd1306_update();
}

static void draw_accel_vector(int16_t ax, int16_t ay) {
    int x_end = OLED_CENTER_X +
                DISPLAY_X_SIGN * ax * X_PIXELS_PER_G / ACCEL_COUNTS_PER_G;
    int y_end = OLED_CENTER_Y +
                DISPLAY_Y_SIGN * ay * Y_PIXELS_PER_G / ACCEL_COUNTS_PER_G;

    x_end = clamp_int(x_end, 0, OLED_WIDTH - 1);
    y_end = clamp_int(y_end, 0, OLED_HEIGHT - 1);

    ssd1306_clear();
    draw_line(OLED_CENTER_X, OLED_CENTER_Y, x_end, y_end);

    ssd1306_drawPixel(OLED_CENTER_X - 1, OLED_CENTER_Y, 1);
    ssd1306_drawPixel(OLED_CENTER_X + 1, OLED_CENTER_Y, 1);
    ssd1306_drawPixel(OLED_CENTER_X, OLED_CENTER_Y - 1, 1);
    ssd1306_drawPixel(OLED_CENTER_X, OLED_CENTER_Y + 1, 1);
    ssd1306_update();
}

static int mpu6050_write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, data, 2, false, I2C_TIMEOUT_US);
}

static int mpu6050_read_register(uint8_t reg, uint8_t *value) {
    int result = i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &reg, 1, true, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    return i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, value, 1, false, I2C_TIMEOUT_US);
}

static int mpu6050_read_data(mpu6050_data_t *data) {
    uint8_t raw[14];
    uint8_t start_reg = MPU6050_ACCEL_XOUT_H;

    int result = i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &start_reg, 1, true, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    result = i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, raw, sizeof(raw), false, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    data->ax = (int16_t)((raw[0] << 8) | raw[1]);
    data->ay = (int16_t)((raw[2] << 8) | raw[3]);
    data->az = (int16_t)((raw[4] << 8) | raw[5]);
    data->temp = (int16_t)((raw[6] << 8) | raw[7]);
    data->gx = (int16_t)((raw[8] << 8) | raw[9]);
    data->gy = (int16_t)((raw[10] << 8) | raw[11]);
    data->gz = (int16_t)((raw[12] << 8) | raw[13]);

    return result;
}

static bool mpu6050_setup(void) {
    uint8_t who_am_i = 0;
    int result = mpu6050_read_register(MPU6050_WHO_AM_I, &who_am_i);
    if (result < 0) {
        printf("Failed to read WHO_AM_I from MPU6050\n");
        return false;
    }

    printf("MPU6050 WHO_AM_I = 0x%02X\n", who_am_i);
    if (who_am_i != 0x68 && who_am_i != 0x98) {
        printf("Unexpected WHO_AM_I value\n");
        return false;
    }

    if (mpu6050_write_register(MPU6050_PWR_MGMT_1, 0x00) < 0) {
        printf("Failed to wake MPU6050\n");
        return false;
    }
    sleep_ms(100);

    if (mpu6050_write_register(MPU6050_ACCEL_CONFIG, 0x00) < 0) {
        printf("Failed to configure accelerometer\n");
        return false;
    }
    if (mpu6050_write_register(MPU6050_GYRO_CONFIG, 0x18) < 0) {
        printf("Failed to configure gyroscope\n");
        return false;
    }

    return true;
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    i2c_init(I2C_PORT, I2C_BAUDRATE_HZ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_setup();
    draw_oled_self_test();
    sleep_ms(750);
    ssd1306_clear();
    ssd1306_update();

    if (!mpu6050_setup()) {
        while (true) {
            printf("MPU6050 setup failed\n");
            sleep_ms(1000);
        }
    }

    absolute_time_t next_sample = make_timeout_time_ms(10);
    int sample_count = 0;

    while (true) {
        mpu6050_data_t data;
        int result = mpu6050_read_data(&data);

        if (result >= 0) {
            printf("ax:%6d ay:%6d az:%6d temp:%6d gx:%6d gy:%6d gz:%6d\n",
                   data.ax, data.ay, data.az, data.temp, data.gx, data.gy, data.gz);

            if ((sample_count % OLED_UPDATE_DIVIDER) == 0) {
                draw_accel_vector(data.ax, data.ay);
            }
            sample_count++;
        } else {
            printf("MPU6050 read failed\n");
        }

        sleep_until(next_sample);
        next_sample = delayed_by_ms(next_sample, 10);
    }
}
