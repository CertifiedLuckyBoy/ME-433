#include <stdio.h>

#include "hardware/uart.h"
#include "pico/stdlib.h"

#define UART_PORT uart0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#define UART_BAUD_RATE 115200

int main(void)
{
    stdio_init_all();

    uart_init(UART_PORT, UART_BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    while (!stdio_usb_connected()) {
        sleep_ms(10);
    }

    printf("Pico UART bridge ready\r\n");

    while (true) {
        int pc_byte;
        while ((pc_byte = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
            uart_putc_raw(UART_PORT, (uint8_t)pc_byte);
        }

        while (uart_is_readable(UART_PORT)) {
            putchar_raw(uart_getc(UART_PORT));
        }
        stdio_flush();

        tight_loop_contents();
    }
}
