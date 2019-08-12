#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_timer.h"

#include "driver/gpio.h"


#include "caliper.h"

#define X_CAL_CLK 26
#define X_CAL_SDA 27

unsigned long start_time;
char *out, *tmp;

bool rel_data[23];
bool new_frame_flag;
unsigned long res;
char buf[20] = { 0 };

int read;

#define CONSUME_BIT() {while(gpio_get_level(X_CAL_CLK) == 0); while (gpio_get_level(X_CAL_CLK)==1);}

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void init_caliper() {
    gpio_pad_select_gpio(2);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    gpio_set_level(2, 1);

    gpio_pad_select_gpio(X_CAL_CLK);
    gpio_set_direction(X_CAL_CLK, GPIO_MODE_INPUT);
    //gpio_get_level(X_CAL_CLK);

        gpio_pad_select_gpio(X_CAL_SDA);
    gpio_set_direction(X_CAL_SDA, GPIO_MODE_INPUT);
    //gpio_get_level(X_CAL_SDA);
    //printf(gpio_get_level(GPIO_NUM_4));
    printf("%d-init..........\n", gpio_get_level(X_CAL_SDA));

}

char* decode() {
    // caliper sends two 23-bit signed values
    // want to read on rising edge, low on entry
    
    // timing is really tight here, 4us per transition
    portENTER_CRITICAL(&mux);

    CONSUME_BIT(); // discard first start bit
    
    for (int i = 0; i < 23; i++) { 
        CONSUME_BIT(); // abs data, just throw these out
    }

    CONSUME_BIT(); // discard second start bit

    for (int i = 0; i < 23; i++) { 
        while (gpio_get_level(X_CAL_CLK)==0) {} // read after rising edge

        rel_data[i] = gpio_get_level(X_CAL_SDA); // don't try to do anything else in here; even the left shift takes too long

        while (gpio_get_level(X_CAL_CLK)==1) {} // wait for falling edge
    }

    portEXIT_CRITICAL(&mux);


    res = 0;
    for (int i = 0; i < 23; i++) {
        res |= ((long) rel_data[i] << i);
    }

    if (res >> 22) { // check msb; take two's compliment if it's negative
        buf[0] = '-';
        res ^= 0x7FFFFF;
        res++;
    } else { 
        buf[0] = '+';
    }

    sprintf(buf+1, "%.4f", res / 10240.0); // leave the sign
    //dtostrf(res / 10240.0, 6, 4, buf+1); // leave the sign
    // printf("%.4f -- %s\n", res / 10240.0, buf);

    return buf;
}

char* take_reading() {
    new_frame_flag = false;

    while (new_frame_flag == false) {
        while (gpio_get_level(X_CAL_CLK)) {} // wait for rising edge
        
        start_time = esp_timer_get_time();
        
        while (gpio_get_level(X_CAL_CLK)) {} // wait for falling edge

        if ((esp_timer_get_time() - start_time) > 10000) { // new packet, ~300000 apart
            tmp = decode(); // do it before breaking out just incase the timight is tight
            // printf("%lld packet\n", esp_timer_get_time() - start_time);
            new_frame_flag = true;
        }
    }

    return tmp;
}


void task_caliper_test(void *ignore) {
    printf("CALIPER TEST\n\n");

	vTaskDelete(NULL);
}

