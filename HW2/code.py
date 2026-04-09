import time
import board
import pwmio

# Servo signal output pin
SERVO_PIN = board.GP15

# Standard RC servo timing: 50 Hz period (20 ms)
PWM_FREQUENCY = 50
MIN_PULSE_US = 1000.0
MAX_PULSE_US = 2000.0
MAX_ANGLE = 180.0

servo_pwm = pwmio.PWMOut(SERVO_PIN, frequency=PWM_FREQUENCY, duty_cycle=0)


def pulse_us_to_duty_cycle(pulse_us):
    # CircuitPython uses a 16-bit duty cycle value from 0 to 65535.
    period_us = 1_000_000.0 / PWM_FREQUENCY
    return int((pulse_us / period_us) * 65535)


def set_servo_angle(angle):
    if angle < 0.0:
        angle = 0.0
    if angle > MAX_ANGLE:
        angle = MAX_ANGLE

    # Convert 0 to 180 degrees into a pulse width from 1.0 to 2.0 ms.
    pulse_us = MIN_PULSE_US + (angle / MAX_ANGLE) * (MAX_PULSE_US - MIN_PULSE_US)
    servo_pwm.duty_cycle = pulse_us_to_duty_cycle(pulse_us)


while True:
    angle = 0.0
    while angle <= MAX_ANGLE:
        set_servo_angle(angle)
        time.sleep(0.05)
        angle += 5.0

    angle = MAX_ANGLE
    while angle >= 0.0:
        set_servo_angle(angle)
        time.sleep(0.05)
        angle -= 5.0
