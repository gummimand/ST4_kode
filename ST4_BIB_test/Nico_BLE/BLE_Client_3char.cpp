/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#define M5ACTIVE 0

#include "BLEDevice.h"
//#include "BLEScan.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <M5Stack.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUID("d25d6d42-8e94-4e65-a6f0-0ae9f9b2cb65");

// The characteristic of the remote services we are interested in. One for each type of event identified by the server. 
static BLEUUID    charUUID_START("45fedfed-0104-474d-8ee3-cc7d867a7971");
static BLEUUID    charUUID_TURN("4b6cbf78-8673-4014-8e2e-f92e5ebc9d5c");
static BLEUUID    charUUID_STOP("2cc28937-73d0-4694-9462-94e3a1d85247");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLERemoteCharacteristic* pRemoteCharacteristic_start;
static BLERemoteCharacteristic* pRemoteCharacteristic_turn;
static BLERemoteCharacteristic* pRemoteCharacteristic_stop;

static BLEAdvertisedDevice* myDevice;

//Callback for notify from periferal unit
static void notifyCallback_start( // Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
  BLERemoteCharacteristic* pBLERemoteCharacteristic_start,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  //Write to matlab

  /*
  for (int i = length-1; i >= 0; i--) {
    Serial.write(pData[i]);
  }*/

  //Write to terminal + serial plotter
  int myXYZ[length/sizeof(int)]; // length er antal bytes fra notify og sizeof(int) er størrelsen på en int (4byte)
  uint8_t *pHelp;
  pHelp=(uint8_t*) &(myXYZ[0]);  // pH peger på myXYZ, hvor data skal lægges

  for (int i=0; i<length; i++){
  	pHelp[i]=pData[i]; //Fylder myXYZ op 1 byte ad gangen, da pHelp peger på myXYZ.
  }

  
  //terminal
  Serial.print(length);
  for (int i = sizeof(myXYZ)/sizeof(int)-1; i >= 0; i--) {
    Serial.print(myXYZ[i]);

  }
  

  }

static void notifyCallback_turn( // Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
  BLERemoteCharacteristic* pBLERemoteCharacteristic_turn,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  //Write to matlab

  /*
  for (int i = length-1; i >= 0; i--) {
    Serial.write(pData[i]);
  }*/
 
   //Write to terminal + serial plotter
  int myXYZ[length/sizeof(int)]; // length er antal bytes fra notify og sizeof(int) er størrelsen på en int (4byte)
  uint8_t *pHelp;
  pHelp=(uint8_t*) &(myXYZ[0]);  // pH peger på myXYZ, hvor data skal lægges

  for (int i=0; i<length; i++){
  	pHelp[i]=pData[i]; //Fylder myXYZ op 1 byte ad gangen, da pHelp peger på myXYZ.
  }

  
  //terminal
  Serial.print(length);
  for (int i = sizeof(myXYZ)/sizeof(int)-1; i >= 0; i--) {
    Serial.print(myXYZ[i]);

  }
  
  }


static void notifyCallback_stop( // Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
  BLERemoteCharacteristic* pBLERemoteCharacteristic_stop,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  //Write to matlab

  /*
  for (int i = length-1; i >= 0; i--) {
    Serial.write(pData[i]);
  }*/

  //Write to terminal + serial plotter
  int myXYZ[length/sizeof(int)]; // length er antal bytes fra notify og sizeof(int) er størrelsen på en int (4byte)
  uint8_t *pHelp;
  pHelp=(uint8_t*) &(myXYZ[0]);  // pH peger på myXYZ, hvor data skal lægges

  for (int i=0; i<length; i++){
  	pHelp[i]=pData[i]; //Fylder myXYZ op 1 byte ad gangen, da pHelp peger på myXYZ.
  }

  
  //terminal
  Serial.print(length);
  for (int i = sizeof(myXYZ)/sizeof(int)-1; i >= 0; i--) {
    Serial.print(myXYZ[i]);

  }
  

  }


class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
  }
};

// Redundant connection routine.
bool connectToServer() {
    BLEClient*  pClient  = BLEDevice::createClient();

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {

      pClient->disconnect();
      return false;
    }
    


    // Obtain a reference to the characteristic in the service of the remote BLE server. Her starter start
    pRemoteCharacteristic_start = pRemoteService->getCharacteristic(charUUID_START);
    if (pRemoteCharacteristic_start == nullptr) {

      pClient->disconnect();
      return false;
    }
  

    // Read the value of the characteristic.
    if(pRemoteCharacteristic_start->canRead()) {
      std::string value = pRemoteCharacteristic_start->readValue();

    }

    if(pRemoteCharacteristic_start->canNotify())
      pRemoteCharacteristic_start->registerForNotify(notifyCallback_start);

// Obtain a reference to the characteristic in the service of the remote BLE server. Her starter turn 
    pRemoteCharacteristic_turn = pRemoteService->getCharacteristic(charUUID_TURN);
    if (pRemoteCharacteristic_turn == nullptr) {

      pClient->disconnect();
      return false;
    }
  

    // Read the value of the characteristic.
    if(pRemoteCharacteristic_turn->canRead()) {
      std::string value = pRemoteCharacteristic_turn->readValue();

    }

    if(pRemoteCharacteristic_turn->canNotify())
      pRemoteCharacteristic_turn->registerForNotify(notifyCallback_turn);

// Obtain a reference to the characteristic in the service of the remote BLE server. Her starter stop
    pRemoteCharacteristic_stop = pRemoteService->getCharacteristic(charUUID_STOP);
    if (pRemoteCharacteristic_stop == nullptr) {

      pClient->disconnect();
      return false;
    }
  

    // Read the value of the characteristic.
    if(pRemoteCharacteristic_stop->canRead()) {
      std::string value = pRemoteCharacteristic_stop->readValue();

    }

    if(pRemoteCharacteristic_stop->canNotify())
      pRemoteCharacteristic_stop->registerForNotify(notifyCallback_stop);

    connected = true;
}

/*
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks


void setup() {
  Serial.begin(115200);
  
  BLEDevice::init("");
  #if M5ACTIVE
    M5.begin(true, true, false);
  #endif
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      
    } else {
      
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic_start->writeValue(newValue.c_str(), newValue.length());
    pRemoteCharacteristic_turn->writeValue(newValue.c_str(), newValue.length());
    pRemoteCharacteristic_stop->writeValue(newValue.c_str(), newValue.length());
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  #if M5ACTIVE
    int bytesAvailable = Serial.available();
    if(bytesAvailable) {
      char readData = (char)Serial.read();
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(GREEN , BLACK);
      M5.Lcd.setTextSize(10);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print(readData);
    }
  #endif

  delay(1000); // Delay a second between loops.
} // End of loop
