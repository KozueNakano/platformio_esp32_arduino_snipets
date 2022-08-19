#include "ioSignalProcess.hpp"

static const int mode0Pin = 14;
static const int mode1Pin = 27;
static const int sendPin = A0;
static const int sigPin = A1;

static const uint64_t mode0Pin_mask = 0b1 << mode0Pin;
static const uint64_t mode1Pin_mask = 0b1 << mode1Pin;
static const uint64_t sendPin_mask = 0b1 << sendPin;

void swIoSetting(void)
{
  pinMode(mode0Pin, INPUT);
  pinMode(mode1Pin, INPUT);
  pinMode(sendPin, INPUT);
  pinMode(sigPin, INPUT);
  gpio_set_pull_mode((gpio_num_t)mode0Pin, GPIO_PULLDOWN_ONLY);
  gpio_set_pull_mode((gpio_num_t)mode1Pin, GPIO_PULLDOWN_ONLY);
  gpio_set_pull_mode((gpio_num_t)sendPin, GPIO_PULLDOWN_ONLY);
  gpio_set_pull_mode((gpio_num_t)sigPin, GPIO_PULLUP_ONLY);
  gpio_hold_en((gpio_num_t)mode0Pin);
  gpio_hold_en((gpio_num_t)mode1Pin);
  gpio_hold_en((gpio_num_t)sendPin);
  gpio_hold_en((gpio_num_t)sigPin);
  gpio_deep_sleep_hold_en();
}

int updateModeSwitch(void)
{
  bool mode0PinState = LOW; // pulldown
  bool mode1PinState = LOW;
  bool mode0PinState_late = LOW;
  bool mode1PinState_late = LOW;
  do
  {
    mode0PinState = digitalRead(mode0Pin);
    mode1PinState = digitalRead(mode1Pin);
    delay(10);
    mode0PinState_late = digitalRead(mode0Pin);
    mode1PinState_late = digitalRead(mode1Pin);

  } while ((mode0PinState == mode1PinState) || (mode0PinState_late == mode1PinState_late) || (mode0PinState != mode0PinState_late) || (mode1PinState != mode1PinState_late));

  // mode間違ってるとwakeupしないんじゃなくて、即wakeuploopする
  if (mode1PinState == HIGH)
  {
    esp_sleep_enable_ext1_wakeup((gpio_num_t)(BIT64(mode0Pin) | BIT64(sendPin)), ESP_EXT1_WAKEUP_ANY_HIGH);
    return 1;
  }
  else
  {
    esp_sleep_enable_ext1_wakeup((gpio_num_t)(BIT64(mode1Pin) | BIT64(sendPin)), ESP_EXT1_WAKEUP_ANY_HIGH);
    return 0;
  }
}

wakeupCause getWakeupCause(void)
{
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  uint64_t ext1_wakeup_status = esp_sleep_get_ext1_wakeup_status();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    return SIG;
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    if (ext1_wakeup_status & mode0Pin_mask)
      return MODE;
    if (ext1_wakeup_status & mode1Pin_mask)
      return MODE;
    if (ext1_wakeup_status & sendPin_mask)
      return SEND;
    break;
  case ESP_SLEEP_WAKEUP_TIMER:
    return SEND;
    break;
  default:
    return UNDEFINDE;
    break;
  }
  return UNDEFINDE;
}

void enterLightSleep(uint64_t sleepUs, unsigned long swWakeQuietUs)
{
  if (swWakeQuietUs > 0)
  {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
    esp_sleep_enable_timer_wakeup(swWakeQuietUs);
    esp_light_sleep_start();
  }

  esp_sleep_enable_ext0_wakeup((gpio_num_t)sigPin, LOW);
  updateModeSwitch();
  esp_sleep_enable_timer_wakeup(sleepUs);
  esp_light_sleep_start();
}