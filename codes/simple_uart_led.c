#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <mraa.h>

#define LED_PIN 61

int main()
{
    // Initialize LED
    mraa_gpio_context led = mraa_gpio_init(LED_PIN);
    mraa_gpio_dir(led, MRAA_GPIO_OUT);
    mraa_gpio_write(led, 1);

    // Initialize UART
    mraa_uart_context uart = mraa_uart_init_raw("/dev/ttyS3");
    mraa_uart_set_baudrate(uart, 9600);

    char buffer[100] = {0};

    printf("Type N to turn LED ON, F to turn LED OFF\n");

    while (1) {
        // Get input from PC keyboard
        fgets(buffer, sizeof(buffer), stdin);

        // Send through UART TX (loopback — TX shorted to RX)
        mraa_uart_write(uart, buffer, strlen(buffer));

        usleep(10000);  // wait for loopback

        // Read back from UART RX
        memset(buffer, 0, sizeof(buffer));
        int n = mraa_uart_read(uart, buffer, sizeof(buffer));

        if (n > 0) {
            printf("Received: %s\n", buffer);

            if (buffer[0] == 'N' || buffer[0] == 'n') {
                mraa_gpio_write(led, 0);
                printf("LED ON\n");
            } else if (buffer[0] == 'F' || buffer[0] == 'f') {
                mraa_gpio_write(led, 1);
                printf("LED OFF\n");
            } else {
                printf("Unknown command. Use N or F\n");
            }
        } else {
            printf("Nothing received — check TX and RX pins are shorted\n");
        }
    }
}
