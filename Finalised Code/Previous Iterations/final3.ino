#include <TFT_eSPI.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include "config.h"  // Contains startLat, startLon, stopLats[], stopLons[], numStops


// in this version, persona selection is loaded onto the hardware


// ----- PIN DEFINITIONS -----
#define BUTTON_PIN 15
#define BACKLIGHT_PIN 12

// ----- TFT DISPLAY -----
TFT_eSPI tft = TFT_eSPI();

// ----- GPS (BN-880) -----
TinyGPSPlus gps;
#define GPS_SERIAL Serial1

// ----- Compass (CMPS12) -----
#define CMPS12_ADDRESS 0x60
#define ANGLE_8 1
unsigned int angle16;

// ----- State Machines -----
enum State {
  SELECT_PERSONA,
  SHOW_PERSONA_INTRO,
  NAVIGATING,
  AT_CHECKPOINT
};

State currentState = SELECT_PERSONA;

enum Persona {
  EXPLORER,
  SCIENTIST,
  ARTIST,
  NUM_PERSONAS
};

Persona currentPersona = EXPLORER;

// ----- Bitmaps (to be defined in imagelist.h) -----
#include "imagelist.h"
const unsigned char* personaImages[NUM_PERSONAS] = {
  explorer_intro,
  scientist_intro,
  artist_intro
};

const unsigned char* directionArrows[4] = {
  arrow_N, arrow_E, arrow_S, arrow_W
};

// ----- GPS and Navigation -----
double currentLat, currentLon;
int currentStop = 0;
bool gpsReady = false;

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  GPS_SERIAL.begin(9600);
  Wire.begin();  // I2C for CMPS12

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);
  showPersonaSelection();
}

// ----- Main Loop -----
void loop() {
  while (GPS_SERIAL.available()) {
    gps.encode(GPS_SERIAL.read());
    if (gps.location.isValid()) {
      currentLat = gps.location.lat();
      currentLon = gps.location.lng();
      gpsReady = true;
    }
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(200);  // Debounce
    handleButtonPress();
  }

  if (currentState == NAVIGATING && gpsReady) {
    displayDirectionArrow();
  }
}

// ----- Button Logic -----
void handleButtonPress() {
  switch (currentState) {
    case SELECT_PERSONA:
      currentPersona = static_cast<Persona>((currentPersona + 1) % NUM_PERSONAS);
      showPersonaSelection();
      break;
    case SHOW_PERSONA_INTRO:
      currentState = NAVIGATING;
      tft.fillScreen(TFT_BLACK);
      break;
    case NAVIGATING:
      currentState = AT_CHECKPOINT;
      showCheckpointSuggestion();
      break;
    case AT_CHECKPOINT:
      currentStop++;
      if (currentStop >= numStops) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(30, 120);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE);
        tft.println("Trail Complete!");
      } else {
        currentState = NAVIGATING;
      }
      break;
  }
}

// ----- Persona Selection -----
void showPersonaSelection() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 30);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.println("Select Persona:");
  tft.setCursor(20, 80);
  if (currentPersona == EXPLORER) tft.println("> Explorer");
  else tft.println("  Explorer");
  tft.setCursor(20, 110);
  if (currentPersona == SCIENTIST) tft.println("> Scientist");
  else tft.println("  Scientist");
  tft.setCursor(20, 140);
  if (currentPersona == ARTIST) tft.println("> Artist");
  else tft.println("  Artist");

  delay(1000);
  tft.fillScreen(TFT_BLACK);
  tft.drawXBitmap(0, 0, personaImages[currentPersona], 240, 240, TFT_WHITE);
  currentState = SHOW_PERSONA_INTRO;
}

// ----- Compass -----
int readCompass() {
  Wire.beginTransmission(CMPS12_ADDRESS);
  Wire.write(ANGLE_8);
  Wire.endTransmission();
  Wire.requestFrom(CMPS12_ADDRESS, 5);
  while (Wire.available() < 5);
  Wire.read();  // angle8 not used
  int high_byte = Wire.read();
  int low_byte = Wire.read();
  Wire.read(); Wire.read();  // pitch, roll
  angle16 = (high_byte << 8) | low_byte;
  return angle16 / 10;
}

// ----- Show Direction Arrow -----
void displayDirectionArrow() {
  double lat = (currentStop == 0) ? startLat : stopLats[currentStop - 1];
  double lon = (currentStop == 0) ? startLon : stopLons[currentStop - 1];

  double courseTo = TinyGPSPlus::courseTo(currentLat, currentLon, lat, lon);
  int currentAngle = readCompass();
  int relative = (int(courseTo) - currentAngle + 360) % 360;

  const unsigned char* arrow;
  if (relative < 45 || relative >= 315) arrow = arrow_N;
  else if (relative >= 45 && relative < 135) arrow = arrow_E;
  else if (relative >= 135 && relative < 225) arrow = arrow_S;
  else arrow = arrow_W;

  tft.fillScreen(TFT_BLACK);
  tft.drawXBitmap(0, 0, arrow, 240, 240, TFT_WHITE);
}

// ----- Checkpoint Suggestions -----
void showCheckpointSuggestion() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 100);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.println("Checkpoint Reached!");
  tft.setCursor(10, 140);
  tft.setTextColor(TFT_WHITE);
  tft.println("(Add interaction text)");
}
