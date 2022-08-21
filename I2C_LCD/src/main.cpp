#include <Arduino.h>
#include "lcd_ctrl.hpp"

void setup()
{
  esp_sleep_enable_timer_wakeup(1000000);
  lcdInit();
}

void loop()
{
  setState(NP);
  setNetStatus(0);
  delay(1000);

  setState(DO);
  setNetStatus(1);
  esp_light_sleep_start();
  delay(1000);

  setState(REST);
  setNetStatus(2);
  delay(1000);
  lcdShutdown();
  delay(1000);
  esp_deep_sleep_start();
  
}
