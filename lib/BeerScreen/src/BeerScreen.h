#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_FT6206.h>
#include <BeerScale.h>

// Constants
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define ROW_HEIGHT 20
#define GAUGE_W ROW_HEIGHT * 2
#define GAUGEx (TFT_WIDTH - GAUGE_W)
#define GAUGEy (ROW_HEIGHT * 2)
#define LOGOx1 0
#define LOGOy1 (ROW_HEIGHT * 2)
#define LOGO_WIDTH (TFT_WIDTH - ROW_HEIGHT * 2)
#define LOGO_HEIGHT (TFT_HEIGHT - ROW_HEIGHT * 2)

#define THUMB_W 66
#define THUMB_H 92

#define BUTTON_W 100
#define BUTTON_H 50

#define TARE_X 50
#define TARE_Y 90
#define CAL_X 50
#define CAL_Y 190
#define CANCEL_X 0
#define CANCEL_Y 270
#define SAVE_X 100
#define SAVE_Y 270

// Font
#define FONT_HEIGHT 8 //font size 1 = 6x8, font size 2 = 12x16

void drawCalButton(Adafruit_ILI9341 *tft, int color = ILI9341_LIGHTGREY);
void drawTareButton(Adafruit_ILI9341 *tft, int color = ILI9341_LIGHTGREY);
void updateScreenValues(Adafruit_ILI9341 *tft, BeerStatus_t status);
void draw_percent_2_GaugeY1(Adafruit_ILI9341 *tft, float percent);
void draw_calibration_screen(Adafruit_ILI9341 *tft);
void drawSaveButton(Adafruit_ILI9341 *tft);
void drawCancelButton(Adafruit_ILI9341 *tft);