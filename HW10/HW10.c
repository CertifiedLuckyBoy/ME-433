#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#define I2C_PORT i2c0
#define I2C_SDA_PIN 4
#define I2C_SCL_PIN 5
#define I2C_BAUD_HZ (400 * 1000)

#define MPU6050_ADDR 0x68
#define MPU6050_PWR_MGMT_1 0x6B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_DATA_START 0x3B

#define BUTTON_PIN 15
#define SEND_RATE_HZ 60

static void mpu6050_write(uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, MPU6050_ADDR, data, 2, false);
}

static int mpu6050_read(uint8_t reg, uint8_t *data, uint8_t length)
{
    int result = i2c_write_blocking(I2C_PORT, MPU6050_ADDR, &reg, 1, true);
    if (result < 0) {
        return 0;
    }

    result = i2c_read_blocking(I2C_PORT, MPU6050_ADDR, data, length, false);
    return result == length;
}

static int16_t make_int16(uint8_t high, uint8_t low)
{
    return (int16_t)((high << 8) | low);
}

int main()
{
    stdio_init_all();

    i2c_init(I2C_PORT, I2C_BAUD_HZ);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    sleep_ms(100);

    // Wake the MPU6050 and use the default +/-2g accel and +/-250 dps gyro ranges.
    mpu6050_write(MPU6050_PWR_MGMT_1, 0x00);
    mpu6050_write(MPU6050_ACCEL_CONFIG, 0x00);
    mpu6050_write(MPU6050_GYRO_CONFIG, 0x00);

    absolute_time_t next_send = get_absolute_time();

    while (true) {
        uint8_t data[14];
        int button_pressed = gpio_get(BUTTON_PIN) == 0;

        if (mpu6050_read(MPU6050_DATA_START, data, 14)) {
            int16_t ax = make_int16(data[0], data[1]);
            int16_t ay = make_int16(data[2], data[3]);
            int16_t az = make_int16(data[4], data[5]);
            int16_t gx = make_int16(data[8], data[9]);
            int16_t gy = make_int16(data[10], data[11]);
            int16_t gz = make_int16(data[12], data[13]);

            printf("(%d,%d,%d,%d,%d,%d,%d)\n", ax, ay, az, gx, gy, gz, button_pressed);
        } else {
            printf("(0,0,0,0,0,0,%d)\n", button_pressed);
        }

        next_send = delayed_by_us(next_send, 1000000 / SEND_RATE_HZ);
        sleep_until(next_send);
    }
}
