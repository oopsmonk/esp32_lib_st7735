
#include "ST7735.h"

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
  uint8_t cmd;
  uint8_t data[16];
  uint8_t databytes;  // No of data in data, 0xFF = end of cmds
} lcd_init_cmd_t;

static spi_device_handle_t spi_dev;

#define ST7735_TFTWIDTH_128 128   // for 1.44 and mini
#define ST7735_TFTWIDTH_80 80     // for mini
#define ST7735_TFTHEIGHT_128 128  // for 1.44" display
#define ST7735_TFTHEIGHT_160 160  // for 1.8" and mini display

#define LCD_WIDTH 128
#define LCD_HEIGHT 160

// ST77XX commands
#define ST_CMD_DELAY 0x80  // special signifier for command lists

#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

#define ST77XX_RDID1 0xDA
#define ST77XX_RDID2 0xDB
#define ST77XX_RDID3 0xDC
#define ST77XX_RDID4 0xDD

// ST7735 commands
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH 0x04

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR 0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1 0xC0
#define ST7735_PWCTR2 0xC1
#define ST7735_PWCTR3 0xC2
#define ST7735_PWCTR4 0xC3
#define ST7735_PWCTR5 0xC4
#define ST7735_VMCTR1 0xC5

#define ST7735_PWCTR6 0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// RGB-565 16bit, 128*160;
static uint8_t display_buff[LCD_WIDTH * LCD_HEIGHT * 2];

DRAM_ATTR static const lcd_init_cmd_t st7735_init_cmds[] = {
    // software reset with delay
    {ST77XX_SWRESET, {0}, ST_CMD_DELAY},
    // Out of sleep mode with delay
    {ST77XX_SLPOUT, {0}, ST_CMD_DELAY},
    // Framerate ctrl - normal mode. Rate = fosc/(1x2+40) * (LINE+2C+2D)
    {ST7735_FRMCTR1, {0x01, 0x2C, 0x2D}, 3},
    // Framerate ctrl - idle mode.  Rate = fosc/(1x2+40) * (LINE+2C+2D)
    {ST7735_FRMCTR2, {0x01, 0x2C, 0x2D}, 3},
    // Framerate - partial mode. Dot/Line inversion mode
    {ST7735_FRMCTR3, {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}, 6},
    // Display inversion ctrl: No inversion
    {ST7735_INVCTR, {0x07}, 1},
    // Power control1 set GVDD: -4.6V, AUTO mode.
    {ST7735_PWCTR1, {0xA2, 0x02, 0x84}, 3},
    // Power control2 set VGH/VGL: VGH25=2.4C VGSEL=-10 VGH=3 * AVDD
    {ST7735_PWCTR2, {0xC5}, 1},
    // Power control3 normal mode(Full color): Op-amp current small, booster voltage
    {ST7735_PWCTR3, {0x0A, 0x00}, 2},
    // Power control4 idle mode(8-colors): Op-amp current small & medium low
    {ST7735_PWCTR4, {0x8A, 0x2A}, 2},
    // Power control5 partial mode + full colors
    {ST7735_PWCTR5, {0x8A, 0xEE}, 2},
    // VCOMH VoltageVCOM control 1: VCOMH=0x0E=2.850
    {ST7735_VMCTR1, {0x0E}, 1},
    // Display Inversion Off
    {ST77XX_INVOFF, {0}, 0},
    // Memory Data Access Control: top-bottom/left-right refresh
    {ST77XX_MADCTL, {0xC8}, 1},
    // Color mode, Interface Pixel Format: RGB-565, 16-bit/pixel
    {ST77XX_COLMOD, {0x05}, 1},

    // Column Address Set: 2, 127+2
    {ST77XX_CASET, {0x00, 0x02, 0x00, 0x7F + 0x02}, 4},
    // Row Address Set: 1,159+1
    {ST77XX_RASET, {0x00, 0x01, 0x00, 0x9F + 0x01}, 4},

    // Gamma Adjustments (pos. polarity). Not entirely necessary, but provides accurate colors.
    {ST7735_GMCTRP1,
     {0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10},
     16},
    // Gamma Adjustments (neg. polarity). Not entirely necessary, but provides accurate colors.
    {ST7735_GMCTRN1,
     {0x03, 0x1d, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10},
     16},
    // Normal Display Mode On
    {ST77XX_NORON, {0}, ST_CMD_DELAY},
    // Display On
    {ST77XX_DISPON, {0}, ST_CMD_DELAY},
    {0, {0}, 0xFF},
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void st7735_cmd(const uint8_t cmd) {
  esp_err_t ret;
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));                        // Zero out the transaction
  t.length = 8;                                    // Command is 8 bits
  t.tx_buffer = &cmd;                              // The data is the cmd itself
  t.user = (void *)0;                              // D/C needs to be set to 0
  ret = spi_device_polling_transmit(spi_dev, &t);  // Transmit!
  assert(ret == ESP_OK);                           // Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void st7735_data(const uint8_t *data, int len) {
  esp_err_t ret;
  spi_transaction_t t;
  if (len == 0) return;                            // no need to send anything
  memset(&t, 0, sizeof(t));                        // Zero out the transaction
  t.length = len * 8;                              // Len is in bytes, transaction length is in bits.
  t.tx_buffer = data;                              // Data
  t.user = (void *)1;                              // D/C needs to be set to 1
  ret = spi_device_polling_transmit(spi_dev, &t);  // Transmit!
  assert(ret == ESP_OK);                           // Should have had no issues.
}

// This function is called (in irq context!) just before a transmission starts. It will
// set the D/C line to the value indicated in the user field.
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
  int dc = (int)t->user;
  gpio_set_level(PIN_NUM_DC, dc);
}

static void st7735_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
  uint8_t data[4];
  st7735_cmd(ST77XX_CASET);
  data[0] = 0x00;
  data[1] = x0 + 0x02;
  data[2] = 0x00;
  data[3] = x1 + 0x02;
  st7735_data(data, 4);

  st7735_cmd(ST77XX_RASET);
  data[0] = 0x00;
  data[1] = y0 + 0x01;
  data[2] = 0x00;
  data[3] = y1 + 0x01;
  st7735_data(data, 4);

  // memory write
  st7735_cmd(ST77XX_RAMWR);
}

void st7735_fill_screen(uint16_t color) {
  for (int i = 0; i < (LCD_WIDTH * LCD_HEIGHT * 2); i = i + 2) {
    display_buff[i] = color & 0xFF;
    display_buff[i + 1] = color >> 8;
  }

  st7735_set_address_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
  st7735_data(display_buff, LCD_WIDTH * LCD_HEIGHT * 2);
}

void st7735_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  // rudimentary clipping (drawChar w/big text requires this)
  if ((x >= LCD_WIDTH) || (y >= LCD_HEIGHT)) return;
  if ((x + w - 1) >= LCD_WIDTH) w = LCD_WIDTH - x;
  if ((y + h - 1) >= LCD_HEIGHT) h = LCD_HEIGHT - y;

  st7735_set_address_window(x, y, x + w - 1, y + h - 1);

  st7735_cmd(ST77XX_RAMWR);

  for (int i = 0; i < (w * h * 2); i++) {
    display_buff[i] = color & 0xFF;
    display_buff[i + 1] = color >> 8;
  }
  st7735_data(display_buff, w * h * 2);
}

void st7735_invert_color(int i) {
  if (i) {
    st7735_cmd(ST77XX_INVON);
  } else {
    st7735_cmd(ST77XX_INVOFF);
  }
}

void st7735_init() {
  esp_err_t ret;
  // esp32 spi configuration
  spi_bus_config_t buscfg = {.miso_io_num = PIN_NUM_MISO,
                             .mosi_io_num = PIN_NUM_MOSI,
                             .sclk_io_num = PIN_NUM_CLK,
                             .quadwp_io_num = -1,  // unused
                             .quadhd_io_num = -1,  // unused
                             .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * 2};

  spi_device_interface_config_t devcfg = {
      .clock_speed_hz = 10 * 1000 * 1000,       // Clock out at 10 MHz
      .mode = 0,                                // SPI mode 0
      .spics_io_num = PIN_NUM_CS,               // CS pin
      .queue_size = 7,                          // We want to be able to queue 7 transactions at a time
      .pre_cb = lcd_spi_pre_transfer_callback,  // Specify pre-transfer callback to handle D/C line
  };

  spi_host_device_t spi_host = VSPI_HOST;
#ifdef CONFIG_ST7735_HOST_HSPI
  spi_host = HSPI_HOST;
#endif

  // Initialize the SPI bus
  ret = spi_bus_initialize(spi_host, &buscfg, 1);
  ESP_ERROR_CHECK(ret);
  // Attach the LCD to the SPI bus
  ret = spi_bus_add_device(spi_host, &devcfg, &spi_dev);
  ESP_ERROR_CHECK(ret);

  // Initialize non-SPI GPIOs
  gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
  gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);
  gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_OUTPUT);

  // Reset the display
  gpio_set_level(PIN_NUM_RST, 0);
  vTaskDelay(500 / portTICK_RATE_MS);
  gpio_set_level(PIN_NUM_RST, 1);
  vTaskDelay(500 / portTICK_RATE_MS);

  // Send all the init commands
  int cmd = 0;
  while (st7735_init_cmds[cmd].databytes != 0xff) {
    st7735_cmd(st7735_init_cmds[cmd].cmd);
    st7735_data(st7735_init_cmds[cmd].data, st7735_init_cmds[cmd].databytes & 0x1F);
    if (st7735_init_cmds[cmd].databytes & ST_CMD_DELAY) {
      vTaskDelay(100 / portTICK_RATE_MS);
    }
    cmd++;
  }

  /// Enable backlight
  gpio_set_level(PIN_NUM_BCKL, 1);
}

void st7735_draw_pixel(int16_t x, int16_t y, uint16_t color) {
  if ((x < 0) || (x >= LCD_WIDTH) || (y < 0) || (y >= LCD_HEIGHT)) return;

  display_buff[0] = color & 0xFF;
  display_buff[1] = color >> 8;
  st7735_set_address_window(x, y, 1, 1);
  st7735_data(display_buff, 2);
}

void st7735_draw_char(int16_t x, int16_t y, char c, int16_t color, int16_t bg_color, uint8_t size) {
  if ((x >= LCD_WIDTH) ||          // Clip right
      (y >= LCD_HEIGHT) ||         // Clip bottom
      ((x + 6 * size - 1) < 0) ||  // Clip left
      ((y + 8 * size - 1) < 0))    // Clip top
    return;

  for (int8_t i = 0; i < 5; i++) {  // Char bitmap = 5 columns
    uint8_t line = std_font[c * 5 + i];
    for (int8_t j = 0; j < 8; j++, line >>= 1) {
      if (line & 1) {
        if (size == 1) {
          st7735_draw_pixel(x + i, y + j, color);
        } else {
          st7735_rect(x + (i * size), y + (j * size), size, size, color);
        }
      } else if (bg_color != color) {
        if (size == 1) {
          st7735_draw_pixel(x + i, y + j, bg_color);
        } else {
          st7735_rect(x + (i * size), y + (j * size), size, size, bg_color);
        }
      }
    }
  }
}

uint32_t st7735_draw_string(uint16_t x, uint16_t y, const char *pt, int16_t color, int16_t bg_color, uint8_t size) {
  // check row and colume
  uint32_t x_offset = 5 + 1, y_offset = 7;  // font size 5x7.

  uint32_t count = 0;
  if (y > 15) return 0;
  while (*pt) {
    st7735_draw_char(x * x_offset, y * y_offset, *pt, color, bg_color, size);
    pt++;
    x = x + size;
    if (x > 20) return count;  // number of characters printed
    count++;
  }
  return count;  // number of characters printed
}
