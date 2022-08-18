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

// wifi credential
#define WIFI_SSID "mqtt_server_ssid"
#define WIFI_PASSWORD "mosquitto"
// mqtt credential
const char hostName[] = "raspberrypi.local";
#define MQTT_PORT 1883
const char username[] = "sub";
const char password[] = "mosquitto";

EventGroupHandle_t networkTaskFinished_event_group;
#define MQTT_OK_BIT BIT0
TaskHandle_t task_network_handle = NULL;

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

bool newtwork_keepConn = false;

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch (event)
  {
  case SYSTEM_EVENT_STA_GOT_IP:
    xTimerStop(wifiReconnectTimer, 0);
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    connectToMqtt();
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    if (newtwork_keepConn == true)
      xTimerStart(wifiReconnectTimer, 0);
    break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  xTimerStop(mqttReconnectTimer, 0);
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdPub0 = mqttClient.publish("test/lol", 0, false, "test 1");
  Serial.println("Publishing at QoS 0");
  uint16_t packetIdPub1 = mqttClient.publish("test/lol", 1, false, "test 2");
  Serial.print("Publishing at QoS 1, packetId: ");
  Serial.println(packetIdPub1);
  uint16_t packetIdPub2 = mqttClient.publish("test/lol", 2, false, "test 3");
  Serial.print("Publishing at QoS 2, packetId: ");
  Serial.println(packetIdPub2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected() && (newtwork_keepConn == true))
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttPublish(uint16_t packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  xEventGroupSetBits(networkTaskFinished_event_group, MQTT_OK_BIT);
}

struct networkTaskArgs
{
  EventGroupHandle_t sleepEventHandle;
  EventBits_t net_ok_bit;
};

void task_network(void *pvParameters)
{
  struct networkTaskArgs *argStruct = (networkTaskArgs *)pvParameters;

  networkTaskFinished_event_group = xEventGroupCreate();
  xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(hostName, MQTT_PORT);
  mqttClient.setCredentials(username, password);

  unsigned long preSendMqttMillis = millis();
  unsigned long sendMqttIntervalMillis = 10000;
  while (true)
  {
    if ((millis() - preSendMqttMillis) > sendMqttIntervalMillis)
    {
      preSendMqttMillis = millis();
      xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);
      newtwork_keepConn = true;
      connectToWifi();
      uint32_t eBits = xEventGroupWaitBits(
          networkTaskFinished_event_group, // イベントグループを指定
          MQTT_OK_BIT,                     // 一つ以上のイベントビットを指定
          pdTRUE,                          // 呼び出し後にイベントビットをクリアするか
          pdTRUE,                          // 指定したイベントビットがすべて揃うまで待つか
          pdMS_TO_TICKS(5000)              // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
      );
      newtwork_keepConn = false;
      xTimerStop(wifiReconnectTimer, 0);
      xTimerStop(mqttReconnectTimer, 0);
      mqttClient.disconnect(false);
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      Serial.println("send finish");
    }
    // xEventGroupSetBits(sleepable_event_group, NET_OK_BIT);
    xEventGroupSetBits(argStruct->sleepEventHandle, argStruct->net_ok_bit);
  }
  vTaskDelete(NULL);
}

/*main------------------------------------------*/
static EventGroupHandle_t sleepable_event_group;
#define NET_OK_BIT BIT0

struct networkTaskArgs netTaskArg;
/*------------------------------------------main*/

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
  xEventGroupClearBits(sleepable_event_group, 0xFFFFFF);
  uint32_t eBits = xEventGroupWaitBits(
      sleepable_event_group, // イベントグループを指定
      NET_OK_BIT,            // 一つ以上のイベントビットを指定
      pdTRUE,                // 呼び出し後にイベントビットをクリアするか
      pdTRUE,                // 指定したイベントビットがすべて揃うまで待つか
      portMAX_DELAY          // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
  );
  esp_light_sleep_start();
}