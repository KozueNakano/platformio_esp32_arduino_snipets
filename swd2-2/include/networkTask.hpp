// networkTask.hpp
#ifndef NETWORK_TASK
#define NETWORK_TASK

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
#include "time.h"
#include "sntp.h"
#include "stateNum.hpp"



struct networkTaskArgs
{
  EventGroupHandle_t sleepEventHandle;
  EventBits_t net_ok_bit;
};

void task_network(void *pvParameters);
bool getTimeAvailable(void);
void setSendNow(void);
uint64_t getMacaddress_int(void);
void set_keepIndexCb(void(*func)(void));
void set_deleteBeforeKeepCb(void(*func)(void));
void set_printArrayCb(void(*func)());
void set_getJsonStringCb(bool(*func)(String**));
void set_lcdSetNetStatusCb(void(*func)(int));

#endif