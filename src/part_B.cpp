#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <math.h>

// WiFi credentials
const char* ssid = "BLVD63";
const char* password = "sdBLVD63";

// Cloud endpoints
const char* serverUrl = "http://3.145.90.184:5000/sensor";
const char* resetUrl  = "http://3.145.90.184:5000/reset";

// LSM6DSO I2C setup
#define LSM6DSO_ADDR 0x6B
#define CTRL1_XL     0x10
#define OUTX_L_A     0x28

// Detection thresholds
static const float STEP_THRESHOLD = 1.08;
static const float JUMP_THRESHOLD_MAG = 1.35;
static const float JUMP_THRESHOLD_Z = 1.2;
static const float RESET_THRESH_G = 0.85F;

bool overThreshold = false;
unsigned long lastStepTime = 0;
unsigned long lastJumpTime = 0;

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

int16_t read16bitRegister(uint8_t regL) {
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(regL);
  Wire.endTransmission(false);
  Wire.requestFrom((uint16_t)LSM6DSO_ADDR, (uint8_t)2);
  uint8_t low = Wire.read();
  uint8_t high = Wire.read();
  return (int16_t)((high << 8) | low);
}

void setupIMU() {
  Wire.begin(21, 22);
  Wire.setClock(100000);
  delay(100);
  writeRegister(CTRL1_XL, 0x40);  
  delay(100);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println(WiFi.localIP());
}

void resetServerCounters() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(resetUrl);
    int httpResponseCode = http.POST("");
    Serial.print("Reset Response: ");
    Serial.println(httpResponseCode);
    http.end();
  }
}

void sendData(float ax, float ay, float az, float mag, unsigned long startCycleTime) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"ax\":" + String(ax, 4) + ",\"ay\":" + String(ay, 4) +
                  ",\"az\":" + String(az, 4) + ",\"mag\":" + String(mag, 4) + "}";

    unsigned long startTx = millis();
    int httpResponseCode = http.POST(json);
    unsigned long endTx = millis();
    unsigned long transmissionTime = endTx - startTx;
    unsigned long totalCycleTime = endTx - startCycleTime;

    Serial.print("Response status: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
      String responseBody = http.getString();
      Serial.println("Response Body: " + responseBody);
    }

    Serial.print("Data transmission time: ");
    Serial.print(transmissionTime);
    Serial.println(" ms");

    Serial.print("End-to-end detection time: ");
    Serial.print(totalCycleTime);
    Serial.println(" ms");

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}

void detectActivity() {
  unsigned long startCycle = millis();  

  int16_t ax_raw = read16bitRegister(OUTX_L_A);
  int16_t ay_raw = read16bitRegister(OUTX_L_A + 2);
  int16_t az_raw = read16bitRegister(OUTX_L_A + 4);

  float scale = 0.000061;
  float ax = ax_raw * scale;
  float ay = ay_raw * scale;
  float az = az_raw * scale;

  float mag = sqrt(ax * ax + ay * ay + az * az);
  unsigned long now = millis();

  if (!overThreshold) {
    if (mag > JUMP_THRESHOLD_MAG && az > JUMP_THRESHOLD_Z && now - lastJumpTime > 600) {
      Serial.println(">>> JUMP DETECTED");
      sendData(ax, ay, az, mag, startCycle);
      overThreshold = true;
      lastJumpTime = now;
    } else if (mag > STEP_THRESHOLD && az <= JUMP_THRESHOLD_Z && now - lastStepTime > 350) {
      Serial.println(">>> STEP DETECTED");
      sendData(ax, ay, az, mag, startCycle);
      overThreshold = true;
      lastStepTime = now;
    }
  }

  if (mag < RESET_THRESH_G) {
    overThreshold = false;
  }

  Serial.print("Mag: ");
  Serial.print(mag, 2);
  Serial.print(" | Z: ");
  Serial.println(az, 2);
}

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  resetServerCounters();
  setupIMU();
}

void loop() {
  detectActivity();
  delay(250);
}
