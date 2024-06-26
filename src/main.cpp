#include <Arduino.h>
#include <Adafruit_MPL3115A2.h>
#include <Adafruit_LSM6DSOX.h>
#include <Adafruit_LIS3MDL.h>

#include "SensorDataHandler.h"
#include "flightstatus.h"
#include <SD.h>

#define DEBUG Serial

Adafruit_MPL3115A2 baro;
Adafruit_LSM6DSOX sox;
Adafruit_LIS3MDL mag;

// ----------------
// Initalization of data handlers
// ----------------
// First parameter is the interval between each data point in milliseconds
// Second parameter is the size of the temporal array in milliseconds (i.e. hold old data is the oldest data point)
// Third parameter is the name of the data

SensorData xAccelData(50, 1000, "xac");
SensorData yAccelData(50, 1000, "yac");
SensorData zAccelData(50, 1000, "zac");

SensorData xGyroData(500, 1000, "xgy");
SensorData yGyroData(500, 1000, "ygy");
SensorData zGyroData(500, 1000, "zgy");

// Storing these at a slower rate b/c less important
SensorData temperatureData(500, 1000, "tmp"); 

FlightStatus flightStatus(&xAccelData, &yAccelData, &zAccelData);

// SensorData *dataHandlers[] = {&altitudeData, &xAccelData, &yAccelData, &zAccelData, &xGyroData, &yGyroData, &zGyroData, &temperatureData, &pressureData, &xMagData, &yMagData, &zMagData};

// For the serial SD card logger
HardwareSerial SD_serial(PB7, PB6); // RX, TX

int last_led_toggle = 0;

void setup(void) {
  
  pinMode(PA9, OUTPUT);
  
  // Uncomment this to see the debug messages
  // Serial.begin(115200);
  // while (!Serial)
  //   delay(10); // will pause Zero, Leonardo, etc until serial console opens

  
  flightStatus.setupSDHs();

  SD_serial.begin(115200);
  while (!SD_serial)
    delay(10);

  Serial.println("All serial communication is setup");

  // Start the SPI SD card
  SD.begin(PA4);



  // Run this to test the data handler, will stop the program after it finishes
  // test_DataHandler();

  TwoWire *wire = new TwoWire(PB11, PB10);

  Serial.println("Setting up barometer...");

  if (!baro.begin(wire)) {
    Serial.println("Could not find sensor. Check wiring.");
    while (1);
  }

  baro.setMode(MPL3115A2_ALTIMETER);
  baro.setSeaPressure(240); // Adjust this to your local forecast!

  temperatureData.restrictSaveSpeed(1000); // Save temperature data every second

  // Setting the barometer to altimeter
  // baro.setMode(MPL3115A2_ALTIMETER);
  Serial.println("Setting up accelerometer and gyroscope...");
  while (!sox.begin_I2C(0x6A, wire)) {
    Serial.println("Could not find sensor. Check wiring.");
    delay(10);
  }

  Serial.println("Setting ACL and Gyro ranges and data rates...");
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS );

  sox.setAccelDataRate(LSM6DS_RATE_104_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_104_HZ);

  // If the range is not set correctly, then print a message
  if (sox.getAccelRange() != LSM6DS_ACCEL_RANGE_16_G) {
    Serial.println("Failed to set ACL range");
  }
  if (sox.getGyroRange() != LSM6DS_GYRO_RANGE_2000_DPS) {
    Serial.println("Failed to set Gyro range");
  }
  if (sox.getAccelDataRate() != LSM6DS_RATE_104_HZ) {
    Serial.println("Failed to set ACL data rate");
  }
  if (sox.getGyroDataRate() != LSM6DS_RATE_104_HZ) {
    Serial.println("Failed to set Gyro data rate");
  }

  // Setup for the magnetometer
  // Serial.println("Setting up magnetometer...");
  // while (!mag.begin_I2C(0x1E, wire)) {
  //   Serial.println("Could not find sensor. Check wiring.");
  //   delay(10);
  // }
  // mag.setDataRate(LIS3MDL_DATARATE_155_HZ);
  // mag.setRange(LIS3MDL_RANGE_4_GAUSS);
  // mag.setOperationMode(LIS3MDL_SINGLEMODE);
  // mag.setPerformanceMode(LIS3MDL_MEDIUMMODE);

  // mag.setIntThreshold(500);
  // mag.configInterrupt(false, false, true, // enable z axis
  //                         true, // polarity
  //                         false, // don't latch
  //                         true); // enabled!

  // if (mag.getDataRate() != LIS3MDL_DATARATE_155_HZ) {
  //   Serial.println("Failed to set Mag data rate");
  // }
  // test_DataHandler();
  Serial.println("Setup Complete!!!");
}

void loop() {
  int toggle_delay = 500;
  if (flightStatus.getStage() > ARMED) {
    toggle_delay = 50;
  }
  uint32_t current_time = millis();
  if (current_time - last_led_toggle > toggle_delay) {
    last_led_toggle = millis();
    digitalWrite(PA9, !digitalRead(PA9));
  }

  sensors_event_t accel;
  sensors_event_t gyro;
  sensors_event_t temp;
  sox.getEvent(&accel, &gyro, &temp);

  xAccelData.addData(DataPoint(current_time, accel.acceleration.x), &SD_serial);
  yAccelData.addData(DataPoint(current_time, accel.acceleration.y), &SD_serial);
  zAccelData.addData(DataPoint(current_time, accel.acceleration.z), &SD_serial);

  xGyroData.addData(DataPoint(current_time, gyro.gyro.x), &SD_serial);
  yGyroData.addData(DataPoint(current_time, gyro.gyro.y), &SD_serial);
  zGyroData.addData(DataPoint(current_time, gyro.gyro.z), &SD_serial);

  temperatureData.addData(DataPoint(current_time, temp.temperature), &SD_serial);

  flightStatus.update(&SD_serial);
}