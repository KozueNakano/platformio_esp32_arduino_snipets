#include <Arduino.h>
#include <networkTask.hpp>

static EventGroupHandle_t sleepable_event_group;
#define NET_OK_BIT BIT0

struct networkTaskArgs netTaskArg;
TaskHandle_t task_network_handle = NULL;

void setup()
{
  // イベントグループの初期化
  sleepable_event_group = xEventGroupCreate();
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);

  netTaskArg.net_ok_bit = NET_OK_BIT;
  netTaskArg.sleepEventHandle = sleepable_event_group;

  Serial.begin(115200);
  Serial.println();

  xTaskCreateUniversal(
      task_network,               // 作成するタスク関数
      "task_network",             // 表示用タスク名
      8192,                       // スタックメモリ量
      &netTaskArg,                // 起動パラメータ
      1,                          // 優先度
      &task_network_handle,       // タスクハンドル
      CONFIG_ARDUINO_RUNNING_CORE // 実行するコア
  );
  esp_sleep_enable_timer_wakeup(1000000);
}

void loop()
{
  Serial.println("wake");
  struct timeval tv;
  int timeget = gettimeofday(&tv, NULL);
  if (getTimeAvailable() && (timeget == 0))
  {
    Serial.print("tv.sec");
    Serial.print(tv.tv_sec);
    Serial.print(" : tv.msec");
    Serial.println(tv.tv_usec/1000);
    Serial.print("timeAvailableFlag");
    Serial.println(getTimeAvailable());
  }
  
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
  uint32_t eBits = xEventGroupWaitBits(
      sleepable_event_group, // イベントグループを指定
      NET_OK_BIT,            // 一つ以上のイベントビットを指定
      pdTRUE,                // 呼び出し後にイベントビットをクリアするか
      pdTRUE,                // 指定したイベントビットがすべて揃うまで待つか
      portMAX_DELAY          // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
  );
  Serial.flush();
  esp_light_sleep_start();
}