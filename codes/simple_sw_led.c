#include <stdio.h>
#include <unistd.h>
#include <mraa.h>

#define SW_PIN  12
#define LED_PIN 13

int main() {
    mraa_init();

    mraa_gpio_context sw  = mraa_gpio_init(SW_PIN);
    mraa_gpio_context led = mraa_gpio_init(LED_PIN);

    mraa_gpio_dir(sw,  MRAA_GPIO_IN);
    mraa_gpio_dir(led, MRAA_GPIO_OUT);

    mraa_gpio_mode(sw, MRAA_GPIO_PULLUP);

    while (1) {
        if (mraa_gpio_read(sw) == 0)
            mraa_gpio_write(led, 1);  // Switch pressed  → LED ON
        else
            mraa_gpio_write(led, 0);  // Switch released → LED OFF

        usleep(50000);
    }

    mraa_gpio_close(sw);
    mraa_gpio_close(led);

    return 0;
}
