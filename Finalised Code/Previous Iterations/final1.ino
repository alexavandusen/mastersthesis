// this will be the final code 

bool debugMode = true;

// Include Libraries
#include <TinyGPSPlus.h>
#include <TFT_eSPI.h>
#include <Wire.h>
//#include <map>
#include "Adafruit_DRV2605.h"

// Include Configurations and Images
#include "route_1.h"     // Assumes this contains GPS waypoints and number of stops
#include "imagelist.h"  // Assumes this includes declarations for images

// TFT Display Settings
#define logoWidth 240
#define logoHeight 240
TFT_eSPI tft = TFT_eSPI();

// The TinyGPSPlus object
TinyGPSPlus gps;
