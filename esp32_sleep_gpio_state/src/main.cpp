#include <Arduino.h>

void setup()
{
  // put your setup code here, to run once:
  esp_sleep_enable_timer_wakeup(5000000);
  Serial.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  pinMode(GPIO_NUM_17,OUTPUT);
  digitalWrite(GPIO_NUM_17, LOW);
  gpio_hold_en(GPIO_NUM_17);
  gpio_deep_sleep_hold_en();
  Serial.println("lightsleep");
  Serial.flush();
  esp_light_sleep_start();

  gpio_hold_dis(GPIO_NUM_17);
  gpio_deep_sleep_hold_dis();
  pinMode (GPIO_NUM_17, INPUT) ;
  gpio_set_pull_mode( GPIO_NUM_17, GPIO_FLOATING) ;
  Serial.println("DeepSleep");
  Serial.flush();
  esp_deep_sleep_start();
}