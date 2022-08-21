#include <Arduino.h>

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  // put your main code here, to run repeatedly:
  int  battMilliVolts = analogReadMilliVolts(A13) *2;
  
  // print out the values you read:
  Serial.printf("ADC millivolts value = %d\n",battMilliVolts);
  
  delay(100);  // delay in between reads for clear read from serial
}