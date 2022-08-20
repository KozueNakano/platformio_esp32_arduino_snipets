#include <Arduino.h>
#include "ioSignalProcess.hpp"
#include "driver/pcnt.h"
#include "networkTask.hpp"

char deviceName[] = "SWD35";

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
  stateTv stateTimeArray[5];
  int arrayQty;
  int index;

public:
  stateTimeData(/* args */);
  void addData(stateTv);
  void serialPrint(void);
  int getIndex(void);
  void deleteData(int deleteBeforeIndex);
  ~stateTimeData();
} stateTime;

stateTimeData::stateTimeData(/* args */)
{
  arrayQty = sizeof(stateTimeArray) / sizeof(stateTimeArray[0]);
  index = 0;
  mutex = xSemaphoreCreateMutex();
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
    }else{
      stateTimeArray[i].state = NP;
      stateTimeArray[i].tv.tv_sec = 0;
      stateTimeArray[i].tv.tv_usec = 0;
    }
  }
  xSemaphoreGive(mutex);
}

stateTimeData::~stateTimeData()
{
}

void setup()
{
  // イベントグループの初期化
  sleepable_event_group = xEventGroupCreate();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);

  netTaskArg.net_ok_bit = NET_OK_BIT;
  netTaskArg.sleepEventHandle = sleepable_event_group;

  xTaskCreateUniversal(
      task_network,               // 作成するタスク関数
      "task_network",             // 表示用タスク名
      8192,                       // スタックメモリ量
      &netTaskArg,                // 起動パラメータ
      1,                          // 優先度
      &task_network_handle,       // タスクハンドル
      CONFIG_ARDUINO_RUNNING_CORE // 実行するコア
  );

  Serial.begin(115200);
  swIoSetting();
  counterInit();
}

uint64_t octetToIntMac(uint8_t *octetArray)
{
  uint64_t intVal = 0;
  for (int i = 0; i < 6; i++)
  {
    intVal = (intVal << 8) | octetArray[i];
  }
  return intVal;
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
    stateTime.serialPrint();
    Serial.print("tv.sec");
    Serial.print(tv.tv_sec);
    Serial.print(" : tv.msec");
    Serial.println(tv.tv_usec / 1000);
    uint8_t octet[6];
    getMacaddress(octet);
    Serial.print("macaddress:");
    Serial.println(getMacaddressString());
    Serial.print("macaddress:");
    Serial.println(octetToIntMac(octet), HEX);
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
    Serial.print("state:");
    Serial.println(getState());

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
