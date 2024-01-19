#include "BeerScreen.h"

void drawCalButton(Adafruit_ILI9341 *tft, int color){
  tft->fillRect(CAL_X, CAL_Y, BUTTON_W, BUTTON_H, color);
  tft->setTextColor(ILI9341_WHITE);
  tft->setTextSize(1);
  tft->setCursor(50, CAL_Y - 20);
  tft->println("Place 120lbs on scale,");
  tft->println("Then push 120 lbs button below");
  tft->setCursor(CAL_X + 10, CAL_Y + (BUTTON_H / 2));
  tft->setTextSize(2);
  tft->println("120 lbs");
}

void drawTareButton(Adafruit_ILI9341 *tft, int color){
  tft->fillRect(TARE_X, TARE_Y, BUTTON_W, BUTTON_H, color);  
  tft->setTextColor(ILI9341_WHITE);
  tft->setTextSize(1);
  tft->setCursor(50, TARE_Y - 20);
  tft->println("Clear your scale,");
  tft->println("Then push TARE button below");
  tft->setCursor(TARE_X + 25, TARE_Y + (BUTTON_H / 2));
  tft->setTextSize(2);
  tft->println("TARE");
}

void drawCancelButton(Adafruit_ILI9341 *tft){
  tft->fillRect(CANCEL_X, CANCEL_Y, BUTTON_W, BUTTON_H, ILI9341_RED);
  tft->setCursor(CANCEL_X + 15, CANCEL_Y + (BUTTON_H / 2));
  tft->setTextColor(ILI9341_WHITE);
  tft->setTextSize(2);
  tft->println("Cancel");
}

void drawSaveButton(Adafruit_ILI9341 *tft){
  tft->fillRect(SAVE_X, SAVE_Y, BUTTON_W, BUTTON_H, ILI9341_DARKGREEN);
  tft->setCursor(SAVE_X + 20, SAVE_Y + (BUTTON_H / 2));
  tft->setTextColor(ILI9341_WHITE);
  tft->setTextSize(2);
  tft->println("SAVE");
}

void draw_percent_2_GaugeY1(Adafruit_ILI9341 *tft, float percent) {
    int pixel_total = tft->height() - GAUGEy;
    int gaugeY1 = tft->height() - ((pixel_total * percent) / 100);
    int red = 255 - map(percent, 0, 100, 0, 255);
    int green = map(percent, 0, 100, 0, 255);

    // Draw placeholder/Background for Gauge
    tft->fillRect(GAUGEx, GAUGEy, GAUGE_W, gaugeY1, ILI9341_BLACK);
    // Draw filled portion
    tft->fillRect(GAUGEx, gaugeY1, GAUGE_W, tft->height() - gaugeY1, tft->color565(red, green, 0));
    // Draw text
    if (percent > 98) {
      tft->setCursor(GAUGEx + 2, gaugeY1);
    } else {
      tft->setCursor(GAUGEx + 2, gaugeY1 - 13);
    }
    tft->setTextColor(ILI9341_WHITE);
    tft->setTextSize(1);
    tft->print(percent,1);
    tft->print("%");
}

void updateScreenValues(Adafruit_ILI9341 *tft, BeerStatus_t status){
    // Clear out Status text and update values
    tft->fillRect(150, 0, tft->width() - 150, ROW_HEIGHT * 2, ILI9341_BLACK);

    // Display weight
    tft->setCursor(17, 6);
    tft->setTextColor(ILI9341_BLUE);
    tft->setTextSize(1);
    tft->print("Approx Beer Remaining: ");
    tft->print(status.weight_lbs, 2);
    tft->println(" lbs");

    // Display units
    tft->setCursor(5, ROW_HEIGHT);
    tft->setTextColor(ILI9341_BLUE);
    tft->print("Approx Drinks Remaining: ");
    tft->print(status.units_remain, 2);

    // Draw Percent level shape for Gauge
    draw_percent_2_GaugeY1(tft, status.level_percent);
}

void draw_calibration_screen(Adafruit_ILI9341 *tft){
  drawTareButton(tft);
  drawCalButton(tft);
  drawCancelButton(tft);
  drawSaveButton(tft);
}