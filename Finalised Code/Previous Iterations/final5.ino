#include <TinyGPSPlus.h>
#include <TFT_eSPI.h>
#include "route_1.h"           // startLat/startLon and stopLats/stopLons, numberOfStops :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3}
// compass in this will not work yet


// GPS serial pins
#define GPS_RX_PIN 16         // ESP32 RX2 pin (GPS TX → ESP32 RX2)
#define GPS_TX_PIN 17         // ESP32 TX2 pin (not used but declared)

// How close to consider "at" a checkpoint (meters)
const double THRESHOLD_METERS = 5.0;

// Arrow drawing parameters
const int ARROW_LENGTH = 30; // pixels from center
const int ARROW_HEAD_LEN = 8;
const double ARROW_HEAD_ANGLE = 30.0 * DEG_TO_RAD; // 30°

TinyGPSPlus gps;
TFT_eSPI tft = TFT_eSPI();

int currentStop = 0;
bool showingCross = false;

void setup() {
  // Serial for debug
  Serial.begin(115200);
  // GPS on Serial2
  Serial2.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // LCD init
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
}

// Haversine distance (meters)
double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;
  double φ1 = lat1 * DEG_TO_RAD;
  double φ2 = lat2 * DEG_TO_RAD;
  double dφ = (lat2 - lat1) * DEG_TO_RAD;
  double dλ = (lon2 - lon1) * DEG_TO_RAD;
  double a = sin(dφ/2)*sin(dφ/2) +
             cos(φ1)*cos(φ2)*sin(dλ/2)*sin(dλ/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}

// Bearing from point1 → point2 (degrees, 0 = North, clockwise)
double bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  double φ1 = lat1 * DEG_TO_RAD;
  double φ2 = lat2 * DEG_TO_RAD;
  double Δλ = (lon2 - lon1) * DEG_TO_RAD;
  double y = sin(Δλ) * cos(φ2);
  double x = cos(φ1)*sin(φ2) -
             sin(φ1)*cos(φ2)*cos(Δλ);
  double θ = atan2(y, x);
  double bearing = fmod((θ * RAD_TO_DEG + 360.0), 360.0);
  return bearing;
}

void drawArrow(double bearing) {
  tft.fillScreen(TFT_BLACK);
  int cx = tft.width()/2;
  int cy = tft.height()/2;
  double rad = bearing * DEG_TO_RAD;

  // Tip of arrow
  int x1 = cx + ARROW_LENGTH * sin(rad);
  int y1 = cy - ARROW_LENGTH * cos(rad);
  // Main shaft
  tft.drawLine(cx, cy, x1, y1, TFT_WHITE);

  // Arrowhead
  double ang1 = rad + PI - ARROW_HEAD_ANGLE;
  double ang2 = rad + PI + ARROW_HEAD_ANGLE;
  int x2 = x1 + ARROW_HEAD_LEN * sin(ang1);
  int y2 = y1 - ARROW_HEAD_LEN * cos(ang1);
  int x3 = x1 + ARROW_HEAD_LEN * sin(ang2);
  int y3 = y1 - ARROW_HEAD_LEN * cos(ang2);
  tft.drawLine(x1, y1, x2, y2, TFT_WHITE);
  tft.drawLine(x1, y1, x3, y3, TFT_WHITE);
}

void drawCross() {
  tft.fillScreen(TFT_BLACK);
  int cx = tft.width()/2;
  int cy = tft.height()/2;
  int s = 20;  // half‐size of cross
  // Diagonal lines
  tft.drawLine(cx-s, cy-s, cx+s, cy+s, TFT_RED);
  tft.drawLine(cx-s, cy+s, cx+s, cy-s, TFT_RED);
}

void loop() {
  // Feed GPS parser
  while (Serial2.available()) {
    gps.encode(Serial2.read());
  }

  // If we have a valid fix:
  if (gps.location.isValid()) {
    double lat = gps.location.lat();
    double lon = gps.location.lng();
    if (currentStop < numberOfStops) {
      double tgtLat = stopLats[currentStop];
      double tgtLon = stopLons[currentStop];
      double dist = distanceMeters(lat, lon, tgtLat, tgtLon);

      if (dist <= THRESHOLD_METERS) {
        // At checkpoint
        if (!showingCross) {
          drawCross();
          showingCross = true;
          Serial.printf("Reached stop %d (%.1f m)\n", currentStop+1, dist);
          delay(5000); // pause so user sees the cross - this is currently 5 seconds...
        }
        currentStop++;
      } else {
        // Show arrow
        double bear = bearingDegrees(lat, lon, tgtLat, tgtLon);
        drawArrow(bear);
        showingCross = false;
      }
    } else {
      // All stops done: show cross permanently
      if (!showingCross) {
        drawCross();
        showingCross = true;
        Serial.println("Route complete!");
      }
    }
  }
}
