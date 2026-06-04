#include <stdio.h>

#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "ssd1306.h"

// HW5 starter:
// Goal 1: talk to the MPU6050 over I2C.
// Goal 2: read ax/ay/az/temp/gx/gy/gz at 100 Hz.
// Goal 3: draw a tilt vector on the OLED.

#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9
#define I2C_BAUDRATE_HZ (400 * 1000)

#define WHO_AM_I 0x75
#define PWR_MGMT_1 0x6B
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define ACCEL_XOUT_H 0x3B

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t temp;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu6050_data_t;

static void i2c_bus_setup(void) {
    i2c_init(I2C_PORT, I2C_BAUDRATE_HZ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

// TODO Step 3: write a helper to write one MPU6050 register.
// static int mpu6050_write_register(uint8_t reg, uint8_t value) { ... }

// TODO Step 4: write a helper to read one MPU6050 register.
// static int mpu6050_read_register(uint8_t reg, uint8_t *value) { ... }

// TODO Step 5: write mpu6050_setup().
// It should:
// 1. read WHO_AM_I and check for 0x68 or 0x98,
// 2. write 0x00 to PWR_MGMT_1,
// 3. configure the accelerometer for +/-2g,
// 4. configure the gyroscope for +/-2000 dps.
static bool mpu6050_setup(void) {
    return false;
}

// TODO Step 6: write mpu6050_read_data().
// Do one burst read of 14 bytes starting at ACCEL_XOUT_H.
// Combine high and low bytes into signed 16-bit values.
static int mpu6050_read_data(mpu6050_data_t *data) {
    (void)data;
    return -1;
}

// TODO Step 8: write a line drawing helper or reuse your HW4 OLED drawing style.
// static void draw_line(int x0, int y0, int x1, int y1) { ... }

// TODO Step 9: convert ax/ay into an OLED line from the center of the screen.
static void draw_accel_vector(int16_t ax, int16_t ay) {
    (void)ax;
    (void)ay;

    ssd1306_clear();
    // Draw something simple first, then replace this with the vector.
    ssd1306_drawPixel(64, 16, 1);
    ssd1306_update();
}

int main(void) {
    stdio_init_all();
    sleep_ms(2000);

    i2c_bus_setup();

    ssd1306_setup();
    ssd1306_clear();
    ssd1306_update();

    if (!mpu6050_setup()) {
        while (true) {
            printf("TODO: finish MPU6050 setup\n");
            sleep_ms(1000);
        }
    }

    absolute_time_t next_sample = make_timeout_time_ms(10);

    while (true) {
        mpu6050_data_t data;

        if (mpu6050_read_data(&data) >= 0) {
            // TODO Step 7: print all seven raw values at 100 Hz.
            printf("ax:%6d ay:%6d az:%6d temp:%6d gx:%6d gy:%6d gz:%6d\n",
                   data.ax, data.ay, data.az, data.temp, data.gx, data.gy, data.gz);

            draw_accel_vector(data.ax, data.ay);
        }

        sleep_until(next_sample);
        next_sample = delayed_by_ms(next_sample, 10);
    }
}
