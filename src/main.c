#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

#include "sdkconfig.h" // generated by "make menuconfig"

#include "ssd1306.h"
#include "caliper.h"

int lev=0;
int last=0;
char *buf2;

void blink_task(void *pvParameter)
{
    while(1) {
		buf2 = take_reading();
		printf("buf: %s\n", buf2);
		//xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);
		//vTaskDelay(100/portTICK_PERIOD_MS);
		xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048, buf2, 6, NULL);
		//vTaskDelay(500/portTICK_PERIOD_MS);
		
    }
}


void app_main(void)
{
	i2c_master_init();
	ssd1306_init();

	init_caliper();

	//xTaskCreate(&task_ssd1306_display_pattern, "ssd1306_display_pattern",  2048, NULL, 6, NULL);
	xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);
	vTaskDelay(100/portTICK_PERIOD_MS);
	xTaskCreate(&task_ssd1306_display_text, "ssd1306_display_text",  2048, (void *)"beginning", 6, NULL);
	xTaskCreate(&task_ssd1306_display_clear, "ssd1306_display_clear",  2048, NULL, 6, NULL);
	vTaskDelay(100/portTICK_PERIOD_MS);
	//xTaskCreate(&task_ssd1306_contrast, "ssid1306_contrast", 2048, NULL, 6, NULL);
	//xTaskCreate(&task_ssd1306_scroll, "ssid1306_scroll", 2048, NULL, 6, NULL);
	//xTaskCreate(&task_caliper_test, "ssd1306_display_clear",  2048, NULL, 6, NULL);

	xTaskCreate(&blink_task, "blink_task",  2048, NULL, 6, NULL);
}