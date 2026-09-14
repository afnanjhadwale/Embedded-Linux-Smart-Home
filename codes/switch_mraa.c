#include <stdio.h>
#include <mraa.h>

#define SW_PIN 35
#define LED_PIN 61

int main() {
    mraa_init();

    mraa_gpio_context sw = mraa_gpio_init(SW_PIN);
    mraa_gpio_context led = mraa_gpio_init(LED_PIN);

    mraa_gpio_dir(sw, MRAA_GPIO_IN);
    mraa_gpio_dir(led, MRAA_GPIO_OUT);

    while (1) {
        if (mraa_gpio_read(sw) == 0) 
            mraa_gpio_write(led, 0);
        else
            mraa_gpio_write(led, 1); // Turn off the LED
	
    }
}
