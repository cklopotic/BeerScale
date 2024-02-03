
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "SPIFFS.h"

#include <EEPROM.h>
#include <HX711.h>
#include <BeerScale.h>
#include <BeerScreen.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <Adafruit_FT6206.h>
#include "ImageHelper.h"
#include <Preferences.h>

//Wifi setup
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "ip";
const char* PARAM_INPUT_4 = "gateway";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* ipPath = "/ip.txt";
const char* gatewayPath = "/gateway.txt";

//ssid max 32 characters
//pass max 63 characters
//IP address max 15 characters
//gateway max 15 characters


//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
Preferences preferences;

IPAddress localIP;
//IPAddress localIP(192, 168, 1, 200); // hardcoded

// Set your Gateway IP address
IPAddress localGateway;
//IPAddress localGateway(192, 168, 1, 1); //hardcoded
IPAddress subnet(255, 255, 0, 0);

// Timer variables
unsigned long previousMillis = 0;
const long interval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)

//******   Scale Setup   *****
BeerScale beerScale;
//HX711 Scale circuit wiring
const uint8_t SCALE_DOUT_PIN = 46;
const uint8_t SCALE_SCK_PIN = 3;

// Scale Adjustment settings
const uint16_t KNOWN_WEIGHT_LBS = 120;  //used for scale calibration

BeerStatus_t status;
BeerStatus_t previousStatus;

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
bool sdCardPresent;

// Create the display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// The FT6206 uses hardware I2C (SCL/SDA) AKA: the touchscreen
Adafruit_FT6206 ts = Adafruit_FT6206();




//*****   Memory Setup   ******
#define EEPROM_SIZE 32 //number of bytes needed (long data type is 4 bytes, floats & doubles are 8)
                      // sizeof(scaleOffset) + sizeof(scaleDivider) + logoIndex + KegSizeIndex
int eep_add_offset = 0; //scale offset
int eep_add_divider = 8; //scale divider
int eep_add_logoIndex = 16; //logo selected
int eep_add_kegSizeIndex = 20; //keg size

enum screenStates { BLANK, LOGO, CHANGE_LOGO, CALIBRATE_SCALE } displayScreen = BLANK;

// Function prototypes
void drawBeerSelection();
void clearBeerLogo();
void loadBeerLogo();
void takeReading();
int getThumbnailIndexFromTouch(TS_Point p);

// Main loop non-blocking delay routine variables
unsigned long start_time; 
unsigned long timed_event;
unsigned long current_time;

//functions 
//TODO: can/should get moved to other files once refactored
// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
  Serial.print("Total space: ");
  Serial.println(SPIFFS.totalBytes());
  Serial.print("Used space: ");
  Serial.println(SPIFFS.usedBytes());

}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

// Initialize WiFi
bool initWiFi() {
  if(ssid=="" || ip==""){
    Serial.println("Undefined SSID or IP address.");
    return false;
  }

  WiFi.mode(WIFI_STA);
  localIP.fromString(ip.c_str());
  localGateway.fromString(gateway.c_str());


  if (!WiFi.config(localIP, localGateway, subnet)){
    Serial.println("STA Failed to configure");
    return false;
  }
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.println("Connecting to WiFi...");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  while(WiFi.status() != WL_CONNECTED) {
    currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      Serial.println("Failed to connect.");
      return false;
    }
  }

  Serial.println(WiFi.localIP());
  return true;
}

void notifyClients(BeerStatus_t status) {
  String dataPayload;
  dataPayload = "weight:";
  dataPayload += status.weight_lbs;
  dataPayload += ",units:";
  dataPayload += status.units_remain;
  dataPayload += ",percent:";
  dataPayload += status.level_percent;
  Serial.println(dataPayload);
  ws.textAll(dataPayload);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
      notifyClients(status);
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

// Callback processor for main webpage. 
String processor(const String& var) {
  if(var == "STATE") {
   
  }
  return String();
}


// the setup function runs once when you press reset or power the board
void setup() {
  
  Serial.begin(115200);
  Serial.println("Startup Initiated");
  
  preferences.begin("beer-app", false);

  initSPIFFS();
  
  ssid = preferences.getString("ssid");
  pass = preferences.getString("pass");
  ip = preferences.getString("ip");
  gateway = preferences.getString("gateway");

  Serial.println(ssid);
  //Serial.println(pass);
  Serial.println(ip);
  Serial.println(gateway);
  
  Serial.println("SPI bus starting");
  // Setup SPI bus using hardware SPI
  SPI.begin();
  tft.begin(SPI_FREQUENCY);
  tft.setRotation(0);  // Set rotation to 0 degrees

  // Clear the screen
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(17, 6);
  tft.setTextColor(ILI9341_BLUE);
  tft.setTextSize(1);
  tft.println("Attempting WiFi Connection");
  tft.print("SSID: ");
  tft.println(ssid);
  tft.print("IP: ");
  tft.println(ip);
    

  if(initWiFi()) {
    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(SPIFFS, "/BeerScale.html", "text/html", false, processor);
    });
    server.serveStatic("/", SPIFFS, "/");    

    //server.onFileUpload()

    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
    server.begin();
  }
  else {
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP("BEERSCALE-WIFI-MANAGER", NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 
    // Clear the screen
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(17, 6);
    tft.setTextColor(ILI9341_BLUE);
    tft.setTextSize(1);
    tft.println("WiFi Connection Failed");
    tft.println("Running as Access Point");
    tft.println("Connect to this Wifi &");
    tft.println("on web broswer url:");
    tft.println(IP);
    delay(6000);

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/wifimanager.html", "text/html");
    });
    
    server.serveStatic("/", SPIFFS, "/");
    
    server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      preferences.begin("beer-app", false);
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            preferences.putString("ssid", ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            preferences.putString("pass", pass.c_str());
          }
          // HTTP POST ip value
          if (p->name() == PARAM_INPUT_3) {
            ip = p->value().c_str();
            Serial.print("IP Address set to: ");
            Serial.println(ip);
            // Write file to save value
            preferences.putString("ip", ip.c_str());
          }
          // HTTP POST gateway value
          if (p->name() == PARAM_INPUT_4) {
            gateway = p->value().c_str();
            Serial.print("Gateway set to: ");
            Serial.println(gateway);
            // Write file to save value
            preferences.putString("gateway", gateway.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
      preferences.end();
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    server.begin();
  }
 

  Serial.println("Reading Saved Settings");
  // Read saved settings
  logoIndex = preferences.getInt("logoIndex", 1);
  preferences.end();

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(eep_add_logoIndex, logoIndex);
  
  constrain(logoIndex,1,9);
    
  // Setup SD card
  sdCardPresent = SD.begin(SD_CS, SD_SCK_MHZ(25));  // ESP32 requires 25 MHz limit
  if(sdCardPresent){
    Serial.println("SD card initialized.");
  } else {
    Serial.println("SD begin() failed");
  }
  
  // Initialize and setup the scale
  Serial.println("Initializing the scale");
  beerScale.init(SCALE_DOUT_PIN, SCALE_SCK_PIN, eep_add_offset, eep_add_divider, eep_add_kegSizeIndex);

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
  
  timed_event = 2000; // after 1000 ms trigger the event
  current_time = millis();
  start_time = current_time;
  Serial.println("Startup Completed!");
}

// the loop function runs over and over again forever
void loop() {
  ws.cleanupClients();
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
        // check if touch in on Beer Logo if an SD card is present
        if (sdCardPresent){
          if((p.x > LOGOx1) && (p.x < (LOGOx1 + LOGO_WIDTH))) {
            if ((p.y > LOGOy1) && (p.y <= (LOGOy1 + LOGO_HEIGHT))) {
                drawBeerSelection();
            }
          }
        }
        // check if touch is on scale
        if((p.x > GAUGEx) && (p.x < tft.width())) {
          if ((p.y > GAUGEy) && (p.y <= tft.height())) {
            // calibrate scale code here
            Serial.println("Calibrate Scale hit");
            clearBeerLogo();
            displayScreen = CALIBRATE_SCALE;
            draw_calibration_screen(&tft);
          }
        }
        break;
      case CALIBRATE_SCALE:
        // check if touch is on TARE button
        if((p.x > TARE_X) && (p.x < (TARE_X + BUTTON_W))) {
          if ((p.y > TARE_Y) && (p.y <= (TARE_Y + BUTTON_H))) {
            // Tare scale 
            Serial.println("Tare Scale");
            drawTareButton(&tft, ILI9341_DARKGREY);
            beerScale.tare();
            takeReading();
            drawTareButton(&tft);
          }
        }
        // check if touch is on 120 lbs button
        if((p.x > CAL_X) && (p.x < (CAL_X + BUTTON_W))) {
          if ((p.y > CAL_Y) && (p.y <= (CAL_Y + BUTTON_H))) {
            // Calibrate scale 
            Serial.println("Scale Calibration: 120 lbs");
            drawCalButton(&tft, ILI9341_DARKGREY);
            beerScale.setKnownWeight(KNOWN_WEIGHT_LBS);
            takeReading();
            drawCalButton(&tft);
          }
        }
        // check if touch is on cancel button
        if((p.x > CANCEL_X) && (p.x < (CANCEL_X + BUTTON_W))) {
          if ((p.y > CANCEL_Y) && (p.y <= (CANCEL_Y + BUTTON_H))) {
            // Cancel button hit
            Serial.println("Scale Calibration Canceled");
            beerScale.readScaleParams(); //restore previous values;
            loadBeerLogo();
            }
          }
        // check if touch is on Save button
        if((p.x > SAVE_X) && (p.x < (SAVE_X + BUTTON_W))) {
          if ((p.y > SAVE_Y) && (p.y <= (SAVE_Y + BUTTON_H))) {
            // SAVE  button hit
            Serial.println("Scale Calibration Saved");
            beerScale.saveScaleParams();
            loadBeerLogo();
            }
          }         
        break;
      case CHANGE_LOGO:
        int index = getThumbnailIndexFromTouch(p);
        if(index > 0){
          if (logoIndex != index){
            logoIndex = index;
            EEPROM.put(eep_add_logoIndex, logoIndex);
            preferences.begin("beer-app", false);
            preferences.putInt("logoIndex", logoIndex);
            preferences.end();
            //TODO: allow for different keg sizes.
            beerScale.setKegSize(0); //fixed size to a half barrel
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
void takeReading(){
    beerScale.getBeerRemaining(&status);
    notifyClients(status);      
    if (fabs(previousStatus.weight_lbs - status.weight_lbs) < 0.15f) return;
    
    previousStatus = status;
    updateScreenValues(&tft, status);
    notifyClients(status);
}

void drawBeerSelection(){
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
    // draw a frame around the current logo to indicate current selection
    int col = (logoIndex -1) % 3;
    int row = (logoIndex-1)/3;
    int thumb_x = col * THUMB_W + LOGOx1 + 1;
    int thumb_y = (row * THUMB_H) + LOGOy1 + 1;
    tft.drawRect(thumb_x, thumb_y, THUMB_W - 2,THUMB_H - 2, ILI9341_RED);

    delay(1500);
    Serial.println("Delay done, select logo");
}

void clearBeerLogo(){         
  tft.fillRect(LOGOx1, LOGOy1, LOGOx1 + LOGO_WIDTH, LOGOy1 + LOGO_HEIGHT, ILI9341_BLACK);
  displayScreen = BLANK;
}

void copyFileToSPIFFS(const char* sdFilePath, const char* spiffsFilePath) {
  // Open the file on the SD card
  File32 sdFile = SD.open(sdFilePath, O_RDONLY);

  if (sdFile) {
    // Open a file on SPIFFS for writing
    File spiffsFile = SPIFFS.open(spiffsFilePath, FILE_WRITE);

    if (spiffsFile) {
      // Copy data from SD file to SPIFFS file
      while (sdFile.available()) {
        spiffsFile.write(sdFile.read());
      }

      // Close both files
      spiffsFile.close();
      sdFile.close();
    } else {
      // Handle error opening SPIFFS file
    }
  } else {
    // Handle error opening SD file
  }
}

void loadBeerLogo(){
  if (sdCardPresent){
    filenameLogo[5] = '0' + logoIndex; //replace the index number in the filename string

    /* If using SPIFFS
      File file = SPIFFS.open(filenameLogo, "r");
        if (file) {

          // Skip BMP file header (14 bytes) and bitmap information header (40 bytes)
          file.seek(54);
          
          // Calculate row size considering padding
          int rowSize = (3 * LOGO_WIDTH + 3) & ~3;
          
          // Read pixel data into the array from bottom to top and left to right
          for (int y = LOGO_HEIGHT - 1; y >= 0; --y) {
            for (int x = 0; x < LOGO_WIDTH; ++x) {
            uint8_t blue = file.read();
            uint8_t green = file.read();
            uint8_t red = file.read();

            // Convert RGB888 to RGB565 format
            uint16_t rgb565 = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
            
            bmpLogoImage[y * LOGO_WIDTH + x] = rgb565;
            }
            // Skip any padding bytes at the end of the row
            for (int p = 0; p < (rowSize - 3 * LOGO_WIDTH); ++p) {
              file.read();
            }
          }

          file.close();
          Serial.println("BMP image loaded from SPIFFS");
        } else {
          Serial.println("Failed to open file for reading");
        }
      tft.drawRGBBitmap(LOGOx1,LOGOy1, bmpLogoImage, BEER_LOGO_WIDTH, BEER_LOGO_HEIGHT);
    */  // End using SPIFFS
    reader.drawBMP(filenameLogo, tft, LOGOx1,LOGOy1);
  } else { // Embedded Logo
    tft.drawRGBBitmap(LOGOx1,LOGOy1, buschLightBMP, BEER_LOGO_WIDTH, BEER_LOGO_HEIGHT);
  }
  copyFileToSPIFFS(filenameLogo,"/Logo.bmp");
  displayScreen = LOGO;
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

void saveFileToSDCard(const char* filename, const char* data, size_t len) {
    File32 file = SD.open(filename, O_WRITE);
    if (file) {
        file.write(data, len);
        file.close();
        Serial.println("File saved to SD card successfully");
    } else {
        Serial.println("Error opening file on SD card");
    }
}