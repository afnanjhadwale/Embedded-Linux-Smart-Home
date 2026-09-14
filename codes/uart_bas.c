#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mraa.h>

#define RFID_UART_PORT "/dev/ttyS3"

int main() {
    mraa_init();
    char buffer[100]={0};
    mraa_uart_context uart = mraa_uart_init_raw(RFID_UART_PORT);

    while (1) {
    	
    	fgets(buffer,sizeof(buffer),stdin);
        /* send data through UART */
        mraa_uart_write(uart,buffer,sizeof(buffer));

       usleep(10000);
        mraa_uart_read(uart, buffer,sizeof(buffer));
	printf("%s\n",buffer);
  

        
    }
}

