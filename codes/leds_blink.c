/*********************************************************************************************
 * © 2022 PHYTEC EMBEDDED PVT LTD. - All Rights Reserved. Permission to use, modify, copy, and distribute
 * this source code, object code, or executable code (collectively, Software), is granted only
 * under the terms of a valid written license agreement with PHYTEC. Unauthorized copying
 * or other use of the Software is strictly prohibited.  Software is owned by and constitutes
 * the proprietary works, trade secrets, and copyrights of Embitel or its licensors.
 *
 * For further information, contact PHYTEC EMBEDDED Pvt Ltd.
 *********************************************************************************************/
/*********************************************************************************************/
/*      Application: Led_Blink_GPIO.c
 *
 *      Brief: This application is to blink user Led connected to PC13 
 *
 *      Author: PHYTEC EMBEDDED PVT LTD
 */
/*********************************************************************************************/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>	
#include<stdio.h>
#include<string.h>  
#include<stdlib.h>

#define GPIO_EN  "/sys/class/gpio/export"

#define LED1_DIR "/sys/class/gpio/PC13/direction"
#define LED1_Val "/sys/class/gpio/PC13/value"

#define LED2_DIR "/sys/class/gpio/PC17/direction"
#define LED2_Val "/sys/class/gpio/PC17/value"

#define LED3_DIR "/sys/class/gpio/PC19/direction"
#define LED3_Val "/sys/class/gpio/PC19/value"


int  led1_fd,led2_fd,led3_fd;
char gpio_buf[30];
int  led1_num = 77,led2_num = 81, led3_num = 83;


void leds_init()
{
	//led1 export
	led1_fd = open(GPIO_EN,O_WRONLY);
	if(led1_fd < 0)
	{
		printf("Unable to open the file  %s\n",GPIO_EN);
		exit(0);
	}
	printf("Enable LED1\n");
	sprintf(gpio_buf,"%d",led1_num);
	write(led1_fd,gpio_buf,strlen(gpio_buf));
	close(led1_fd);
	
	//led2 export
	led2_fd = open(GPIO_EN,O_WRONLY);
	if(led2_fd < 0)
	{
		printf("Unable to open the file  %s\n",GPIO_EN);
		exit(0);
	}
	printf("Enable LED2\n");
	sprintf(gpio_buf,"%d",led2_num);
	write(led2_fd,gpio_buf,strlen(gpio_buf));
	close(led2_fd);
	
	//led3 export
	led3_fd = open(GPIO_EN,O_WRONLY);
	if(led3_fd < 0)
	{
		printf("Unable to open the file  %s\n",GPIO_EN);
		exit(0);
	}
	printf("Enable LED3\n");
	sprintf(gpio_buf,"%d",led3_num);
	write(led3_fd,gpio_buf,strlen(gpio_buf));
	close(led3_fd);
	
	//led1 direction
	printf("Configuring LED1 direction  \n");
	led1_fd = open(LED1_DIR,O_WRONLY);
	if(led1_fd < 0)
	{
		printf("Unable to open the file   %s",LED1_DIR);
		exit(0);
	}

	write(led1_fd,"out",3);
	close(led1_fd);
	
	//led2 direction
	printf("Configuring LED2 direction  \n");
	led2_fd = open(LED2_DIR,O_WRONLY);
	if(led2_fd < 0)
	{
		printf("Unable to open the file   %s",LED2_DIR);
		exit(0);
	}

	write(led2_fd,"out",3);
	close(led2_fd);
	
	//led3 direction
	printf("Configuring LED3 direction  \n");
	led3_fd = open(LED3_DIR,O_WRONLY);
	if(led3_fd < 0)
	{
		printf("Unable to open the file   %s",LED3_DIR);
		exit(0);
	}

	write(led3_fd,"out",3);
	close(led3_fd);
	
	//LED1 value
	printf("Set the value into LED1_Val \n");

	led1_fd = open( LED1_Val,O_WRONLY);
	if(led1_fd < 0)
	{
		printf("Unable to open the file   %s",LED1_Val);
		exit(0);
	}
	
	//LED2 value
	printf("Set the value into LED2_Val \n");

	led2_fd = open( LED2_Val,O_WRONLY);
	if(led2_fd < 0)
	{
		printf("Unable to open the file   %s",LED2_Val);
		exit(0);
	}
	
	//LED3 value
	printf("Set the value into LED3_Val \n");

	led3_fd = open( LED3_Val,O_WRONLY);
	if(led3_fd < 0)
	{
		printf("Unable to open the file   %s",LED3_Val);
		exit(0);
	}
	
	
}


int main()
{	int count=20;
	leds_init();
	
	printf("Toggling the LEDs! \n");
	while(count--)
	{
		write(led1_fd,"0" ,1);
		write(led2_fd,"0" ,1);
		write(led3_fd,"0" ,1);
		usleep(500000);
		write(led1_fd,"1" ,1);
		write(led2_fd,"1" ,1);
		write(led3_fd,"1" ,1);
		usleep(500000);
	}
	close(led1_fd);
	close(led2_fd);
	close(led3_fd);
}







