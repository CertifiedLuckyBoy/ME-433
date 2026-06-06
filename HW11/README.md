# HW11: STM32 and Pico UART Bridge

This project connects a NUCLEO-C092RC and a Pico 2 W over a 115200 baud UART.
Each board also exposes a USB virtual serial port to the computer. Text entered
on either computer serial port is sent across the hardware UART and appears on
the other computer serial port.

## Wiring

Disconnect both USB cables before changing the wiring.

| NUCLEO-C092RC | Pico 2 W |
| --- | --- |
| A0 / PA0 / USART1_TX | GP1 / UART0_RX / physical pin 2 |
| A1 / PA1 / USART1_RX | GP0 / UART0_TX / physical pin 1 |
| GND | GND / physical pin 3 |

Do not connect 3.3 V, 5 V, or VBUS between the boards.

## STM32

- Board: NUCLEO-C092RC
- ST-LINK virtual COM: USART2 on PA2/PA3
- External UART: USART1 on PA0/PA1
- Baud rate: 115200, 8 data bits, no parity, 1 stop bit
- Main source: `Core/Src/main.c`
- CubeMX configuration: `HW11.ioc`

Open the project in STM32CubeIDE, build it, and run it through ST-LINK.

## Pico

- Board: Pico 2 W
- USB stdio enabled
- UART0 TX: GP0
- UART0 RX: GP1
- Baud rate: 115200
- Main source: `pico/pico_uart_bridge.c`

Build the `pico` directory with the Pico SDK and copy
`pico/build/pico_uart_bridge.uf2` to the Pico BOOTSEL drive.

## Demonstration

Open both USB virtual serial ports at 115200 baud:

- STM32: the USB serial device named `STM32 STLink`
- Pico: the USB serial device named `Pico`

Type text into the STM32 terminal and show that it appears in the Pico terminal.
Then type different text into the Pico terminal and show that it appears in the
STM32 terminal. Record both directions for the required submission video.
