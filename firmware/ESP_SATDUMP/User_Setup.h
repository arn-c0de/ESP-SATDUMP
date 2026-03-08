// TFT_eSPI User Setup — LAFVIN 4.0" ST7796S on ESP32
// Place this file in your TFT_eSPI library folder, or use
// User_Setup_Select.h to point at a custom path.

#define ST7796_DRIVER

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS    5
#define TFT_DC   16
#define TFT_RST  17
#define TFT_BL    4

#define TFT_BACKLIGHT_ON HIGH

#define TFT_WIDTH  320
#define TFT_HEIGHT 480

// Use SPI port 0 (VSPI)
//#define USE_HSPI_PORT   // comment out to use VSPI

#define SPI_FREQUENCY       27000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000

// Fonts
#define LOAD_GLCD   // Font 1. Original Adafruit 8 pixel font
#define LOAD_FONT2  // Font 2. Small 16 pixel high font
#define LOAD_FONT4  // Font 4. Medium 26 pixel high font
#define LOAD_FONT6  // Font 6. Large 48 pixel font
#define LOAD_FONT7  // Font 7. 7 segment 48 pixel font
#define LOAD_FONT8  // Font 8. Large 75 pixel font
#define LOAD_GFXFF  // FreeFonts
#define SMOOTH_FONT
