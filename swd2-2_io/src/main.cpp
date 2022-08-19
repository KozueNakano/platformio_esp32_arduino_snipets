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

#define PULSE_INPUT_PIN A1    //パルスの入力ピン
#define PCNT_H_LIM_VAL 32767  //カウンタの上限 今回は使ってない
#define PCNT_L_LIM_VAL -32768 //カウンタの下限 今回は使ってない

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

  pcnt_config_t pcnt_config; //設定用の構造体の宣言
  pcnt_config.pulse_gpio_num = PULSE_INPUT_PIN;
  pcnt_config.ctrl_gpio_num = PCNT_PIN_NOT_USED;
  pcnt_config.lctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.hctrl_mode = PCNT_MODE_KEEP;
  pcnt_config.channel = PCNT_CHANNEL_0;
  pcnt_config.unit = PCNT_UNIT_0;
  pcnt_config.pos_mode = PCNT_COUNT_DIS;
  pcnt_config.neg_mode = PCNT_COUNT_INC;
  pcnt_config.counter_h_lim = PCNT_H_LIM_VAL;
  pcnt_config.counter_l_lim = PCNT_L_LIM_VAL;
  pcnt_unit_config(&pcnt_config);   //ユニット初期化
  pcnt_counter_pause(PCNT_UNIT_0);  //カウンタ一時停止
  pcnt_counter_clear(PCNT_UNIT_0);  //カウンタ初期化
  pcnt_counter_resume(PCNT_UNIT_0); //カウント開始
}

void loop()
{

  wakeupCause wokeUpTo = getWakeupCause();
  // start pulse counter
  pcnt_counter_clear(PCNT_UNIT_0);

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
      pcnt_counter_clear(PCNT_UNIT_0);
      preSigMillis = millis();
      Serial.print("signal pressed");
    }
    if ((millis() - preSigMillis) > sigDeadMillis)
    {
      int16_t count = 0; //カウント数
      pcnt_get_counter_value(PCNT_UNIT_0, &count);
      if (count != 0)
      {
        pcnt_counter_clear(PCNT_UNIT_0);
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
