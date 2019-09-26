#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "ascii_font.h"

#ifdef CONFIG_ST7735_HOST_VSPI
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 23  // SDA
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5

#define PIN_NUM_DC 21
#define PIN_NUM_RST 22
#elif CONFIG_ST7735_HOST_HSPI
#define PIN_NUM_MISO -1
#define PIN_NUM_MOSI 23  // SDA
#define PIN_NUM_CLK 18
#define PIN_NUM_CS 5

#define PIN_NUM_DC 21
#define PIN_NUM_RST 22
#endif

// LCD backlight contorl
#define PIN_NUM_BCKL CONFIG_ST7735_BL_PIN

// Some ready-made 16-bit (RGB-565) color settings:
#define	RGB565_BLACK      0x0000
#define RGB565_WHITE      0xFFFF
#define	RGB565_RED        0xF800
#define	RGB565_GREEN      0x07E0
#define	RGB565_BLUE       0x001F
#define RGB565_CYAN       0x07FF
#define RGB565_MAGENTA    0xF81F
#define RGB565_YELLOW     0xFFE0
#define	RGB565_ORANGE     0xFC00


void st7735_init();
void st7735_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7735_invert_color(int i);
void st7735_fill_screen(uint16_t color);
uint32_t st7735_draw_string(uint16_t x, uint16_t y, const char *pt, int16_t color, int16_t bg_color, uint8_t size);
void st7735_draw_char(int16_t x, int16_t y, char c, int16_t color, int16_t bg_color, uint8_t size);
