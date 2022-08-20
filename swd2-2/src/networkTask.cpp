#include <networkTask.hpp>

// wifi credential
#define WIFI_SSID "mqtt_server_ssid"
#define WIFI_PASSWORD "mosquitto"
// mqtt credential
#define HOSTNAME "raspberrypi.local"
#define MQTT_PORT 1883
//#define USERNAME "sub"
//#define PASSWORD "mosquitto"

const char *ntpServer1 = "raspberrypi.local";
const char *time_zone = "JST-9"; // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
static const unsigned long mqttReconnectMillis = 2000;
static const unsigned long wifiReconnectMillis = 2000;
static const unsigned long sendMqttIntervalMillis = 10000;
static const unsigned long netTaskTimeout = 5000;

EventGroupHandle_t networkTaskFinished_event_group;
#define MQTT_OK_BIT BIT0
#define NTP_OK_BIT BIT1

static bool newtwork_keepConn = false;
static bool timeAvailable = false;

static bool sendNowFlag = true;

void (*keepIndexCb)(void);
void (*deleteBeforeKeepCb)(void);
void (*printArrayCb)(void);
bool (*getJsonStringCb)(String **);

void set_keepIndexCb(void (*func)(void))
{
    keepIndexCb = func;
}

void set_deleteBeforeKeepCb(void (*func)(void))
{
    deleteBeforeKeepCb = func;
}

void set_printArrayCb(void (*func)())
{
    printArrayCb = func;
}
void set_getJsonStringCb(bool (*func)(String **))
{
    getJsonStringCb = func;
}

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
        configTzTime(time_zone, ntpServer1);
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
    keepIndexCb();
    // Serial.println(*getJsonStringCb());
    // uint16_t packetIdPub0 = mqttClient.publish("test/lol", 0, false, "test 1");
    // Serial.println("Publishing at QoS 0");
    String *tempStr;
    bool dataAvailable = getJsonStringCb(&tempStr);
    Serial.print("available");
    Serial.println(dataAvailable);
    if (dataAvailable != 0)
    {
        const char *c_str = tempStr->c_str();
        uint16_t packetIdPub1 = mqttClient.publish("/topic/qos1", 1, false, c_str); // todo トピック名適当
        Serial.print("Publishing at QoS 1, packetId: ");
        Serial.println(packetIdPub1);
    }else{
        deleteBeforeKeepCb();
        xEventGroupSetBits(networkTaskFinished_event_group, MQTT_OK_BIT);
    }
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
    deleteBeforeKeepCb();
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    xEventGroupSetBits(networkTaskFinished_event_group, MQTT_OK_BIT);
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    printLocalTime();
    timeAvailable = true;
    xEventGroupSetBits(networkTaskFinished_event_group, NTP_OK_BIT);
}

void task_network(void *pvParameters)
{
    struct networkTaskArgs *argStruct = (networkTaskArgs *)pvParameters;

    networkTaskFinished_event_group = xEventGroupCreate();
    xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);

    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(mqttReconnectMillis), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(wifiReconnectMillis), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(WiFiEvent);
    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(HOSTNAME, MQTT_PORT);
    // mqttClient.setCredentials(USERNAME, PASSWORD);

    sntp_set_time_sync_notification_cb(timeavailable);
    sntp_servermode_dhcp(1);

    unsigned long preSendMqttMillis = millis();
    while (true)
    {
        if (((millis() - preSendMqttMillis) > sendMqttIntervalMillis) || sendNowFlag)
        {
            preSendMqttMillis = millis();
            sendNowFlag = false;
            xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);
            newtwork_keepConn = true;
            connectToWifi();
            uint32_t eBits = xEventGroupWaitBits(
                networkTaskFinished_event_group, // イベントグループを指定
                MQTT_OK_BIT | NTP_OK_BIT,        // 一つ以上のイベントビットを指定
                pdTRUE,                          // 呼び出し後にイベントビットをクリアするか
                pdTRUE,                          // 指定したイベントビットがすべて揃うまで待つか
                pdMS_TO_TICKS(netTaskTimeout)    // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
            );
            newtwork_keepConn = false;
            xTimerStop(wifiReconnectTimer, 0);
            xTimerStop(mqttReconnectTimer, 0);
            mqttClient.disconnect(false);
            WiFi.disconnect();
            WiFi.mode(WIFI_OFF);
            Serial.println("send finish");
        }
        xEventGroupSetBits(argStruct->sleepEventHandle, argStruct->net_ok_bit);
    }
    vTaskDelete(NULL);
}

bool getTimeAvailable(void)
{
    return timeAvailable;
}

void setSendNow(void)
{
    sendNowFlag = true;
    return;
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

uint64_t getMacaddress_int(void)
{
    uint8_t octet[6];
    WiFi.macAddress(octet);
    return octetToIntMac(octet);
}
