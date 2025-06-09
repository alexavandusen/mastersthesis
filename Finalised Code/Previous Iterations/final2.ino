#include <TFT_eSPI.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include "imagelist.h"

// ----- Pin Definitions -----
#define BACKLIGHT_PIN 12
#define CHECKPOINT_BUTTON 15
#define PAUSE_BUTTON 27

// ----- GPS & Compass -----
#define GPS_SERIAL Serial1
#define CMPS12_ADDRESS 0x60
#define ANGLE_8 1

// ----- Wi-Fi Credentials -----
const char* ssid = "YourSSID";
const char* password = "YourPassword";

// ----- Global State -----
TFT_eSPI tft = TFT_eSPI();
TinyGPSPlus gps;
int currentStop = 0;
bool gpsReady = false;
bool paused = false;

// ----- Persona Enum -----
enum Persona {
  EXPLORER,
  SCIENTIST,
  ARTIST,
  NUM_PERSONAS
};

Persona currentPersona = EXPLORER;
const unsigned char* personaImages[NUM_PERSONAS] = {
  explorer_intro,
  scientist_intro,
  artist_intro
};

// ----- Arrow Images -----
enum Direction {
  DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW
};

const unsigned char* directionArrows[8] = {
  arrow_N, arrow_NE, arrow_E, arrow_SE, arrow_S, arrow_SW, arrow_W, arrow_NW
};

// ----- Setup -----
void setup() {
  Serial.begin(115200);
  GPS_SERIAL.begin(9600);
  Wire.begin();
  
  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  pinMode(CHECKPOINT_BUTTON, INPUT_PULLUP);
  pinMode(PAUSE_BUTTON, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);

  setupWiFi();
  receivePersonaFromWiFi();  // Mocked for now
  showPersonaIntro();
}

// ----- Loop -----
void loop() {
  handleGPS();

  if (digitalRead(PAUSE_BUTTON) == LOW) {
    delay(300);
    paused = !paused;
    if (paused) {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(60, 120);
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.println("Paused");
    } else {
      tft.fillScreen(TFT_BLACK);
    }
  }

  if (paused) return;

  if (gpsReady) {
    showDirectionArrow();
  }

  if (digitalRead(CHECKPOINT_BUTTON) == LOW) {
    delay(300);
    showCheckpointSuggestion();
    currentStop++;
    delay(2000);
    tft.fillScreen(TFT_BLACK);
  }

  delay(500);
}

// ----- Functions -----

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected.");
}

void receivePersonaFromWiFi() {
  // MOCKED: Replace with HTTP or WebSocket handling
  String persona = "SCIENTIST";
  currentPersona = resolvePersona(persona);
}

void handleGPS() {
  while (GPS_SERIAL.available()) {
    gps.encode(GPS_SERIAL.read());
    if (gps.location.isValid()) {
      gpsReady = true;
    }
  }
}

void showPersonaIntro() {
  tft.fillScreen(TFT_BLACK);
  tft.drawXBitmap(0, 0, personaImages[currentPersona], 240, 240, TFT_WHITE);
  delay(3000);
  tft.fillScreen(TFT_BLACK);
}

void showDirectionArrow() {
  double lat = (currentStop == 0) ? startLat : stopLats[currentStop - 1];
  double lon = (currentStop == 0) ? startLon : stopLons[currentStop - 1];
  double currentLat = gps.location.lat();
  double currentLon = gps.location.lng();
  double courseTo = TinyGPSPlus::courseTo(currentLat, currentLon, lat, lon);
  int currentAngle = readCompass();
  int relative = (int(courseTo) - currentAngle + 360) % 360;

  const unsigned char* arrow = directionArrows[getDirectionIndex(relative)];
  tft.fillScreen(TFT_BLACK);
  tft.drawXBitmap(0, 0, arrow, 240, 240, TFT_WHITE);
}

void showCheckpointSuggestion() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 100);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(2);
  tft.println("Checkpoint Reached!");
  tft.setCursor(20, 140);
  tft.setTextColor(TFT_WHITE);
  tft.println("Suggestion:");
  tft.setCursor(20, 170);
  tft.println("(Add content here)");
}

int readCompass() {
  Wire.beginTransmission(CMPS12_ADDRESS);
  Wire.write(ANGLE_8);
  Wire.endTransmission();
  Wire.requestFrom(CMPS12_ADDRESS, 5);
  while (Wire.available() < 5);
  Wire.read();  // angle8 not used
  int high = Wire.read();
  int low = Wire.read();
  Wire.read(); Wire.read();  // pitch, roll
  return ((high << 8) | low) / 10;
}

int getDirectionIndex(int deg) {
  if (deg >= 337.5 || deg < 22.5) return DIR_N;
  else if (deg >= 22.5 && deg < 67.5) return DIR_NE;
  else if (deg >= 67.5 && deg < 112.5) return DIR_E;
  else if (deg >= 112.5 && deg < 157.5) return DIR_SE;
  else if (deg >= 157.5 && deg < 202.5) return DIR_S;
  else if (deg >= 202.5 && deg < 247.5) return DIR_SW;
  else if (deg >= 247.5 && deg < 292.5) return DIR_W;
  else return DIR_NW;
}

Persona resolvePersona(String name) {
  name.toUpperCase();
  if (name == "SCIENTIST") return SCIENTIST;
  else if (name == "ARTIST") return ARTIST;
  else return EXPLORER;
}
