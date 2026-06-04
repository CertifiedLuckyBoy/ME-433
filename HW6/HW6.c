#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "usb_descriptors.h"

// HW6: Use the MPU6050 as a USB HID mouse.
// Regular mode: tilt controls mouse X/Y using accelerometer X/Y.
// Remote working mode: button toggles a slow circle, LED on.

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

#define BUTTON_PIN 15
#define MODE_LED_PIN 16

#define MOUSE_INTERVAL_MS 10
#define BUTTON_DEBOUNCE_MS 35
#define MPU_RETRY_INTERVAL_MS 500
#define MPU_READ_FAILURE_LIMIT 3

typedef struct {
    int16_t ax;
    int16_t ay;
    int16_t az;
    int16_t temp;
    int16_t gx;
    int16_t gy;
    int16_t gz;
} mpu6050_data_t;

static bool automatic_mode = false;
static bool mpu_ready = false;
static uint8_t mpu_read_failures = 0;

static int mpu6050_write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int result = i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, data, sizeof(data), false, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    return result == (int)sizeof(data) ? 0 : -1;
}

static int mpu6050_read_register(uint8_t reg, uint8_t *value) {
    int result = i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &reg, 1, true, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }
    if (result != 1) {
        return -1;
    }

    result = i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, value, 1, false, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }

    return result == 1 ? 0 : -1;
}

static int mpu6050_read_data(mpu6050_data_t *data) {
    uint8_t raw[14];
    uint8_t start_reg = MPU6050_ACCEL_XOUT_H;

    int result = i2c_write_timeout_us(I2C_PORT, MPU6050_ADDR, &start_reg, 1, true, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }
    if (result != 1) {
        return -1;
    }

    result = i2c_read_timeout_us(I2C_PORT, MPU6050_ADDR, raw, sizeof(raw), false, I2C_TIMEOUT_US);
    if (result < 0) {
        return result;
    }
    if (result != (int)sizeof(raw)) {
        return -1;
    }

    data->ax = (int16_t)((raw[0] << 8) | raw[1]);
    data->ay = (int16_t)((raw[2] << 8) | raw[3]);
    data->az = (int16_t)((raw[4] << 8) | raw[5]);
    data->temp = (int16_t)((raw[6] << 8) | raw[7]);
    data->gx = (int16_t)((raw[8] << 8) | raw[9]);
    data->gy = (int16_t)((raw[10] << 8) | raw[11]);
    data->gz = (int16_t)((raw[12] << 8) | raw[13]);

    return 0;
}

static bool mpu6050_setup(void) {
    uint8_t who_am_i = 0;
    if (mpu6050_read_register(MPU6050_WHO_AM_I, &who_am_i) < 0) {
        return false;
    }

    if (who_am_i != 0x68 && who_am_i != 0x98) {
        return false;
    }

    if (mpu6050_write_register(MPU6050_PWR_MGMT_1, 0x00) < 0) {
        return false;
    }
    sleep_ms(100);

    if (mpu6050_write_register(MPU6050_ACCEL_CONFIG, 0x00) < 0) {
        return false;
    }
    if (mpu6050_write_register(MPU6050_GYRO_CONFIG, 0x18) < 0) {
        return false;
    }

    return true;
}

static void i2c_bus_setup(void) {
    i2c_init(I2C_PORT, I2C_BAUDRATE_HZ);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

static void button_and_led_setup(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    gpio_init(MODE_LED_PIN);
    gpio_set_dir(MODE_LED_PIN, GPIO_OUT);
    gpio_put(MODE_LED_PIN, 0);
}

static void update_mode_button(void) {
    static bool last_raw_pressed = false;
    static bool debounced_pressed = false;
    static uint32_t last_change_ms = 0;

    bool raw_pressed = !gpio_get(BUTTON_PIN);
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if (raw_pressed != last_raw_pressed) {
        last_raw_pressed = raw_pressed;
        last_change_ms = now_ms;
    }

    if ((now_ms - last_change_ms) >= BUTTON_DEBOUNCE_MS &&
        raw_pressed != debounced_pressed) {
        debounced_pressed = raw_pressed;
        if (debounced_pressed) {
            automatic_mode = !automatic_mode;
            gpio_put(MODE_LED_PIN, automatic_mode);
        }
    }
}

static int8_t accel_to_mouse_delta(int16_t accel, int sign) {
    int32_t value = (int32_t)accel * sign;

    if (value > 14000) {
        return 8;
    }
    if (value > 9500) {
        return 5;
    }
    if (value > 4500) {
        return 2;
    }
    if (value < -14000) {
        return -8;
    }
    if (value < -9500) {
        return -5;
    }
    if (value < -4500) {
        return -2;
    }

    return 0;
}

static void automatic_circle_delta(int8_t *dx, int8_t *dy) {
    static const int8_t circle[][2] = {
        {2, 0}, {2, 1}, {1, 2}, {0, 2}, {-1, 2}, {-2, 1},
        {-2, 0}, {-2, -1}, {-1, -2}, {0, -2}, {1, -2}, {2, -1},
    };
    static uint8_t index = 0;
    static uint8_t hold_count = 0;

    *dx = circle[index][0];
    *dy = circle[index][1];

    hold_count++;
    if (hold_count >= 8) {
        hold_count = 0;
        index = (uint8_t)((index + 1) % (sizeof(circle) / sizeof(circle[0])));
    }
}

static void hid_mouse_task(void) {
    static uint32_t last_report_ms = 0;
    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    if ((now_ms - last_report_ms) < MOUSE_INTERVAL_MS) {
        return;
    }
    last_report_ms = now_ms;

    if (!tud_hid_ready()) {
        return;
    }

    int8_t dx = 0;
    int8_t dy = 0;

    if (automatic_mode) {
        automatic_circle_delta(&dx, &dy);
    } else if (mpu_ready) {
        mpu6050_data_t data;
        if (mpu6050_read_data(&data) >= 0) {
            mpu_read_failures = 0;
            dx = accel_to_mouse_delta(data.ax, 1);
            dy = accel_to_mouse_delta(data.ay, -1);
        } else {
            if (mpu_read_failures < MPU_READ_FAILURE_LIMIT) {
                mpu_read_failures++;
            }
            if (mpu_read_failures >= MPU_READ_FAILURE_LIMIT) {
                mpu_ready = false;
            }
        }
    }

    tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, dx, dy, 0, 0);
}

static void retry_mpu_if_needed(void) {
    static uint32_t last_retry_ms = 0;

    if (mpu_ready || automatic_mode) {
        return;
    }

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - last_retry_ms) < MPU_RETRY_INTERVAL_MS) {
        return;
    }

    last_retry_ms = now_ms;
    mpu_ready = mpu6050_setup();
    if (mpu_ready) {
        mpu_read_failures = 0;
    }
}

int main(void) {
    stdio_init_all();
    i2c_bus_setup();
    button_and_led_setup();

    mpu_ready = mpu6050_setup();

    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    while (true) {
        tud_task();
        update_mode_button();
        retry_mpu_if_needed();
        hid_mouse_task();
    }
}

uint16_t tud_hid_get_report_cb(uint8_t instance,
                               uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer,
                               uint16_t reqlen) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

void tud_hid_set_report_cb(uint8_t instance,
                           uint8_t report_id,
                           hid_report_type_t report_type,
                           uint8_t const *buffer,
                           uint16_t bufsize) {
    (void)instance;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
