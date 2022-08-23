#include <networkTask.hpp>

// wifi credential
#define WIFI_SSID "SWD2-2_AP"           //"mqtt_server_ssid"
#define WIFI_PASSWORD "switchDevice2-2" //"mosquitto"
// mqtt credential
#define HOSTNAME "GarmX280.local" //"raspberrypi.local"
#define MQTT_PORT 1883

const char *ntpServer1 = "GarmX280.local"; //"raspberrypi.local";
const char *time_zone = "JST-9";           // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)

EventGroupHandle_t networkTaskFinished_event_group;
#define MQTT_OK_BIT BIT0
#define NTP_OK_BIT BIT1

WiFiClient espClient;
PubSubClient client(espClient);

static bool timeAvailable = false;
static bool sendNowFlag = true;
static bool wifiConnected = false;

static unsigned long preSendMqttMillis = millis();
static const unsigned long sendMqttIntervalMillis = 60000;
static const unsigned long netTaskTimeout = 10000;

TimerHandle_t mqttConnectTimer;
static const unsigned long mqttConnectMillis = 2000;
TimerHandle_t ntpReconfigTimer;
static const unsigned long ntpReconfigMillis = 2000;

void (*keepIndexCb)(void);
void (*deleteBeforeKeepCb)(void);
void (*printArrayCb)(void);
bool (*getJsonStringCb)(String **);
void (*lcdSetNetStatusCb)(int);

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

void set_lcdSetNetStatusCb(void (*func)(int))
{
    lcdSetNetStatusCb = func;
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
    //Serial.println("Got time adjustment from NTP!");
    // printLocalTime();
    timeAvailable = true;
    xTimerStop(ntpReconfigTimer, 0);
    xEventGroupSetBits(networkTaskFinished_event_group, NTP_OK_BIT);
}

void ntpReconfig()
{
    if (!wifiConnected)
        return;
    configTzTime(time_zone, ntpServer1);
}

void mqttConnect()
{
    if (!wifiConnected)
        return;

    keepIndexCb();
    String *tempStr;
    bool dataAvailable = getJsonStringCb(&tempStr);
    if (dataAvailable != 0)
    {
        if (!client.connected())
        {
            // Create a random client ID
            String clientId = "ESP32Client-";
            clientId += String(random(0xffff), HEX);
            // Attempt to connect
            if (client.connect(clientId.c_str()))
            {
                //Serial.println("mqtt connected");
                const char *c_str = tempStr->c_str();
                // String tempMsg = "message";
                if (client.publish("outTopic", c_str /*tempMsg.c_str()*/))
                {
                    deleteBeforeKeepCb();
                    xEventGroupSetBits(networkTaskFinished_event_group, MQTT_OK_BIT);
                }
            }
            else
            {
                //Serial.print("mqtt failed, rc=");
                //Serial.print(client.state());
            }
        }
    }
    xEventGroupSetBits(networkTaskFinished_event_group, MQTT_OK_BIT);
    return;
}

void connectToWifi()
{
    //Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_STA_GOT_IP:
        //Serial.println("WiFi connected");
        wifiConnected = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        //Serial.println("WiFi lost connection");
        break;
    }
}

void task_network(void *pvParameters)
{
    //Serial.println("task start");
    struct networkTaskArgs *argStruct = (networkTaskArgs *)pvParameters;

    if (((millis() - preSendMqttMillis) > sendMqttIntervalMillis) || sendNowFlag)
    {
        preSendMqttMillis = millis();
        sendNowFlag = false;

        networkTaskFinished_event_group = xEventGroupCreate();
        xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);

        randomSeed(micros());

        WiFi.onEvent(WiFiEvent);

        sntp_set_time_sync_notification_cb(timeavailable);
        sntp_servermode_dhcp(1);
        wifiConnected = false;
        connectToWifi();

        client.setServer(HOSTNAME, MQTT_PORT);
        mqttConnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(mqttConnectMillis), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(mqttConnect));
        ntpReconfigTimer = xTimerCreate("ntpTimer", pdMS_TO_TICKS(ntpReconfigMillis), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(ntpReconfig));
        xTimerStart(mqttConnectTimer, 0);
        xTimerStart(ntpReconfigTimer, 0);
        uint32_t eBits = xEventGroupWaitBits(
            networkTaskFinished_event_group, // イベントグループを指定
            MQTT_OK_BIT | NTP_OK_BIT,        // 一つ以上のイベントビットを指定
            pdFALSE,                         // タイムアウト以外で戻った場合に、イベントフラグをクリアする
            pdTRUE,                          // 指定したイベントビットがすべて揃うまで待つか
            pdMS_TO_TICKS(netTaskTimeout)    // 待ち時間 portMAX_DELAY or / portTICK_TATE_MS
        );
        if ((eBits & (MQTT_OK_BIT | NTP_OK_BIT)) == (MQTT_OK_BIT | NTP_OK_BIT))
        {
            // Serial.println("net task SUCSESS");
            lcdSetNetStatusCb(2);
        }
        else
        {
            // Serial.println("network FAILS");
            lcdSetNetStatusCb(1);
        }
        xTimerStop(mqttConnectTimer, 0);
        xTimerStop(ntpReconfigTimer, 0);
        xEventGroupClearBits(networkTaskFinished_event_group, 0xFFFFFF);
        client.disconnect();
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        // Serial.println("send finish");
    }
    xEventGroupSetBits(argStruct->sleepEventHandle, argStruct->net_ok_bit);
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

void setMqttBufferSize(uint16_t size)
{
    if (client.setBufferSize(size))
    {
        //Serial.println("mqtt buffer fail");
    }
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
