#include <Arduino.h>
#include <Wire.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>

// I2C address and register definitions for LSM6DSO
#define LSM6DSO_ADDR 0x6B
#define CTRL1_XL     0x10
#define OUTX_L_A     0x28

void writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(LSM6DSO_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

int16_t read16bitRegister(uint8_t regL) {
  const int maxRetries = 3;
  for (int retry = 0; retry < maxRetries; retry++) {
    Wire.beginTransmission(LSM6DSO_ADDR);
    Wire.write(regL);
    if (Wire.endTransmission(false) != 0) {
      delay(40);
      continue;
    }
    delay(40);
    if (Wire.requestFrom((uint16_t)LSM6DSO_ADDR, (uint8_t)2) < 2) {
      delay(40);
      continue;
    }
    uint8_t low = Wire.read();
    uint8_t high = Wire.read();
    return (int16_t)((high << 8) | low);
  }
  return 0;
}

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic = nullptr;

class MyBLECallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {}
};

// Activity detection variables
static const float RESET_THRESH_G = 0.85F;
volatile unsigned long stepCount = 0;
volatile unsigned long jumpCount = 0;
bool overThreshold = false;

// Setup BLE server and advertising
void setupBLE() {
  BLEDevice::init("SDSUCS-Monitor");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );

  pCharacteristic->setCallbacks(new MyBLECallbacks());
  pCharacteristic->setValue("steps:0,jumps:0");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE advertising started...");
}

// Setup IMU sensor (LSM6DSO)
void setupIMU() {
  Wire.begin(21, 22); // SDA = GPIO21, SCL = GPIO22
  Wire.setClock(100000);
  delay(100);
  writeRegister(CTRL1_XL, 0x40); // 104Hz, ±2g
  delay(100);
  Serial.println("IMU initialized.");
}

// Detect steps and jumps based on magnitude and Z-axis
void detectActivity() {
  unsigned long t_start = millis();  // Start timestamp

  int16_t ax_raw = read16bitRegister(OUTX_L_A);
  int16_t ay_raw = read16bitRegister(OUTX_L_A + 2);
  int16_t az_raw = read16bitRegister(OUTX_L_A + 4);

  float scale = 0.000061; 
  float ax = ax_raw * scale;
  float ay = ay_raw * scale;
  float az = az_raw * scale;

  float mag = sqrt(ax * ax + ay * ay + az * az);

  if (!overThreshold) {
    if (mag > 1.35 && az > 1.2) {
      jumpCount++;
      overThreshold = true;
      unsigned long detectionTime = millis() - t_start;
      Serial.println(">>> JUMP DETECTED");
      Serial.print("BLE JUMP Detection Time: ");
      Serial.print(detectionTime);
      Serial.println(" ms");
      return;
    } else if (mag > 1.0 && az <= 1.2) {
      stepCount++;
      overThreshold = true;
      unsigned long detectionTime = millis() - t_start;
      Serial.println(">>> STEP DETECTED");
      Serial.print("BLE STEP Detection Time: ");
      Serial.print(detectionTime);
      Serial.println(" ms");
      return;
    }
  }

  if (mag < RESET_THRESH_G) {
    overThreshold = false;
  }

  Serial.print("Mag: ");
  Serial.print(mag, 2);
  Serial.print(" | Z: ");
  Serial.print(az, 2);
  Serial.print(" | Steps: ");
  Serial.print(stepCount);
  Serial.print(" | Jumps: ");
  Serial.println(jumpCount);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n--- BLE Activity Monitor ---");
  setupIMU();
  setupBLE();
}

void loop() {
  static unsigned long lastStep = 0;
  static unsigned long lastJump = 0;

  detectActivity();

  if (stepCount != lastStep || jumpCount != lastJump) {
    lastStep = stepCount;
    lastJump = jumpCount;

    char msg[32];
    snprintf(msg, sizeof(msg), "steps:%lu,jumps:%lu", stepCount, jumpCount);
    pCharacteristic->setValue(msg);
    pCharacteristic->notify();
  }

  delay(250);
}
