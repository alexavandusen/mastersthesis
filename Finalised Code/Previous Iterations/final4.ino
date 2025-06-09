#include <TinyGPS++.h>
#include <TFT_eSPI.h>
#include <QMC5883LCompass.h>
#include "route_1.h"        // your stopLats[], stopLons[], numberOfStops :contentReference[oaicite:2]{index=2}:contentReference[oaicite:3]{index=3}

// === Pin definitions ===
#define GPS_RX_PIN 16       // change to your board’s GPS RX pin
#define GPS_TX_PIN 17       // change to your board’s GPS TX pin

#define GPS_BAUD 9600

// === Serial & peripherals ===
HardwareSerial gpsSerial(2); 
TinyGPSPlus     gps;
TFT_eSPI        tft = TFT_eSPI();
QMC5883LCompass compass;

// === State ===
int targetIndex = 0;         // current waypoint

// === Forward declarations ===
double calculateBearing(double lat1, double lon1, double lat2, double lon2);
double distanceMeters(double lat1, double lon1, double lat2, double lon2);
void   drawArrow(float angle);

void setup() {
  Serial.begin(115200);
  // GPS on Serial2 (UART2) with RX/TX swapped as needed
  gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  // TFT init
  tft.init();
  tft.setRotation(1);       // landscape; change if needed
  tft.fillScreen(TFT_BLACK);

  // Compass init
  compass.init();
  compass.setMode(0x01, 0x0C, 0x00, 0x00);  // cont,200Hz,±2G,OSR=512

  // Splash
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("Navigation", 10, 10);
  delay(1000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {
  // 1) Feed GPS parser
  while (gpsSerial.available()) {
    gps.encode(gpsSerial.read());
  }

  // 2) Wait for a valid fix
  if (!gps.location.isValid()) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.drawString("Waiting GPS...", 10, tft.height()/2 - 10);
    return;
  }

  double lat = gps.location.lat();
  double lon = gps.location.lng();

  // 3) Compute true bearing to target
  double wLat = stopLats[targetIndex];
  double wLon = stopLons[targetIndex];
  double brng = calculateBearing(lat, lon, wLat, wLon);

  // 4) Read compass heading
  compass.read();
  float heading = compass.getAzimuth();

  // 5) Compute relative heading [0–360)
  float rel = brng - heading;
  if (rel < 0)    rel += 360.0;
  if (rel >= 360) rel -= 360.0;

  // 6) Draw arrow toward target
  drawArrow(rel);

  // 7) Show numeric values (optional)
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  char buf[32];
  snprintf(buf, sizeof(buf), "B:%3.0f H:%3.0f", brng, heading);
  tft.drawString(buf, 5, tft.height() - 12);

  // 8) Advance when close enough (<10 m)
  if (distanceMeters(lat, lon, wLat, wLon) < 20.0) {
    targetIndex = (targetIndex + 1) % numberOfStops;
  }
}

// Draws a line-arrow at ‘angle’ degrees (0 = East, 90 = South), centered on screen
void drawArrow(float angle) {
  tft.fillScreen(TFT_BLACK);
  int cx = tft.width()  / 2;
  int cy = tft.height() / 2;
  int len = min(cx, cy) - 20;

  // Convert to radians, adjust so 0° = up
  float rad = radians(angle - 90.0);

  int x2 = cx + len * cos(rad);
  int y2 = cy + len * sin(rad);

  // Shaft
  tft.drawLine(cx, cy, x2, y2, TFT_GREEN);

  // Arrowhead
  int ah = 10;
  float left  = rad + radians(150);
  float right = rad - radians(150);
  int x3 = x2 + ah * cos(left);
  int y3 = y2 + ah * sin(left);
  int x4 = x2 + ah * cos(right);
  int y4 = y2 + ah * sin(right);
  tft.fillTriangle(x2, y2, x3, y3, x4, y4, TFT_GREEN);
}

// Bearing between two GPS coords, 0 = North, clockwise
double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
  double φ1 = radians(lat1), φ2 = radians(lat2);
  double Δλ = radians(lon2 - lon1);
  double y = sin(Δλ) * cos(φ2);
  double x = cos(φ1)*sin(φ2)
           - sin(φ1)*cos(φ2)*cos(Δλ);
  double θ = atan2(y, x);
  return fmod(degrees(θ) + 360.0, 360.0);
}

// Haversine distance (m)
double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000.0;
  double φ1 = radians(lat1), φ2 = radians(lat2);
  double Δφ = radians(lat2 - lat1);
  double Δλ = radians(lon2 - lon1);
  double a = sin(Δφ/2)*sin(Δφ/2)
           + cos(φ1)*cos(φ2)
           * sin(Δλ/2)*sin(Δλ/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}
