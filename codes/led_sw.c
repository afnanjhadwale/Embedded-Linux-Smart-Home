#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>

#define GPIO_EN "/sys/class/gpio/export"

#define LED_DIR "/sys/class/gpio/PC13/direction"
#define LED_VAL "/sys/class/gpio/PC13/value"

#define SW_DIR "/sys/class/gpio/PC12/direction"
#define SW_VAL "/sys/class/gpio/PC12/value"

int led_init(int);
int sw_init(int);

int sw_fd,led_fd;

int main() {
  
  int sw_num = 76, led_num = 77;
  char sw_val;

  led_fd = led_init(led_num);
  sw_init(sw_num);
  
  while (1) 
  {
  
  //printf("Set the value into GPIO_Val \n");
  sw_fd = open(SW_VAL, O_RDONLY);
  if (sw_fd < 0) {
    printf("Unable to open the file %s", SW_VAL);
    exit(0);
  }

    read(sw_fd, &sw_val, 1);

    if(sw_val == '1'){
      write(led_fd, "1", 1);
      usleep(50);
    }
    else{
      write(led_fd, "0", 1);
      usleep(50);
    }
    usleep(5000);
    close(sw_fd);
    
  }
  return 0;
}

int led_init(int led_num){

  char led_buf[30];

  led_fd = open(GPIO_EN, O_WRONLY);

  if (led_fd < 0) {
    printf("Unable to open the file %s\n", GPIO_EN);
    exit(0);
  }

  //printf("Enable the GPIO");
  sprintf(led_buf, "%d", led_num);
  write(led_fd, led_buf, strlen(led_buf));
  close(led_fd);

  //printf("Configuring GPIO direction \n");
  led_fd = open(LED_DIR, O_WRONLY);

  if (led_fd < 0) {
    printf("Unable to open the file %s", LED_DIR);
    exit(0);
  }

  write(led_fd, "out", 3);
  close(led_fd);

  //printf("Set the value into GPIO_Val \n");

  led_fd = open(LED_VAL, O_WRONLY);
  if (led_fd < 0) {
    printf("Unable to open the file %s", LED_VAL);
    exit(0);
  }
  return led_fd;

}


int sw_init(int sw_num){

  char sw_buf[30];

  sw_fd = open(GPIO_EN, O_WRONLY);

  if (sw_fd < 0) {
    printf("Unable to open the file %s\n", GPIO_EN);
    exit(0);
  }

  //printf("Enable the GPIO");
  sprintf(sw_buf, "%d", sw_num);
  write(sw_fd, sw_buf, strlen(sw_buf));
  close(sw_fd);

  //printf("Configuring GPIO direction \n");
  sw_fd = open(SW_DIR, O_WRONLY);

  if (sw_fd < 0) {
    printf("Unable to open the file %s", SW_DIR);
    exit(0);
  }

  write(sw_fd, "in", 2);
  close(sw_fd);

}
