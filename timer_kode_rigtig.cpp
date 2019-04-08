#include <Arduino.h>
#include <timer_table.h>
#include <M5Stack.h>

#define NOTE_DH2 330

volatile int interruptCounter;
float totalInterruptCounter=-1;
int current_time_interval;

hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
volatile uint32_t lastIsrAt = 0;

int index_counter=0;
float beep_next=IR1_table[index_counter];

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux); 
}

void setup() {
 
  Serial.begin(115200);
  M5.begin();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  current_time_interval = (1000000/10); // 1000000 er 1 gang pr sek. 
  timerAlarmWrite(timer, current_time_interval, true);
  timerAlarmEnable(timer);
}
 
void loop() {

  if (interruptCounter > 0) {
    
    portENTER_CRITICAL(&timerMux); // der er ikke nogen der kan afbryde os imellem denne og port_EXIT_CRITICAl -> kan ikke interruptes her 
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
 
    totalInterruptCounter++;
  
    Serial.print(" Total number:");
    Serial.println(totalInterruptCounter/10);       
    
    if ((totalInterruptCounter/10)>=beep_next){  
      M5.Speaker.tone(NOTE_DH2, 200);      
      Serial.println("!!!!!!!!!!!bip!!!!!!!!!!!");
      index_counter++;
      beep_next=IR1_table[index_counter];
      return; 
    }
    M5.update();
  }
}