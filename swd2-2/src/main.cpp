#include <Arduino.h>
#include "ioSignalProcess.hpp"
#include "driver/pcnt.h"
#include "networkTask.hpp"
#include "stateTimeData.hpp"
#include "lcd_ctrl.hpp"

String deviceName = "SWD36";

EventGroupHandle_t sleepable_event_group;
#define NET_OK_BIT BIT0

struct networkTaskArgs netTaskArg;
TaskHandle_t task_network_handle = NULL;

bool sigAtSleepFlag = false;
unsigned long preSigMillis = 0;
const unsigned long sigDeadMillis = 1000;

stateTimeData stateTime;

void printDataArray_cb(void)
{
  // Serial.println("-------printDataArray_cb--------");
  stateTime.serialPrint();
}
void keepIndex_cb(void)
{
  // Serial.println("-------keepIndex_cb--------");
  stateTime.keepIndex();
}
void deleteDataBeforeKeep_cb(void)
{
  // Serial.println("-------deleteDataBeforeKeep_cb--------");
  stateTime.deleteDataBeforeKeep();
}

String jsonStringBuffer;
void reserveJsonString(void)
{
  jsonStringBuffer.reserve(stateTime.requiredStringLength());
}

bool getJsonString_cb(String **stringBuffer)
{
  // Serial.println("------getJsonString_cb-------");
  bool available = stateTime.getJsonString(&jsonStringBuffer, &deviceName, getMacaddress_int());
  *stringBuffer = &jsonStringBuffer;
  return available;
}

int tempCounter = 0;
void sigDetected(void)
{
  counterClear();
  preSigMillis = millis();

  struct timeval tv;
  int timeget = gettimeofday(&tv, NULL);
  if (getTimeAvailable() && (timeget == 0))
  {
    updateModeToggleState();
    stateTv tempStateTv;
    tempStateTv.state = getState();
    tempStateTv.tv = tv;
    /*
    tempStateTv.state = NP;
    tempStateTv.tv.tv_sec = tempCounter;
    tempStateTv.tv.tv_usec = tempCounter * 1000;
    */

    stateTime.addData(tempStateTv);
    tempCounter++;
    // stateTime.serialPrint();
    // Serial.println("signal store");
    // Serial.print("tv.sec");
    // Serial.print(tv.tv_sec);
    // Serial.print(" : tv.msec");
    // Serial.println(tv.tv_usec / 1000);
    // Serial.print("macaddress:");
    // Serial.println(getMacaddress_int(), HEX);
    // Serial.flush();
  }
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  lcdInit();
  reserveJsonString();
  setMqttBufferSize(stateTime.requiredStringLength()+20);
  // イベントグループの初期化
  sleepable_event_group = xEventGroupCreate();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);

  netTaskArg.net_ok_bit = NET_OK_BIT;
  netTaskArg.sleepEventHandle = sleepable_event_group;
  set_printArrayCb(printDataArray_cb);
  set_keepIndexCb(keepIndex_cb);
  set_deleteBeforeKeepCb(deleteDataBeforeKeep_cb);
  set_getJsonStringCb(getJsonString_cb);
  set_lcdSetNetStatusCb(lcdSetNetStatus);

  swIoSetting();
  counterInit();
}

void loop()
{

  wakeupCause wokeUpTo = getWakeupCause();
  // start pulse counter
  counterClear();
  // Serial.println();
  //Serial.println("wakeup! cause:-----------------------------------------------");
  int battMilliVolts = analogReadMilliVolts(A13) * 2;
  if (battMilliVolts < 3500)
  {
    lcdShutdown();
    // Serial.println("LOW BATT");
    // Serial.flush();
    esp_deep_sleep_start();
  }

  switch (wokeUpTo)
  {
  case SIG:
    // Serial.println("sig");
    sigAtSleepFlag = true;
    break;
  case TIMER:
    // Serial.println("timer");
    break;
  case SEND:
    // Serial.println("send");
    setSendNow();
    break;
  case MODE:
    // Serial.println("mode");
    break;
  default:
    // Serial.println();
    break;
  }

  // Serial.println("main while start");
  // Serial.flush();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
    xTaskCreateUniversal(
      task_network,               // 作成するタスク関数
      "task_network",             // 表示用タスク名
      8192,                       // スタックメモリ量
      &netTaskArg,                // 起動パラメータ
      1,                          // 優先度
      &task_network_handle,       // タスクハンドル
      CONFIG_ARDUINO_RUNNING_CORE // 実行するコア
  );
  while (true)
  {
    //Serial.println("_");
    if ((sigAtSleepFlag == true))
    {
      sigAtSleepFlag = false;
      sigDetected();
    }

    if (getCount() != 0)
    {
      counterClear();
      if ((millis() - preSigMillis) > sigDeadMillis)
      {
        sigDetected();
      }
    }

    updateModeState();
    lcdSetState(getState());

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
  // Serial.println();
  // Serial.println("------------------------------------------main while end");
  // Serial.flush();

  unsigned long swQuitMillis = 0;
  unsigned long millisFromPreSig = millis() - preSigMillis;
  if (millisFromPreSig < sigDeadMillis)
  {
    swQuitMillis = (sigDeadMillis - millisFromPreSig);
  }
  enterLightSleep(3000000, (swQuitMillis * 1000));
}
