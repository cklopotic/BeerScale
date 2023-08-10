/*
  //    Setup using PlatformIO in VS Code (ESP32-S3-DevKitC in Arduino Mode)
  // 
  //****  ADDITIONAL Libraries TO INSTALL...........
  //      - "Adafruit ILI9341" (for the display). Install additional libraries when prompted.
  //      - "Adafruit ImageReader Library" Install additional libarires when prompted.
  //      - "HX711" by Rob Tillaart
  //      - Adafruit FT6206 (for the capacitive touchscreen)
  //
*/

#include <EEPROM.h>
#include <HX711.h>
#include <BeerScale.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_FT6206.h>
#include "ImageData.h"


//******   Scale Setup   *****
BeerScale beerScale;
//HX711 Scale circuit wiring
const uint8_t SCALE_DOUT_PIN = 46;
const uint8_t SCALE_SCK_PIN = 3;

// Scale Adjustment settings
long scaleOffset; 
float scaleDivider;
const uint16_t KNOWN_WEIGHT_LBS = 120;  //used for scale calibration

KegSize_t halfBarrel = {
  .emptyWeight = 33.0,
  .fullWeight = 160.0
};

KegSize_t quarterBarrel = {
  .emptyWeight = 22.0,
  .fullWeight = 87.0
};

KegSize_t activeKegSize;
BeerStatus_t status;

// Configuration for CS and DC pins
#define TFT_CS   10
#define TFT_DC   14
#define TFT_RST  18

// Config for display baudrate (default max is 24mhz)
#define SPI_FREQUENCY  24000000

// SD Card Setup
#define SD_CS    16
SdFat SD;         // SD card filesystem
Adafruit_ImageReader reader(SD); // Image-reader object, pass in SD filesys

// Create the display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// The FT6206 uses hardware I2C (SCL/SDA) AKA: the touchscreen
Adafruit_FT6206 ts = Adafruit_FT6206();

// Constants
#define ROW_HEIGHT 20
#define GAUGE_W ROW_HEIGHT * 2
#define GAUGEx (tft.width() - GAUGE_W)
#define GAUGEy (ROW_HEIGHT * 2)
#define LOGOx1 0
#define LOGOy1 (ROW_HEIGHT * 2)
#define LOGO_WIDTH (tft.width() - ROW_HEIGHT * 2)
#define LOGO_HEIGHT (tft.height() - ROW_HEIGHT * 2)

// Font
#define FONT_HEIGHT 8 //font size 1 = 6x8, font size 2 = 12x16


//*****   Memory Setup   ******
#define EEPROM_SIZE 32 //number of bytes needed (long data type is 4 bytes)
                      // sizeof(scaleOffset) + sizeof(scaleDivider) + logoIndex
int eep_add_offset = 0;
int eep_add_divider = 8;
int eep_add_logoIndex = 16;

enum screenStates { BLANK, LOGO, CHANGE_LOGO, CALIBRATE_SCALE } displayScreen = BLANK;


// Function prototypes
void saveScaleParams();
void readScaleParams();
void updateBackground();
void draw_percent_2_GaugeY1(int percent);
void changeBeerLogo();
void clearBeerLogo();
void loadBeerLogo();
void paintCalScale();
void takeReading();
int getThumbnailIndexFromTouch(TS_Point p);
void drawTareButton(int color = ILI9341_LIGHTGREY);
void drawCalButton(int color = ILI9341_LIGHTGREY);

// Main loop non-blocking delay routine variables
unsigned long start_time; 
unsigned long timed_event;
unsigned long current_time;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);

  // Setup SPI bus using hardware SPI
  SPI.begin();
  tft.begin(SPI_FREQUENCY);
  tft.setRotation(0);  // Set rotation to 0 degrees

  // Read saved settings
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(eep_add_logoIndex, logoIndex);
  constrain(logoIndex,1,9);
  readScaleParams();

  activeKegSize = halfBarrel;
  
  // Setup SD card
  if(!SD.begin(SD_CS, SD_SCK_MHZ(25))) { // ESP32 requires 25 MHz limit
    Serial.println("SD begin() failed");
    //for(;;); // Fatal error, do not continue
  }

  // Initialize and setup the scale
  Serial.println("Initializing the scale");
  beerScale.init(SCALE_DOUT_PIN, SCALE_SCK_PIN);
  beerScale.set_offset(scaleOffset);
  beerScale.set_scale(scaleDivider);


  // Setup Touch Screen
  if (!ts.begin(40)) { 
    Serial.println("Unable to start touchscreen.");
  } 
  else { 
    Serial.println("Touchscreen started."); 
  }

  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);
  // Load logo image
  loadBeerLogo();
  
  timed_event = 1000; // after 1000 ms trigger the event
  current_time = millis();
  start_time = current_time;
}

// the loop function runs over and over again forever
void loop() {

  current_time = millis(); // update the timer every cycle
  
  //Look for touches on screen
  if (ts.touched())
  {   
    // Retrieve a point  
    TS_Point p = ts.getPoint();
    // flip it around to match the screen.
    p.x = map(p.x, 0, 240, 240, 0);
    p.y = map(p.y, 0, 320, 320, 0);

    // Check which screen we are on
    switch (displayScreen)
    {
      case LOGO: //normal run mode screen
        // check if touch in on Beer Logo
        if((p.x > LOGOx1) && (p.x < (LOGOx1 + LOGO_WIDTH))) {
          if ((p.y > LOGOy1) && (p.y <= (LOGOy1 + LOGO_HEIGHT))) {
            changeBeerLogo();
          }
        }
        // check if touch is on scale
        if((p.x > GAUGEx) && (p.x < tft.width())) {
          if ((p.y > GAUGEy) && (p.y <= tft.height())) {
            // calibrate scale code here
            Serial.println("Calibrate Scale hit");
            paintCalScale();
          }
        }
        break;
      case CALIBRATE_SCALE:
        // check if touch is on TARE button
        if((p.x > TARE_X) && (p.x < (TARE_X + BUTTON_W))) {
          if ((p.y > TARE_Y) && (p.y <= (TARE_Y + BUTTON_H))) {
            // Tare scale 
            Serial.println("Tare Scale");
            drawTareButton(ILI9341_DARKGREY);
            scaleOffset = beerScale.tare();
            Serial.print(">>Scale offset:");
            Serial.println(scaleOffset);
            takeReading();
            drawTareButton();
          }
        }
        // check if touch is on 120 lbs button
        if((p.x > CAL_X) && (p.x < (CAL_X + BUTTON_W))) {
          if ((p.y > CAL_Y) && (p.y <= (CAL_Y + BUTTON_H))) {
            // Calibrate scale 
            Serial.println("Scale Calibration: 120 lbs");
            drawCalButton(ILI9341_DARKGREY);
            scaleDivider = beerScale.setKnownWeight(KNOWN_WEIGHT_LBS);
            Serial.print(">>Scale divider: ");
            Serial.println(scaleDivider);
            takeReading();
            drawCalButton();
          }
        }
        // check if touch is on cancel button
        if((p.x > CANCEL_X) && (p.x < (CANCEL_X + BUTTON_W))) {
          if ((p.y > CANCEL_Y) && (p.y <= (CANCEL_Y + BUTTON_H))) {
            // Cancel button hit
            Serial.println("Scale Calibration Canceled");
            readScaleParams(); //restore previous values;
            loadBeerLogo();
            }
          }
        // check if touch is on Save button
        if((p.x > SAVE_X) && (p.x < (SAVE_X + BUTTON_W))) {
          if ((p.y > SAVE_Y) && (p.y <= (SAVE_Y + BUTTON_H))) {
            // SAVE  button hit
            Serial.println("Scale Calibration Saved");
            saveScaleParams();
            }
          }         
        break;
      case CHANGE_LOGO:
        int index = getThumbnailIndexFromTouch(p);
        if(index > 0){
          if (logoIndex != index){
            logoIndex = index;
            EEPROM.put(eep_add_logoIndex, logoIndex);
            if(EEPROM.commit()){
            Serial.print(">>>> Logo Value saved. LogoIndex: ");
            Serial.println(logoIndex);
            } else {
              Serial.println("ERROR: Logo value not saved");
            }
          }
        }
        loadBeerLogo();
        break;
    } // end of switch
  } // end of touch
  else if ((current_time - start_time >= timed_event) && (displayScreen == LOGO)) {
    // Take weight reading and update text & gauge on display
    takeReading();
    start_time = current_time;  // reset the timer
  }
}

// Function implementations

void draw_percent_2_GaugeY1(float percent) {
    int pixel_total = tft.height() - GAUGEy;
    percent = constrain(percent, 0, 100);
    int gaugeY1 = tft.height() - ((pixel_total * percent) / 100);
    int red = 255 - map(percent, 0, 100, 0, 255);
    int green = map(percent, 0, 100, 0, 255);

    // Draw placeholder/Background for Gauge
    tft.fillRect(GAUGEx, GAUGEy, GAUGE_W, gaugeY1, ILI9341_BLACK);
    // Draw filled portion
    tft.fillRect(GAUGEx, gaugeY1, GAUGE_W, tft.height() - gaugeY1, tft.color565(red, green, 0));
    // Draw text
    if (percent > 98) {
      tft.setCursor(GAUGEx + 2, gaugeY1);
    } else {
      tft.setCursor(GAUGEx + 2, gaugeY1 - 13);
    }
    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);
    tft.print(percent,1);
    tft.print("%");
}

void takeReading(){
    //Serial.print("Raw weight: ");
    //Serial.print(beerScale.getUnitWeight());
    //Serial.println(" lbs");
    beerScale.getBeerRemaining(&status, &activeKegSize);    
    
    // Clear out Status text and update values
    tft.fillRect(150, 0, tft.width() - 150, ROW_HEIGHT * 2, ILI9341_BLACK);

    // Display weight
    tft.setCursor(17, 6);
    tft.setTextColor(ILI9341_BLUE);
    tft.setTextSize(1);
    tft.print("Approx Beer Remaining: ");
    tft.print(status.weight_lbs, 2);
    tft.println(" lbs");

    // Display units
    tft.setCursor(5, ROW_HEIGHT);
    tft.setTextColor(ILI9341_BLUE);
    tft.print("Approx Drinks Remaining: ");
    tft.print(status.units_remain, 2);

    // Draw Percent level shape for Gauge
    draw_percent_2_GaugeY1(status.level_percent);
}

void changeBeerLogo(){
    Serial.println("Beer Logo hit");
    clearBeerLogo();
    displayScreen = CHANGE_LOGO;
    //load up the 9 thumbnails, 3x3 grid display
    for (int row = 0; row < 3; row++){
        for (int col = 0; col < 3; col++){
          int thumb_x = col * THUMB_W + LOGOx1;
          int thumb_y = (row * THUMB_H) + LOGOy1;
          int index = (3*row + col) + 1;
          filenameThumb[6] = '0' + (index);
          reader.drawBMP(filenameThumb, tft, thumb_x,thumb_y);
      }
    }
    // SD card
    //reader.drawBMP("/busch2.bmp", tft, LOGOx1,LOGOy1);
    // Embedded Logo
    //tft.drawRGBBitmap(LOGOx1,LOGOy1, buschLightBMP, BEER_LOGO_WIDTH, BEER_LOGO_HEIGHT);
    delay(1500);
    Serial.println("Delay done, select logo");
}

void clearBeerLogo(){         
  tft.fillRect(LOGOx1, LOGOy1, LOGOx1 + LOGO_WIDTH, LOGOy1 + LOGO_HEIGHT, ILI9341_BLACK);
  displayScreen = BLANK;
}

void loadBeerLogo(){
  // Embedded Logo
  //tft.drawRGBBitmap(LOGOx1,LOGOy1, buschLightBMP, BEER_LOGO_WIDTH, BEER_LOGO_HEIGHT);
  // SD card
  filenameLogo[5] = '0' + logoIndex;
  reader.drawBMP(filenameLogo, tft, LOGOx1,LOGOy1);
  displayScreen = LOGO;
}

void paintCalScale(){
  clearBeerLogo();
  displayScreen = CALIBRATE_SCALE;
  
  // Tare button
  drawTareButton();

  // Calibrated Weight button
  drawCalButton();

  // Cancel Button
  tft.fillRect(CANCEL_X, CANCEL_Y, BUTTON_W, BUTTON_H, ILI9341_RED);
  tft.setCursor(CANCEL_X + 15, CANCEL_Y + (BUTTON_H / 2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Cancel");

  // Save Button
  tft.fillRect(SAVE_X, SAVE_Y, BUTTON_W, BUTTON_H, ILI9341_DARKGREEN);
  tft.setCursor(SAVE_X + 20, SAVE_Y + (BUTTON_H / 2));
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("SAVE");
}

void saveScaleParams(){
  EEPROM.put(eep_add_offset, scaleOffset);
  EEPROM.put(eep_add_divider, scaleDivider);
  if(EEPROM.commit()){
    Serial.print(">>>> Scale Values saved. Offset: ");
    Serial.print(scaleOffset);
    Serial.print(", Divider: ");
    Serial.println(scaleDivider);
  } else {
    Serial.println("ERROR: Values not saved");
  }
  loadBeerLogo();
}

void readScaleParams(){
  EEPROM.get(eep_add_offset, scaleOffset);
  EEPROM.get(eep_add_divider,scaleDivider);
  Serial.print("EEPROM Read>> Scale Offset: ");
  Serial.print(scaleOffset);
  Serial.print(", Divider: ");
  Serial.println(scaleDivider);
}  

int getThumbnailIndexFromTouch(TS_Point p){
  int index = 0;
  if ((p.x > LOGOx1) && (p.x < (3*THUMB_W + LOGOx1))){
    if((p.y > LOGOy1) && (p.y <= LOGOy1 + 3*THUMB_H)){
        //  move the origin of the touch point with respect to the thumbnail region
        p.x = p.x - LOGOx1;
        p.y = p.y - LOGOy1;
        int row = (p.y / THUMB_H);
        int col = (p.x / THUMB_W);
        index = (3*row + col) + 1;
    }
  }
  return index;
}

void drawTareButton(int color){
  tft.fillRect(TARE_X, TARE_Y, BUTTON_W, BUTTON_H, color);  
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(50, TARE_Y - 20);
  tft.println("Clear your scale,");
  tft.println("Then push TARE button below");
  tft.setCursor(TARE_X + 25, TARE_Y + (BUTTON_H / 2));
  tft.setTextSize(2);
  tft.println("TARE");
}

void drawCalButton(int color){
  tft.fillRect(CAL_X, CAL_Y, BUTTON_W, BUTTON_H, color);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setCursor(50, CAL_Y - 20);
  tft.println("Place 120lbs on scale,");
  tft.println("Then push 120 lbs button below");
  tft.setCursor(CAL_X + 10, CAL_Y + (BUTTON_H / 2));
  tft.setTextSize(2);
  tft.println("120 lbs");
}