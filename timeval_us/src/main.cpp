#include <Arduino.h>

#include <WiFi.h>
#include "time.h"
#include "sntp.h"

const char *ssid = "mqtt_server_ssid";
const char *password = "mosquitto";

const char *ntpServer1 = "raspberrypi.local";
const char *time_zone = "JST-9"; // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


bool timeAvailable = false;
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
  WiFi.mode(WIFI_OFF);
  timeAvailable = true;
}

struct timeval pre_tv;
unsigned long preMillis;

void setup()
{
  Serial.begin(115200);

  // set notification call-back function
  sntp_set_time_sync_notification_cb(timeavailable);

  /**
   * NTP server address could be aquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 aquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE aquired NTP server address
   */
  sntp_servermode_dhcp(1); // (optional)

  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  // configTzTime(time_zone, ntpServer1, ntpServer2);
  configTzTime(time_zone, ntpServer1);

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  while (timeAvailable != true)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" AVAILABLE");
  int timeget = gettimeofday(&pre_tv, NULL);
  preMillis = millis();
  esp_sleep_enable_timer_wakeup(5000000);
}

/*
memo
struct timeval tv;
int state;//0:non , 1:toOn , 2:toOff
*/

void loop()
{

  struct timeval tv;
  int timeget = gettimeofday(&tv, NULL);
  unsigned long currentMillis = millis();

  unsigned long diffSec_part = tv.tv_sec - pre_tv.tv_sec;
  unsigned long diffUs_part = tv.tv_usec - pre_tv.tv_usec;
  unsigned long diff_tv_millis = (diffSec_part*1000) + (diffUs_part/1000);
  unsigned long diffMillis = currentMillis - preMillis;
  long long conpare = (long)diff_tv_millis - (long)diffMillis;

  if (timeget != 0)
  {
    Serial.println("Failed to obtain time");
  }
  else
  {
    Serial.printf("diff timeval millis:%lu\n",diff_tv_millis);
    Serial.printf("diff millis:%lu\n",diffMillis);
    Serial.printf("timeval - millis:%ld\n",conpare);
    /*
    Serial.printf("%lld\n", (long long)tv.tv_sec);
    Serial.printf("%lld\n", (long long)tv.tv_usec);
    */
  }

  // delay(5000);
  printLocalTime(); // it will take some time to sync time :)

  Serial.flush();
  esp_light_sleep_start();
}