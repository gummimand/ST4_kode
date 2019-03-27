/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updated by chegewara

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/

//Denne kode kører ude på den perifære enhed.

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>

#include <M5Stack.h>
#include "utility/MPU9250.h"

MPU9250 IMU;
// Linje 37 - 47 handler om at sætte nogle parametre ift. BLE

BLEServer* pServer = NULL;  //* er en pointer, her til en BLE server
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "d25d6d42-8e94-4e65-a6f0-0ae9f9b2cb65"
#define CHARACTERISTIC_UUID "45fedfed-0104-474d-8ee3-cc7d867a7971"

//Function declaration
void M5_IMU_read(); //Fortæller at vi har en funktion, som hedder dette, men specificere først senere, hvad det er den gør, uden dette
// ville den klage over, at der i koden ikke var noget som hed dette.

// Laver en klasse som har nogle funktioner
class MyServerCallbacks: public BLEServerCallbacks { // et : laver en underklasse(??).
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);

  //M5 setup:
  M5.begin();
  Wire.begin();

// Ting der er nødvendig for at benytte IMU
  IMU.calibrateMPU9250(IMU.gyroBias, IMU.accelBias);
  IMU.initMPU9250();
  IMU.initAK8963(IMU.magCalibration);

  // Create the BLE Device
  BLEDevice::init("ESP32"); //BLEDevice er en overordnet klasse som har nogle properties, disse kan hentes ved at benytte ::

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // -> Hent data ud, så vi har objektet og brug funktionen på dette (Læs det er der her)

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristic->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising - annoncere nu er jeg klar, sender navn ud osv.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  // If intPin goes high, all data registers have new data
  // On interrupt, check if data ready interrupt
    M5_IMU_read(); //Kaldes for at få data fra IMU

    //Print values
    Serial.print((int)(1000000*IMU.ax));
    Serial.print(' ');
    Serial.print((int)(1000000*IMU.ay));
    Serial.print(' ');
    Serial.print((int)(1000000*IMU.az));
    Serial.println(' ');

    int transmit_Value[3] = {(1000*IMU.az), (1000*IMU.ay), (1000*IMU.ax)}; //Array, som er 3 lang, hvis vi sætter ax, ay og az værdierne

    // notify changed value
    if (deviceConnected) { //Hvis device er connected, så sæt en værdi, som kan notify til client
        pCharacteristic->setValue((uint8_t*) &transmit_Value, sizeof(int)*3); // sixeof(int) er 4, så ganget med 3 giver det 12
        pCharacteristic->notify();
        //value++;
        delay(50); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) { // && betyder and
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

void M5_IMU_read() { // Denne funktion læser og skriver data på M5Stack
  if (IMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
  {
    // For accelerometer
    IMU.readAccelData(IMU.accelCount); // . bruges når man har objektet og skal ind i det
    IMU.getAres();

    IMU.ax = (float)IMU.accelCount[0] * IMU.aRes; // - accelBias[0];
    IMU.ay = (float)IMU.accelCount[1] * IMU.aRes; // - accelBias[1];
    IMU.az = (float)IMU.accelCount[2] * IMU.aRes; // - accelBias[2];

    // For gyroskop
    IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z adc values
    IMU.getGres();

    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    IMU.gx = (float)IMU.gyroCount[0] * IMU.gRes;
    IMU.gy = (float)IMU.gyroCount[1] * IMU.gRes;
    IMU.gz = (float)IMU.gyroCount[2] * IMU.gRes;

    // For magnetometer
    IMU.readMagData(IMU.magCount);  // Read the x/y/z adc values
    IMU.getMres();
    // User environmental x-axis correction in milliGauss, should be
    // automatically calculated
    //IMU.magbias[0] = +470.;
    // User environmental x-axis correction in milliGauss TODO axis??
    //IMU.magbias[1] = +120.;
    // User environmental x-axis correction in milliGauss
    //IMU.magbias[2] = +125.;

    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    IMU.mx = (float)IMU.magCount[0] * IMU.mRes * IMU.magCalibration[0] -
             IMU.magbias[0];
    IMU.my = (float)IMU.magCount[1] * IMU.mRes * IMU.magCalibration[1] -
             IMU.magbias[1];
    IMU.mz = (float)IMU.magCount[2] * IMU.mRes * IMU.magCalibration[2] -
             IMU.magbias[2];

    //IMU.tempCount = IMU.readTempData();  // Read the adc values
    // Temperature in degrees Centigrade
    //IMU.temperature = ((float) IMU.tempCount) / 333.87 + 21.0;

    int x=64+10;
    int y=128+20;
    int z=192+30;

    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextColor(GREEN , BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0); M5.Lcd.print("MPU9250/AK8963");
    M5.Lcd.setCursor(0, 32); M5.Lcd.print("x");
    M5.Lcd.setCursor(x, 32); M5.Lcd.print("y");
    M5.Lcd.setCursor(y, 32); M5.Lcd.print("z");

    M5.Lcd.setTextColor(YELLOW , BLACK);
    M5.Lcd.setCursor(0, 48 * 2); M5.Lcd.print((int)(1000 * IMU.ax));
    M5.Lcd.setCursor(x, 48 * 2); M5.Lcd.print((int)(1000 * IMU.ay));
    M5.Lcd.setCursor(y, 48 * 2); M5.Lcd.print((int)(1000 * IMU.az));
    M5.Lcd.setCursor(z, 48 * 2); M5.Lcd.print("mg");

    M5.Lcd.setCursor(0, 64 * 2); M5.Lcd.print((int)(IMU.gx));
    M5.Lcd.setCursor(x, 64 * 2); M5.Lcd.print((int)(IMU.gy));
    M5.Lcd.setCursor(y, 64 * 2); M5.Lcd.print((int)(IMU.gz));
    M5.Lcd.setCursor(z, 64 * 2); M5.Lcd.print("o/s");

    M5.Lcd.setCursor(0, 80 * 2); M5.Lcd.print((int)(IMU.mx));
    M5.Lcd.setCursor(x, 80 * 2); M5.Lcd.print((int)(IMU.my));
    M5.Lcd.setCursor(y, 80 * 2); M5.Lcd.print((int)(IMU.mz));
    M5.Lcd.setCursor(z, 80 * 2); M5.Lcd.print("mG");

    M5.Lcd.setCursor(0,  96 * 2); M5.Lcd.print("Gyro Temperature ");
    M5.Lcd.setCursor(z,  96 * 2); M5.Lcd.print(IMU.temperature, 1);
    M5.Lcd.print(" C");
  }
}
