#include <Arduino.h>
#include "ioSignalProcess.hpp"
#include "driver/pcnt.h"

EventGroupHandle_t sleepable_event_group;
#define NET_OK_BIT BIT0

TaskHandle_t test_handle = NULL;

bool sendFlag = false;
bool sigAtSleepFlag = false;
unsigned long preSendMillis = 0;
const unsigned long sendIntervalMillis = 3000;
unsigned long preSigMillis = 0;
const unsigned long sigDeadMillis = 1000;


void task_test(void *pvParameters)
{
  while (true)
  {
    if (sendFlag == true)
    {
      Serial.print("sending");
      for (int i = 0; i < 10; i++)
      {
        delay(100);
        Serial.print(".");
      }
      Serial.println("finish");
    }
    sendFlag = false;
    xEventGroupSetBits(sleepable_event_group, NET_OK_BIT);
  }
}



void setup()
{
  // イベントグループの初期化
  sleepable_event_group = xEventGroupCreate();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
  xTaskCreateUniversal(
      task_test,                  // 作成するタスク関数
      "task_network",             // 表示用タスク名
      8192,                       // スタックメモリ量
      NULL,                       // 起動パラメータ
      1,                          // 優先度
      &test_handle,               // タスクハンドル
      CONFIG_ARDUINO_RUNNING_CORE // 実行するコア
  );

  Serial.begin(115200);
  swIoSetting();
  counterInit();
}

void loop()
{

  wakeupCause wokeUpTo = getWakeupCause();
  // start pulse counter
  counterClear();

  switch (wokeUpTo)
  {
  case SIG:
    Serial.println("sig");
    sigAtSleepFlag = true;
    break;
  case SEND:
    Serial.println("send");
    sendFlag = true;
    break;
  case MODE:
    Serial.println("mode");
    break;
  default:
    break;
  }

  if ((millis() - preSendMillis) > sendIntervalMillis)
  {
    preSendMillis = millis();
    sendFlag = true;
  }

  Serial.printf("mode:%d\n", updateModeSwitch());
  Serial.flush();

  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
  while (true)
  {
    if ((sigAtSleepFlag == true))
    {
      sigAtSleepFlag = false;
      counterClear();
      preSigMillis = millis();
      Serial.print("signal pressed");
    }
    if ((millis() - preSigMillis) > sigDeadMillis)
    {
      if (getCount() != 0)
      {
        counterClear();
        preSigMillis = millis();
        Serial.print("signal pressed");
      }
    }

    delay(100);
    uint32_t eBits = xEventGroupWaitBits(
        sleepable_event_group, // イベントグループを指定
        NET_OK_BIT,            // 一つ以上のイベントビットを指定
        pdTRUE,                // 呼び出し後にイベントビットをクリアするか
        pdTRUE,                // 指定したイベントビットがすべて揃うまで待つか
        0                      // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
    );
    uint32_t sleepable_eventGroup_bitmask = (NET_OK_BIT /*|*/);
    if ((eBits & sleepable_eventGroup_bitmask) == sleepable_eventGroup_bitmask)
    {
      break;
    }
  }

  unsigned long swQuitMillis = 0;
  unsigned long millisFromPreSig = millis() - preSigMillis;
  if (millisFromPreSig < sigDeadMillis)
  {
    swQuitMillis = (sigDeadMillis - millisFromPreSig);
  }

  enterLightSleep(3000000, (swQuitMillis * 1000));
}
