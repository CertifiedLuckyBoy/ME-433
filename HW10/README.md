# HW10

This folder contains the Pico code and the Pygame Zero code for HW10.

## Hardware

- MPU6050 `VCC` to Pico `3V3`
- MPU6050 `GND` to Pico `GND`
- MPU6050 `SDA` to Pico `GP4`
- MPU6050 `SCL` to Pico `GP5`
- MPU6050 `AD0` to `GND` or left at address `0x68`
- Pushbutton from Pico `GP15` to `GND`

The code uses Pico `i2c0`. If your breadboard uses different I2C pins, edit
`I2C_SDA_PIN` and `I2C_SCL_PIN` at the top of `HW10.c`.
The button uses the Pico internal pull-up, so it reads `1` when pressed.

## Pico Protocol

`HW10.c` streams one line at 60 Hz:

```text
(ax,ay,az,gx,gy,gz,button)
```

`ax`, `ay`, and `az` are raw accelerometer readings. `gx`, `gy`, and `gz` are
raw gyroscope readings. `button` is `0` or `1`.

## Python Game

The game uses `pgzero` and `pyserial`. On this computer Python is 3.14, so
`pygame-ce` is listed instead of building `pygame` from source.

```sh
python -m pip install --no-deps -r requirements.txt
python game.py
```

If the game does not automatically find the Pico serial port, pass it as an argument:

```sh
python game.py /dev/tty.usbmodem1101
```

Rotating the MPU6050 moves the crosshair like a mouse. Pressing the GP15 button
fires, like the left mouse button in a simple shooting game.

## Assignment Notes

The required Canvas video should show:

1. Serial monitor output from the Pico, like `(1234,-220,16100,10,-5,20,0)`.
2. The Pygame Zero crosshair moving when the MPU6050 is rotated.
3. The GP15 button firing and the score increasing when targets are hit.
