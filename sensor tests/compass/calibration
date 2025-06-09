#include <QMC5883LCompass.h>

QMC5883LCompass compass;
float offset = 0.0;

void setup() {
  Serial.begin(115200);

  // 1) Initialize the compass (underlying I²C is handled by the library)
  compass.init();

  // 2) Optional: set Continuous mode, 200 Hz, ±2 G, OSR=512
  //    MODE=0x01, ODR=0x0C, RNG=0x00, OSR=0x00
  compass.setMode(0x01, 0x0C, 0x00, 0x00);

  // 3) Prompt and capture “point-north” offset
  Serial.println();
  Serial.println("Point sensor to North, then press any key…");
  Serial.read();  // clear the keypress

  compass.read();            // fetch a sample
  offset = compass.getAzimuth();

  Serial.print("Captured offset = ");
  Serial.print(offset, 1);
  Serial.println("°");
  Serial.println();
}

void loop() {
  // 1) Read fresh data and get raw azimuth
  compass.read();
  int rawAz = compass.getAzimuth();

  // 2) Apply offset and wrap into [0,360)
  float calAz = rawAz - offset;
  if (calAz < 0)    calAz += 360.0;
  if (calAz >= 360) calAz -= 360.0;

  // 3) Get 16-point cardinal direction
  char dir[4] = {0};
  compass.getDirection(dir, (int)calAz);

  // 4) Print raw & calibrated headings plus X/Y/Z
  Serial.print("X="); Serial.print(compass.getX());
  Serial.print("  Y="); Serial.print(compass.getY());
  Serial.print("  Z="); Serial.print(compass.getZ());
  Serial.print("  Raw="); Serial.print(rawAz);  Serial.print("°");
  Serial.print("  Cal="); Serial.print(calAz,1); Serial.print("°");
  Serial.print("  Dir="); Serial.println(dir);

  delay(200);
}
