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
#define PIN_NUM_MOSI 13  // SDA
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 15

#define PIN_NUM_DC 4
#define PIN_NUM_RST 2
#endif

// LCD backlight contorl
#define PIN_NUM_BCKL CONFIG_ST7735_BL_PIN

#ifdef CONFIG_USE_COLOR_RBG565 // R-B-G 5-6-5
// Some ready-made 16-bit (RBG-565) color settings:
#define	COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define	COLOR_RED        0xF800
#define	COLOR_GREEN      0x001F
#define	COLOR_BLUE       0x07E0
#define COLOR_CYAN       0x07FF
#define COLOR_MAGENTA    0xFFE0
#define COLOR_YELLOW     0xF81F
#define	COLOR_GRAY       0x8410
#define	COLOR_OLIVE      0x8011
#else // R-G-B 5-6-5
// Some ready-made 16-bit (RGB-565) color settings:
#define	COLOR_BLACK      0x0000
#define COLOR_WHITE      0xFFFF
#define	COLOR_RED        0xF800
#define	COLOR_GREEN      0x07E0
#define	COLOR_BLUE       0x001F
#define COLOR_CYAN       0x07FF
#define COLOR_MAGENTA    0xF81F
#define COLOR_YELLOW     0xFFE0
#define	COLOR_GRAY       0x8410
#define	COLOR_OLIVE      0x8400
#endif

void st7735_init();
void st7735_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void st7735_invert_color(int i);
void st7735_fill_screen(uint16_t color);
uint32_t st7735_draw_string(uint16_t x, uint16_t y, const char *pt, int16_t color, int16_t bg_color, uint8_t size);
void st7735_draw_char(int16_t x, int16_t y, char c, int16_t color, int16_t bg_color, uint8_t size);
