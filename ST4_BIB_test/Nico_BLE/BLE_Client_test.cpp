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
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("45fedfed-0104-474d-8ee3-cc7d867a7971");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

//Callback for notify from periferal unit
static void notifyCallback( // Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    int arrayLength = length/sizeof(int);
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

  /*
  //terminal
  Serial.print(length);
  for (int i = sizeof(myXYZ)/sizeof(int)-1; i >= 0; i--) {
    Serial.print(myXYZ[i]);

  }
  */


  //serial plotter in arduino
  for (int i = 0; i < sizeof(myXYZ)/sizeof(int)-2; i+=3) {
    Serial.print(myXYZ[i]);
    Serial.print(" ");
    Serial.print(myXYZ[i+1]);
    Serial.print(" ");
    Serial.println(myXYZ[i+2]);
  }


// Dette er til 3 kanaler - det er vigtigt, at den sender de højeste først når det skal ind i Matlab.
// Dette gør, at den sender z-værdierne først

}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    //Serial.println("onDisconnect");
  }
};

// Redundant connection routine.
bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient*  pClient  = BLEDevice::createClient();
    //Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    //Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      //Serial.print("Failed to find our service UUID: ");
      //Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      //Serial.print("Failed to find our characteristic UUID: ");
      //Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //Serial.print("The characteristic value was: ");
      //Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    //Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

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
  //Serial.println("Starting Arduino BLE Client application...");
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
      Serial.println("We are now connected to the BLE Server.");
    } else {
      //Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    ////Serial.println("Setting new characteristic value to \"" + newValue + "\"");

    // Set the characteristic's value to be the array of bytes that is actually a string.
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    //Serial.println("Device is connected!");
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
