#include <Arduino.h>
#include "ioSignalProcess.hpp"
#include "driver/pcnt.h"
#include "networkTask.hpp"

String deviceName = "SWD36";

EventGroupHandle_t sleepable_event_group;
#define NET_OK_BIT BIT0

struct networkTaskArgs netTaskArg;
TaskHandle_t task_network_handle = NULL;

bool sigAtSleepFlag = false;
unsigned long preSigMillis = 0;
const unsigned long sigDeadMillis = 1000;

struct stateTv
{
  stateNum state;
  timeval tv;
};
class stateTimeData
{
private:
  SemaphoreHandle_t mutex;
  stateTv stateTimeArray[1023];
  int arrayQty;
  int index;
  int indexStore;
  const String deviceNameHead = "{\"deviceName\":\"";
  const String deviceNameFoot = "\"";
  const String timeHead = ",\"detectedTime\":[";
  const String timeFoot = "]";
  const String timeMsHead = ",\"detectedTime_ms\":[";
  const String timeMsFoot = "]";
  const String stateHead = ",\"state\":[";
  const String stateFoot = "]";
  const String macHead = ",\"macAddress\":\"";
  const String macFoot = "\"}";

public:
  stateTimeData(/* args */);
  int getArrayQty(void);
  void addData(stateTv);
  void serialPrint(void);
  int getIndex(void);
  void deleteData(int deleteBeforeIndex);
  void keepIndex(void);
  void deleteDataBeforeKeep(void);
  int requiredStringLength(void);
  bool getJsonString(String *buffString, String *deviceName, uint64_t macaddress);

  ~stateTimeData();
} stateTime;

stateTimeData::stateTimeData(/* args */)
{
  arrayQty = sizeof(stateTimeArray) / sizeof(stateTimeArray[0]);
  index = 0;
  mutex = xSemaphoreCreateMutex();
}
int stateTimeData::getArrayQty(void)
{
  return arrayQty;
}
void stateTimeData::addData(stateTv argStr)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  if (index < arrayQty)
  {
    stateTimeArray[index] = argStr;
    index++;
  }
  xSemaphoreGive(mutex);
}

void stateTimeData::serialPrint(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (int i = 0; i < arrayQty; i++)
  {
    if (stateTimeArray[i].tv.tv_sec != 0)
    {
      Serial.print("sec:");
      Serial.print(stateTimeArray[i].tv.tv_sec);
      Serial.print("state");
      Serial.println(stateTimeArray[i].state);
    }
  }
  xSemaphoreGive(mutex);
}

int stateTimeData::getIndex(void)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  return index;
  xSemaphoreGive(mutex);
}

void stateTimeData::deleteData(int deleteBeforeIndex)
{
  xSemaphoreTake(mutex, portMAX_DELAY);
  for (int i = 0; i < arrayQty; i++)
  {
    int readIndex = deleteBeforeIndex + i;
    if (readIndex < arrayQty)
    {
      stateTimeArray[i] = stateTimeArray[deleteBeforeIndex + i];
    }
    else
    {
      stateTimeArray[i].state = NP;
      stateTimeArray[i].tv.tv_sec = 0;
      stateTimeArray[i].tv.tv_usec = 0;
    }
  }
  index = index - deleteBeforeIndex;
  xSemaphoreGive(mutex);
}
void stateTimeData::keepIndex(void)
{
  indexStore = index;
}

void stateTimeData::deleteDataBeforeKeep(void)
{
  deleteData(indexStore);
}

int stateTimeData::requiredStringLength(void)
{
  int reserveLength = deviceNameHead.length() + /*SWDxxxx*/ 7 + deviceNameFoot.length() + timeHead.length() + 11 * arrayQty /*,4294967295*/ + timeFoot.length() + timeMsHead.length() + 11 * arrayQty /*,4294967295*/ + timeMsFoot.length() + stateHead.length() + 2 * arrayQty /*0,1,2*/ + stateFoot.length() + macHead.length() + 12 /*0xFFFFFFFFFFFF*/ + macFoot.length();
  return reserveLength;
}
bool stateTimeData::getJsonString(String *buffString, String *deviceName, uint64_t macaddress_arg)
{
  if (indexStore != 0)
  {
    xSemaphoreTake(mutex, portMAX_DELAY);
    buffString->clear();
    buffString->concat(deviceNameHead);
    buffString->concat(*deviceName);
    buffString->concat(deviceNameFoot);
    buffString->concat(timeHead);
    for (int i = 0; i < indexStore; i++)
    {
      if (i != 0)
        buffString->concat(",");
      buffString->concat(String((stateTimeArray[i].tv.tv_sec), DEC));
    }
    buffString->concat(timeFoot);
    buffString->concat(timeMsHead);
    for (int i = 0; i < indexStore; i++)
    {
      if (i != 0)
        buffString->concat(",");
      buffString->concat(String((stateTimeArray[i].tv.tv_usec / 1000), DEC));
    }
    buffString->concat(timeMsFoot);
    buffString->concat(stateHead);
    for (int i = 0; i < indexStore; i++)
    {
      if (i != 0)
        buffString->concat(",");
      buffString->concat(String((stateTimeArray[i].state), DEC));
    }
    buffString->concat(stateFoot);
    buffString->concat(macHead);
    char macChar[13] = "";
    sprintf(macChar, "%012llx", macaddress_arg);
    buffString->concat(String(macChar));
    buffString->concat(macFoot);
    xSemaphoreGive(mutex);
    return true;
  }else{
    return false;
  }
}
stateTimeData::~stateTimeData()
{
}

void printDataArray_cb(void)
{
  Serial.println("-------printDataArray_cb--------");
  stateTime.serialPrint();
}
void keepIndex_cb(void)
{
  Serial.println("-------keepIndex_cb--------");
  stateTime.keepIndex();
}
void deleteDataBeforeKeep_cb(void)
{
  Serial.println("-------deleteDataBeforeKeep_cb--------");
  stateTime.deleteDataBeforeKeep();
}

String jsonStringBuffer;
void reserveJsonString(void)
{
  jsonStringBuffer.reserve(stateTime.requiredStringLength());
}

bool getJsonString_cb(String** stringBuffer)
{
  Serial.println("------getJsonString_cb-------");
  bool available = stateTime.getJsonString(&jsonStringBuffer, &deviceName, getMacaddress_int());
  *stringBuffer = &jsonStringBuffer;
  return available;
}

void setup()
{
  Serial.begin(115200);
  reserveJsonString();
  // イベントグループの初期化
  sleepable_event_group = xEventGroupCreate();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);

  netTaskArg.net_ok_bit = NET_OK_BIT;
  netTaskArg.sleepEventHandle = sleepable_event_group;
  set_printArrayCb(printDataArray_cb);
  set_keepIndexCb(keepIndex_cb);
  set_deleteBeforeKeepCb(deleteDataBeforeKeep_cb);
  set_getJsonStringCb(getJsonString_cb);

  xTaskCreateUniversal(
      task_network,               // 作成するタスク関数
      "task_network",             // 表示用タスク名
      8192,                       // スタックメモリ量
      &netTaskArg,                // 起動パラメータ
      1,                          // 優先度
      &task_network_handle,       // タスクハンドル
      CONFIG_ARDUINO_RUNNING_CORE // 実行するコア
  );

  swIoSetting();
  counterInit();
}

void sigDetected(void)
{
  counterClear();
  preSigMillis = millis();
  Serial.println("signal pressed");

  struct timeval tv;
  int timeget = gettimeofday(&tv, NULL);
  if (getTimeAvailable() && (timeget == 0))
  {
    updateModeToggleState();
    stateTv tempStateTv;
    tempStateTv.state = getState();
    tempStateTv.tv = tv;
    stateTime.addData(tempStateTv);
    // stateTime.serialPrint();
    Serial.print("tv.sec");
    Serial.print(tv.tv_sec);
    Serial.print(" : tv.msec");
    Serial.println(tv.tv_usec / 1000);
    Serial.print("macaddress:");
    Serial.println(getMacaddress_int(), HEX);
  }
}

void loop()
{

  wakeupCause wokeUpTo = getWakeupCause();
  // start pulse counter
  counterClear();
  vTaskResume(task_network_handle);

  switch (wokeUpTo)
  {
  case SIG:
    Serial.println("sig");
    sigAtSleepFlag = true;
    break;
  case TIMER:
    Serial.println("timer");
    break;
  case SEND:
    Serial.println("send");
    setSendNow();
    break;
  case MODE:
    Serial.println("mode");
    break;
  default:
    break;
  }

  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
  while (true)
  {
    if ((sigAtSleepFlag == true))
    {
      sigAtSleepFlag = false;
      sigDetected();
    }
    if ((millis() - preSigMillis) > sigDeadMillis)
    {
      if (getCount() != 0)
      {
        sigDetected();
      }
    }

    updateModeState();
    Serial.println(".");

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
  vTaskSuspend(task_network_handle);

  unsigned long swQuitMillis = 0;
  unsigned long millisFromPreSig = millis() - preSigMillis;
  if (millisFromPreSig < sigDeadMillis)
  {
    swQuitMillis = (sigDeadMillis - millisFromPreSig);
  }
  enterLightSleep(3000000, (swQuitMillis * 1000));
}
