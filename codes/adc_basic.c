#include <stdlib.h>
#include <unistd.h>

/* mraa header */
#include "mraa.h"

/* AIO port */
#define AIO_PORT 6


int main()
{
    mraa_init();
    mraa_aio_context aio;
    uint16_t value = 0;
    

    /* initialize AIO */
    aio = mraa_aio_init(AIO_PORT);

    while (1) {
        value = mraa_aio_read(aio);
        printf("ADC Value :%d\n",value);
        sleep(2);
    }
}
