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
#include "MPU9250.h"

#define IMU_UNIT 0
#if IMU_UNIT
MPU9250 IMU(Wire,0x68);
#endif
// Linje 37 - 47 handler om at sætte nogle parametre ift. BLE

BLEServer* pServer = NULL;  //* er en pointer, her til en BLE server
BLECharacteristic* pCharacteristic_start = NULL;
BLECharacteristic* pCharacteristic_turn = NULL;
BLECharacteristic* pCharacteristic_stop = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int valueArray[39]={0};
int valueArrayLength = (sizeof(valueArray)/sizeof(int)); //size of array in bytes divided by size of int in bytes = length of array.
uint32_t sample_count = 0;
bool flag_ADC=false;
int sF = 50;

bool startIdentified=true;
bool turnIdentified=false;
bool stopIdentified=false;



//Interrupt timer variables
hw_timer_t* timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "d25d6d42-8e94-4e65-a6f0-0ae9f9b2cb65"
#define CHARACTERISTIC_UUID_START "45fedfed-0104-474d-8ee3-cc7d867a7971"
#define CHARACTERISTIC_UUID_TURN "4b6cbf78-8673-4014-8e2e-f92e5ebc9d5c"
#define CHARACTERISTIC_UUID_STOP "2cc28937-73d0-4694-9462-94e3a1d85247"

/*Function declaration
Fortæller at vi har en funktion, som hedder dette, men specificere først senere, hvad det er den gør, uden dette
ville den klage over, at der i koden ikke var noget som hed dette.
*/
void M5_IMU_read_ADC(); //Read from hardware (ADC)
void setFlagADC();
void storeADCData(); //Store data in array
void M5_wakeup(); //Display "!" to announce wakeup
void onTimer(); //Interrupt callback function
void setupTimer();
void setupBLE();

// Laver en klasse som har nogle funktioner
class MyServerCallbacks: public BLEServerCallbacks { // et : laver en underklasse(??).
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Connected!");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};





void setup() {
  Serial.begin(115200);
  setupTimer();
  //M5 setup:
  M5.begin();
  Wire.begin();

  // Ting der er nødvendig for at benytte IMU
  #if IMU_UNIT
    int status = IMU.begin();
    if (status < 0) {
     Serial.println("IMU initialization unsuccessful");
    }
    // setting the accelerometer full scale range to +/-8G
    IMU.setAccelRange(MPU9250::ACCEL_RANGE_4G);
    // setting the gyroscope full scale range to +/-500 deg/s
    IMU.setGyroRange(MPU9250::GYRO_RANGE_2000DPS);
    // setting DLPF bandwidth to 20 Hz
    IMU.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
    // setting SRD to 1 for a 500 Hz update rate (SAMPLE_RATE=1000/(1+SRD))
    IMU.setSrd(0);
  #endif

  setupBLE();
  M5_wakeup();
  timerAlarmEnable(timer); //Enable interrupt timer
}




void loop() {
  if(flag_ADC && deviceConnected){
    //int startTime=micros();
    M5_IMU_read_ADC(); //Kaldes for at få data fra IMU - Takes approx 560µs
    storeADCData();
    portENTER_CRITICAL(&timerMux);
    flag_ADC=false;
    portEXIT_CRITICAL(&timerMux);
  }




  // notify on start event identified
  if (deviceConnected && startIdentified ) { //Hvis device er connected, så sæt en værdi, som kan notify til client og send pakker af 39
      pCharacteristic_start->setValue((uint8_t*)1, sizeof(int)); //
      pCharacteristic_start->notify();
      startIdentified = false;
      turnIdentified = true;

      delay(500);
  }
  // notify on turn event identified
  if (deviceConnected && turnIdentified ) { //Hvis device er connected, så sæt en værdi, som kan notify til client og send pakker af 39
      pCharacteristic_turn->setValue((uint8_t*)2, sizeof(int)); //
      pCharacteristic_turn->notify();
      turnIdentified = false;
      stopIdentified = true;

      delay(500);
  }
  // notify on stop event identified
  if (deviceConnected && stopIdentified ) { //Hvis device er connected, så sæt en værdi, som kan notify til client og send pakker af 39
      pCharacteristic_stop->setValue((uint8_t*)3, sizeof(int)); //
      pCharacteristic_stop->notify();
      stopIdentified = false;
      startIdentified = true;
      delay(500);
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

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  setFlagADC();
  portEXIT_CRITICAL_ISR(&timerMux);
}

void M5_wakeup() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(GREEN , BLACK);
  M5.Lcd.setTextSize(10);
  M5.Lcd.setCursor(0, 0); M5.Lcd.print("!");
  delay(1000);
  M5.Lcd.writecommand(ILI9341_DISPOFF);
  M5.Lcd.setBrightness(0);
}

void IRAM_ATTR setFlagADC() {
  flag_ADC=true;
}

void IRAM_ATTR M5_IMU_read_ADC() { // Denne funktion læser og skriver data på M5Stack og gemmes i IRAM
  #if IMU_UNIT
    IMU.readSensor();
  #endif
}

void IRAM_ATTR storeADCData(){
  #if IMU_UNIT
    valueArray[sample_count++] = (int)1000*IMU.getAccelZ_mss();//in cm/s^2
    valueArray[sample_count++] = (int)1000*IMU.getAccelY_mss();//increment sample_count and store read data in array.
    valueArray[sample_count++] = (int)1000*IMU.getAccelX_mss();
    //sample_count++;
  #endif
}

void setupTimer() {
  //Setup interrupt timer as from: https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
  timer = timerBegin(0, 80, true); // 0 is timer nr. 80 is prescaler, and allows ms to be set in timerAlarmWrite(), true is count upwards
  timerAttachInterrupt(timer, &onTimer, true); //timer handle, callback funtion and interrupt on edge type(true) level(false)
  timerAlarmWrite(timer, 1000000*1/sF, true); //timer handle, micro-s between interrupts, true to make interrupt reload(means it runs repeatedly)
}

void setupBLE() {
  // Create the BLE Device
  BLEDevice::init("ESP32"); //BLEDevice er en overordnet klasse som har nogle properties, disse kan hentes ved at benytte ::

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // -> Hent data ud, så vi har objektet og brug funktionen på dette (Læs det er der her)

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic for start identification
  pCharacteristic_start = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_START,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Characteristic for turn identification
  pCharacteristic_turn = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TURN,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // Create a BLE Characteristic for stop identification
  pCharacteristic_stop = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_STOP,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor for start, turn, stop characteristics
  pCharacteristic_start->addDescriptor(new BLE2902());
  pCharacteristic_turn->addDescriptor(new BLE2902());
  pCharacteristic_stop->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising - annoncere nu er jeg klar, sender navn ud osv.
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Setup complete. \nWaiting a client connection to notify...");
}
