#include "pico/stdlib.h"
#include "hardware/pwm.h"

// Servo signal output pin
#define SERVO_PIN 15

// Standard RC servo timing: 50 Hz period (20 ms)
#define SERVO_PWM_DIVIDER 125.0f
#define SERVO_PWM_WRAP 19999
#define SERVO_MIN_PULSE_US 1000.0f
#define SERVO_MAX_PULSE_US 2000.0f
#define SERVO_MAX_ANGLE 180.0f

static void servo_init(void) {
    gpio_set_function(SERVO_PIN, GPIO_FUNC_PWM);

    uint slice_num = pwm_gpio_to_slice_num(SERVO_PIN);
    pwm_config config = pwm_get_default_config();

    // 125 MHz / 125 = 1 MHz, so each PWM count is 1 microsecond.
    pwm_config_set_clkdiv(&config, SERVO_PWM_DIVIDER);
    pwm_config_set_wrap(&config, SERVO_PWM_WRAP);
    pwm_init(slice_num, &config, true);
}

void set_servo_angle(float angle) {
    if (angle < 0.0f) {
        angle = 0.0f;
    }
    if (angle > SERVO_MAX_ANGLE) {
        angle = SERVO_MAX_ANGLE;
    }

    // Convert 0 to 180 degrees into a pulse width from 1.0 to 2.0 ms.
    float pulse_us = SERVO_MIN_PULSE_US +
                     (angle / SERVO_MAX_ANGLE) * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US);

    pwm_set_gpio_level(SERVO_PIN, (uint16_t)pulse_us);
}

int main() {
    servo_init();

    while (true) {
        for (float angle = 0.0f; angle <= SERVO_MAX_ANGLE; angle += 5.0f) {
            set_servo_angle(angle);
            sleep_ms(50);
        }

        for (float angle = SERVO_MAX_ANGLE; angle >= 0.0f; angle -= 5.0f) {
            set_servo_angle(angle);
            sleep_ms(50);
        }
    }
}
