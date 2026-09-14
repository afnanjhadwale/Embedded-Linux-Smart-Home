#include <unistd.h>
#include <mraa/gpio.h>

int main() 
{
    mraa_gpio_context led1;
    led1 = mraa_gpio_init(61);
    mraa_gpio_dir(led1, MRAA_GPIO_OUT);
    while (1) 
    {
        mraa_gpio_write(led1, 0);
        usleep(500000);
        mraa_gpio_write(led1, 1);
	usleep(500000);
    }
}

