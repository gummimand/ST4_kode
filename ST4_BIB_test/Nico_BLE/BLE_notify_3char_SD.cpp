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


#define IMU_UNIT 1
#define M5ACTIVE 1
#define SDACTIVE 1
#define NOTIFYACTIVE 1
#define FILENAME_RAW "/data.txt"
#define FILENAME_FILTERED "/data_filtered.txt"

#if IMU_UNIT
  MPU9250 IMU(Wire,0x68);
#endif
// Linje 37 - 47 handler om at sætte nogle parametre ift. BLE

BLEServer* pServer = NULL;  //* er en pointer, her til en BLE server
BLECharacteristic* pCharacteristic = NULL;
BLECharacteristic* pCharacteristic_start = NULL;
BLECharacteristic* pCharacteristic_turn = NULL;
BLECharacteristic* pCharacteristic_stop = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;
int valueArray[1*25]={0};//Kanaler gange samples. //Warning: Array size must be dividable with 3.
int arraySize = sizeof(valueArray)/sizeof(int);
uint32_t sample_count = 0;
bool flag_ADC=false;
int sF = 50;
int interruptCounter=1;

bool startIdentified = false;
bool turnIdentified = false;
bool stopIdentified = false;
int threshold_StartStop = 1100;
int value1=1;
int value2=0;
int value3=0;

#define FILTER_LENGTH 4
double x[FILTER_LENGTH + 1] = {0,0,0,0,0};
double y[FILTER_LENGTH + 1] = {0,0,0,0,0};
double b[FILTER_LENGTH + 1] = {0.3705549358, 0.0000000000, -0.7411098715, 0.0000000000, 0.3705549358};
double a[FILTER_LENGTH + 1] = {1.0000000000, -1.5634341687, 0.4548488847, -0.0723023604, 0.1870198984};

#define FILTER_LENGTH2 2
double xe[FILTER_LENGTH2+1] = {0,0,0};
double ye[FILTER_LENGTH2+1] = {0,0,0};
double be[FILTER_LENGTH2 + 1] = {0.0036216815, 0.0072433630, 0.0036216815};
double ae[FILTER_LENGTH2 + 1] = {1.0000000000, -1.8226949252, 0.8371816513};


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

void Bpfilter(int samplesToFilter);
int BpIIR_filter(int value);
void Envfilter(int samplesToFilter);
int EnvIIR_filter(int value);

void WriteToSDcard_raw(int samplesToWrite);
void WriteToSDcard_filtered(int samplesToWrite);
void writeFile(fs::FS &fs, const char * path, const char * message);
void appendFile(fs::FS &fs, const char * path, const char * message);
String int2str(int inArray[], int size);

// Laver en klasse som har nogle funktioner
class MyServerCallbacks: public BLEServerCallbacks { // et : laver en underklasse(??).
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Device connected");
      delay(200);//To syncronize communication with client
      #if NOTIFYACTIVE
        stopIdentified = true; //Start identifying events
      #endif
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      startIdentified = false; //Stop all identifying
      turnIdentified = false;
      stopIdentified = false;
    }
};


void setup() {
  Serial.begin(115200);
  setupTimer();
  //M5 setup:
  #if M5ACTIVE
    M5.begin(true, true, false);//Enable LCD, Enable SD, disable Serial(since it has begun)
  #endif
  Wire.begin();

  // Ting der er nødvendig for at benytte IMU
  #if IMU_UNIT
    int status = IMU.begin();
    if (status < 0) {
     Serial.println("IMU initialization unsuccessful");
    }
    // setting the accelerometer full scale range to +/-8G
    IMU.setAccelRange(MPU9250::ACCEL_RANGE_16G);
    // setting the gyroscope full scale range to +/-500 deg/s
    IMU.setGyroRange(MPU9250::GYRO_RANGE_2000DPS);
    // setting DLPF bandwidth to 20 Hz
    IMU.setDlpfBandwidth(MPU9250::DLPF_BANDWIDTH_20HZ);
    // setting SRD to 1 for a 500 Hz update rate (SAMPLE_RATE=1000/(1+SRD))
    IMU.setSrd(0);
  #endif

  setupBLE();

  #if M5ACTIVE
    M5_wakeup();
  #endif
  //Initialize raw datafile on SD card WARNING deletes previous file on every reset
  writeFile(SD, FILENAME_RAW, "Raw data collected from M5Stack - Group ST4 4401. Seperated by comma: [Z,Y,X]\n");//Header for data file
  //Initialize filtered datafile on SD card WARNING deletes previous file on every reset
  writeFile(SD, FILENAME_FILTERED, "Filtered data collected from M5Stack - Group ST4 4401. Seperated by comma: [Z,Y,X]\n");//Header for data file

  timerAlarmEnable(timer); //Enable interrupt timer
}




void loop() {
  if(flag_ADC && deviceConnected){
    interruptCounter++;
    M5_IMU_read_ADC(); //Kaldes for at få data fra IMU - Takes approx 560µs
    storeADCData();
    portENTER_CRITICAL(&timerMux);
    flag_ADC=false;
    portEXIT_CRITICAL(&timerMux);
  }

  if(sample_count >= arraySize){
    #if SDACTIVE
      WriteToSDcard_raw(sample_count);
      Bpfilter(sample_count);
      Envfilter(sample_count);//Envelope filter and check for thresholds to identify
      WriteToSDcard_filtered(sample_count);
    #endif
    sample_count=0;
  }

  /*if(startIdentified && ((interruptCounter % 50) == 0)){
    value1 += 1;
    pCharacteristic_start->setValue(value1); //
    pCharacteristic_start->notify();
    printf("Notified value: %d\n", value1);

    //delay(500);
    startIdentified = false;
    turnIdentified = true;
  }
  if(turnIdentified && ((interruptCounter % 50) == 0)){
    value2 += 2;
    pCharacteristic_turn->setValue(value2); //
    pCharacteristic_turn->notify();
    printf("Notified value: %d\n", value2);

    //delay(500);
    turnIdentified = false;
    stopIdentified = true;
  }
  if(stopIdentified && ((interruptCounter % 50) == 0)){
    value3 += 3;
    pCharacteristic_stop->setValue(value3); //
    pCharacteristic_stop->notify();
    printf("Notified value: %d\n", value3);

    //delay(500);
    stopIdentified = false;
    startIdentified = true;
  }*/



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

//Warning: Array size must be dividable with 6.
void IRAM_ATTR storeADCData(){
  #if IMU_UNIT
    valueArray[sample_count++] = (int)1000*IMU.getAccelZ_mss();//in cm/s^2
    //valueArray[sample_count++] = (int)1000*IMU.getAccelY_mss();//increment sample_count and store read data in array.
    //valueArray[sample_count++] = (int)1000*IMU.getAccelX_mss();

    //valueArray[sample_count++] = (int)1000*IMU.getGyroZ_rads(); //in rad/s
    //valueArray[sample_count++] = (int)1000*IMU.getGyroY_rads();//increment sample_count and store read data in array.
    //valueArray[sample_count++] = (int)1000*IMU.getGyroX_rads();

  #endif
}

void Bpfilter(int samplesToFilter){
  for (int i = 0; i < samplesToFilter; i++) {
    valueArray[i] = BpIIR_filter(valueArray[i]);
  }

}
void Envfilter(int samplesToFilter){
  for (int i = 0; i < samplesToFilter; i++) {
    //Absolute value
    if (valueArray[i] < 0) {
      valueArray[i] *= -1;
    }
    valueArray[i] = EnvIIR_filter(valueArray[i]);
    //Check if filtered value can be identified as start or stop.
    if (valueArray[i] > threshold_StartStop && stopIdentified) {
      startIdentified = true;
      stopIdentified = false;
      pCharacteristic_start->setValue(value1);//Aflæs tid
      pCharacteristic_start->notify();//Notify "start fundet".
    }
    if (valueArray[i] < threshold_StartStop && startIdentified) {
      startIdentified = false;
      stopIdentified = true;
      pCharacteristic_stop->setValue(value3);//Aflæs tid
      pCharacteristic_stop->notify();//Notify "stop fundet".
    }
  }

}
int floor_and_convert(double y) {
  if(y > 0)
  {
    y += 0.5;
  }
  else
  {
    y -= 0.5;
  }
  return (int) y;
}
int BpIIR_filter(int value)
{
  x[0] =  (double) (value);           // Read received sample and perform typecast

  y[0] = b[0] * x[0];                 //Run IIR for first element
  for (int i = 1; i <= FILTER_LENGTH; i++) // Run IIR filter for all elements
  {
    y[0] += b[i] * x[i] - a[i] * y[i];
  }

  for (int i = FILTER_LENGTH - 1; i >= 0; i--) // Roll x array in order to hold old sample inputs
  {
    x[i + 1] = x[i];
    y[i + 1] = y[i];
  }


  return floor_and_convert(y[0]);
}
int EnvIIR_filter(int value){
  xe[0] =  (double) (value); // Read received sample and perform tyepecast

  ye[0] = be[0]*xe[0];                   // Run IIR filter for first element

  for(int i = 1;i <= FILTER_LENGTH2;i++)   // Run IIR filter for all other elements
  {
      ye[0] += be[i]*xe[i] - ae[i]*ye[i];
  }

  for(int i = FILTER_LENGTH2-1;i >= 0;i--) // Roll xe and ye arrayes in order to hold old sample inputs and outputs
  {
      xe[i+1] = xe[i];
      ye[i+1] = ye[i];
  }

  return floor_and_convert(ye[0]);     // fixe rounding issues;

}

//From https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/ and Arduino example
void WriteToSDcard_raw(int samplesToWrite) {
  String dataMessage;
  File file = SD.open(FILENAME_RAW);

  if(!file) {//Check if file has been initialized
    Serial.println("File doens't exist");
    return;
  }
  else {//Append data to file
    dataMessage = int2str(valueArray, samplesToWrite);//Converts integer array to string
    Serial.print("Save data: ");
    Serial.println(dataMessage);
    appendFile(SD, FILENAME_RAW, (dataMessage.c_str()) );//Converts string to char* to append
  }
  file.close();
}
//From https://randomnerdtutorials.com/esp32-data-logging-temperature-to-microsd-card/ and Arduino example
void WriteToSDcard_filtered(int samplesToWrite) {
  String dataMessage;
  File file = SD.open(FILENAME_FILTERED);

  if(!file) {//Check if file has been initialized
    Serial.println("File doens't exist");
    return;
  }
  else {//Append data to file
    dataMessage = int2str(valueArray, samplesToWrite);//Converts integer array to string
    Serial.print("Save data: ");
    Serial.println(dataMessage);
    appendFile(SD, FILENAME_FILTERED, (dataMessage.c_str()) );//Converts string to char* to append
  }
  file.close();
}

// Write to the SD card
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.print("File Initialized with: ");
    Serial.println(message);
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

String int2str(int inArray[], int size){
  String returnString = "";

  for (int i=0; i < size; i++){
      returnString += String(inArray[i]) + ",";
  }
  return returnString;
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

  // Create a BLE Characteristic for start
  pCharacteristic_start = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_START,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

    // Create a BLE Characteristic for turn
  pCharacteristic_turn = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TURN,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

    // Create a BLE Characteristic for turn
  pCharacteristic_stop = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_STOP,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
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
