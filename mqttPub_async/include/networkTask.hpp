#include <Arduino.h>
/*
This example uses FreeRTOS softwaretimers as there is no built-in Ticker library
*/
#include <WiFi.h>
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>



struct networkTaskArgs
{
  EventGroupHandle_t sleepEventHandle;
  EventBits_t net_ok_bit;
};

void task_network(void *pvParameters);