#include <TFT_eSPI.h>
#include "NotoSansBold36.h"
#include "fish.h"  // Fish image in XBM format

// ----- TFT Display Setup -----
TFT_eSPI tft = TFT_eSPI();
#define AA_FONT_LARGE NotoSansBold36

int backlight = 12;
int brightness = 0;  // how bright the LED is
int fadeAmount = 5;  // how many points to fade the LED by

#define logoWidth 240   // logo width
#define logoHeight 240  // logo height

// Set X and Y coordinates where the image will be drawn
int x = 0;
int y = 0;

void setup() {
  // Initialize Serial (optional for debugging)
  Serial.begin(115200);

  // Initialize TFT
  pinMode(backlight, OUTPUT);
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Load and set the large antialiased font
  tft.loadFont(AA_FONT_LARGE);

  // Display the fish image
  drawFish();

  // Display the message
  drawMessage();
}

void loop() {
  // Nothing to do in the loop for now
}

void drawFish() {
  // Draw the fish image (240x240)
  tft.drawXBitmap(x, y, fish_bitmap, logoWidth, logoHeight, TFT_WHITE, TFT_BLACK);
  delay(100);
}

void drawMessage() {
  // Set text color and background
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);

  // Draw the message below the fish image
  tft.drawString(" ", 120, 180);
  tft.drawString(" ", 120, 220);
}
