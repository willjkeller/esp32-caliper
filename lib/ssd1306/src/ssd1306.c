#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/task.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define SDA_PIN 18
#define SCL_PIN 19

#define tag "SSD1306"

// https://github.com/yanbe/ssd1306-esp-idf-i2c/blob/master/main/main.c

void i2c_master_init()
{
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = SDA_PIN;
	conf.scl_io_num = SCL_PIN;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	i2c_param_config(I2C_NUM_0, &conf);
	i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0);
}

void ssd1306_init() {
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_CHARGE_PUMP, true);
	i2c_master_write_byte(cmd, 0x14, true);

	i2c_master_write_byte(cmd, OLED_CMD_SET_SEGMENT_REMAP, true); // reverse left-right mapping
	i2c_master_write_byte(cmd, OLED_CMD_SET_COM_SCAN_MODE, true); // reverse up-bottom mapping

	i2c_master_write_byte(cmd, OLED_CMD_DISPLAY_ON, true);
	i2c_master_stop(cmd);

	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		ESP_LOGI(tag, "OLED configured successfully");
	} else {
		ESP_LOGE(tag, "OLED configuration failed. code: 0x%.2X", espRc);
	}
	i2c_cmd_link_delete(cmd);
}

void task_ssd1306_display_pattern(void *ignore) {
	i2c_cmd_handle_t cmd;

	for (uint8_t i = 0; i < 8; i++) {
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
		i2c_master_write_byte(cmd, 0xB0 | i, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		for (uint8_t j = 0; j < 128; j++) {
			i2c_master_write_byte(cmd, 0xFF >> (j % 8), true);
		}
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

	vTaskDelete(NULL);
}

void task_ssd1306_display_clear(void *ignore) {
	i2c_cmd_handle_t cmd;

	uint8_t zero[128] = { 0 };
	for (uint8_t i = 0; i < 8; i++) {
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_SINGLE, true);
		i2c_master_write_byte(cmd, 0xB0 | i, true);

		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
		i2c_master_write(cmd, zero, 128, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	}

	vTaskDelete(NULL);
}


void task_ssd1306_contrast(void *ignore) {
	i2c_cmd_handle_t cmd;

	uint8_t contrast = 0;
	uint8_t direction = 1;
	while (true) {
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
		i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
		i2c_master_write_byte(cmd, OLED_CMD_SET_CONTRAST, true);
		i2c_master_write_byte(cmd, contrast, true);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
		vTaskDelay(1/portTICK_PERIOD_MS);

		contrast += direction;
		if (contrast == 0xFF) { direction = -1; }
		if (contrast == 0x0) { direction = 1; }
	}
	vTaskDelete(NULL);
}

void task_ssd1306_scroll(void *ignore) {
	esp_err_t espRc;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);

	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);

	i2c_master_write_byte(cmd, 0x29, true); // vertical and horizontal scroll (p29)
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_write_byte(cmd, 0x00, true);
	i2c_master_write_byte(cmd, 0x07, true);
	i2c_master_write_byte(cmd, 0x01, true);
	i2c_master_write_byte(cmd, 0x3F, true);

	i2c_master_write_byte(cmd, 0xA3, true); // set vertical scroll area (p30)
	i2c_master_write_byte(cmd, 0x20, true);
	i2c_master_write_byte(cmd, 0x40, true);

	i2c_master_write_byte(cmd, 0x2F, true); // activate scroll (p29)

	i2c_master_stop(cmd);
	espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	if (espRc == ESP_OK) {
		ESP_LOGI(tag, "Scroll command succeeded");
	} else {
		ESP_LOGE(tag, "Scroll command failed. code: 0x%.2X", espRc);
	}

	i2c_cmd_link_delete(cmd);

	vTaskDelete(NULL);
}

void task_ssd1306_display_text(const void *arg_text) {
	char *text = (char*)arg_text;
	uint8_t text_len = strlen(text);

	i2c_cmd_handle_t cmd;

	uint8_t cur_page = 0;

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

	i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
	i2c_master_write_byte(cmd, 0x00, true); // reset column
	i2c_master_write_byte(cmd, 0x10, true);
	i2c_master_write_byte(cmd, 0xB0 | cur_page, true); // reset page

	i2c_master_stop(cmd);
	i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	for (uint8_t i = 0; i < text_len; i++) {
		if (text[i] == '\n') {
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_CMD_STREAM, true);
			i2c_master_write_byte(cmd, 0x00, true); // reset column
			i2c_master_write_byte(cmd, 0x10, true);
			i2c_master_write_byte(cmd, 0xB0 | ++cur_page, true); // increment page

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		} else {
			cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, (OLED_I2C_ADDRESS << 1) | I2C_MASTER_WRITE, true);

			i2c_master_write_byte(cmd, OLED_CONTROL_BYTE_DATA_STREAM, true);
			i2c_master_write(cmd, font8x8_basic_tr[(uint8_t)text[i]], 8, true);

			i2c_master_stop(cmd);
			i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
		}
	}

	vTaskDelete(NULL);
}