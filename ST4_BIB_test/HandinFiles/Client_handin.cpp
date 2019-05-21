/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara
 */

#define M5ACTIVE 1
#define NOTE_DH2 330

#include "BLEDevice.h"
//#include "BLEScan.h"
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <M5Stack.h>
#include <timer_table.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUID("d25d6d42-8e94-4e65-a6f0-0ae9f9b2cb65");

// The characteristic of the remote services we are interested in. One for each type of event identified by the server.
static BLEUUID    charUUID_START("45fedfed-0104-474d-8ee3-cc7d867a7971");
static BLEUUID    charUUID_STOP("2cc28937-73d0-4694-9462-94e3a1d85247");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLERemoteCharacteristic* pRemoteCharacteristic_start;
static BLERemoteCharacteristic* pRemoteCharacteristic_stop;

static BLEAdvertisedDevice* myDevice;

//Function predefine
void setupBeepTimer();


//Timer and interrupt variables for beeps
volatile int interruptCounter;
int time_interval=1000000/10;
bool interrupt_occured= false;

hw_timer_t * timerBeep = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

int index_counter = 0;
int lastBeepAt = 0;
int eventCounter = 1; //Starts on one to sync start beep with interrupts. Counts start/stop, to identify location of an event.

// variables for eventdata to matlab. Dem med 'Byte' kan laves som static -> kan det være en fordel?
uint8_t person1Byte=1;
uint8_t startByte=1;
uint8_t stopByte=3;
uint8_t startCounter = 1;
int sumForTest=0;

int bytesAvailable=0; //Variable to communicate with matlab
bool testRunning=0;

//ISR for beep interrupt
void IRAM_ATTR onTimerBeep() {
  portENTER_CRITICAL_ISR(&timerMux);
  interrupt_occured=true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

//Callback for notify from periferal unit
// Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
static void notifyCallback_start(BLERemoteCharacteristic* pBLERemoteCharacteristic_start, uint8_t* pData, size_t length, bool isNotify) {
  int recievedData_array[length/sizeof(int)]; // length er antal bytes fra notify og sizeof(int) er størrelsen på en int (4byte)
  uint8_t *pHelp;
  pHelp=(uint8_t*) &(recievedData_array[0]);  // pH peger på recievedData_array, hvor data skal lægges
  double eventTime;
  bool eventOK=false;
  if (testRunning){

    eventTime = (double)interruptCounter/10;//Time for event notification


    //Checks if event occurred at correct time
    if (eventCounter % 3 == 1 || eventCounter % 3 == 2) //Start is after start beep or after turn beep
      eventOK=true;
    else
      eventOK=false;

    //Fylder recievedData_array op 1 byte ad gangen, da pHelp peger på recievedData_array.
    for (int i=0; i<length; i++){
      pHelp[i]=pData[i];
    }

    //Send til matlab
    Serial.write(person1Byte); //Hvilken person er det
    Serial.write(startByte); //Hvilket event er det
    Serial.write(startCounter); //Hvilket level er det
    Serial.write((uint8_t)eventOK); //Blev eventet accepteret?

  }
}

// Kalder notify fra BLE_notify - Den opsamler data fra notifyeren
static void notifyCallback_stop(BLERemoteCharacteristic* pBLERemoteCharacteristic_stop, uint8_t* pData, size_t length, bool isNotify) {
  int recievedData_array[length/sizeof(int)]; // length er antal bytes fra notify og sizeof(int) er størrelsen på en int (4byte)
  uint8_t *pHelp;
  pHelp=(uint8_t*) &(recievedData_array[0]);  // pH peger på recievedData_array, hvor data skal lægges
  double eventTime;
  bool eventOK=false;
  if (testRunning){

    eventTime = (double)interruptCounter/10;

    if (eventCounter % 3 == 0)
      eventOK=false; //The beep was before stopping -> the player didn't make the interval.
    else if(eventCounter % 3 == 2)
      eventOK = true; //The stop was after turn beep but before stop beep.
    else
      //Serial.println("Something is out of sync.. stop identified too early"); //Stop is before turn event, and hence something is wrong.


    for (int i=0; i<length; i++){
    	pHelp[i]=pData[i]; //Fylder recievedData_array op 1 byte ad gangen, da pHelp peger på recievedData_array.
    }

    //Send til matlab
    Serial.write(person1Byte);
    Serial.write(stopByte);
    Serial.write(startCounter);
    Serial.write((uint8_t)eventOK);

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

    BLEAddress addressTest =  myDevice->getAddress();

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

    // Obtain a reference to the characteristic in the service of the remote BLE server. Her starter stop
    pRemoteCharacteristic_stop = pRemoteService->getCharacteristic(charUUID_STOP);
    if (pRemoteCharacteristic_stop == nullptr) {
      pClient->disconnect();
      return false;
    }
    //Serial.println(" - Found our characteristic-stop");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic_stop->canRead()) {
      std::string value = pRemoteCharacteristic_stop->readValue();
    }


    //Register callbacks for notification for every characteristic
    if(pRemoteCharacteristic_start->canNotify())
      pRemoteCharacteristic_start->registerForNotify(notifyCallback_start);

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
      //Serial.println("Found advertised device");
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
    M5.begin(true, false, false);//Enable LCD, disable SD, disable Serial(since it has begun)
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

  setupBeepTimer();
} // End of setup.


// This is the Arduino main loop function.
void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      uint8_t readyValue = 10;//Notify matlab that Client is ready.
      Serial.write(readyValue);
      Serial.write(readyValue);
      Serial.write(readyValue);
      Serial.write(readyValue);
    }

    else {
    }
    doConnect = false;
  }

  if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  if (interrupt_occured) { //Maybe change to while(interrupt_occured > 0){..}, to counter misses.
    portENTER_CRITICAL(&timerMux);
    interrupt_occured=false;
    portEXIT_CRITICAL(&timerMux);

    if (IR1_table[interruptCounter]){
      #if M5ACTIVE
      M5.Speaker.tone(NOTE_DH2, 150);
      #endif
      if (lastBeepAt+4 < (interruptCounter)) {
        eventCounter++;
      }
      startCounter=(eventCounter+2)/3;
      lastBeepAt = interruptCounter;
    }
    interruptCounter++;
  }

  bytesAvailable = Serial.available();
  if(bytesAvailable) {
    char readData = (char)Serial.read();
    switch (readData) {
      case 'S':
          testRunning = true;
          timerAlarmEnable(timerBeep);//The timer will begin when connected
        break;
      case 'E':
        if (testRunning) {
          testRunning = false;
          timerAlarmDisable(timerBeep);//Stop beeps
        }
        break;
      default:
        break;
    }
  }
  #if M5ACTIVE
    M5.update();
  #endif
} // End of loop


void setupBeepTimer() {
  timerBeep = timerBegin(0, 80, true);
  timerAttachInterrupt(timerBeep, &onTimerBeep, true);
  timerAlarmWrite(timerBeep, time_interval, true);

}
