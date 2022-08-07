#include <Arduino.h>

class signalDeadtime
{
private:
  /* data */
  bool trueSignal;
  unsigned long preMillis;
  unsigned long deadTime;

public:
  signalDeadtime();
  void init(bool trueSignal_arg, unsigned long deadTIme_arg);
  bool getState(bool signal, unsigned long nowMills);
  ~signalDeadtime();
};

signalDeadtime::signalDeadtime()
{
  trueSignal = true;
  preMillis = 0;
  deadTime = 0;
}

void signalDeadtime::init(bool trueSignal_arg, unsigned long deadTIme_arg)
{
  trueSignal = trueSignal_arg;
  deadTime = deadTIme_arg;
}

bool signalDeadtime::getState(bool signal, unsigned long nowMills)
{
  if (nowMills - preMillis > deadTime) // overFlowProof
  {
    if (signal == trueSignal)
    {
      preMillis = nowMills;
      return true;
    }
  }
  return false;
}

signalDeadtime::~signalDeadtime()
{
}

signalDeadtime deadtime25;
signalDeadtime deadtime26;
signalDeadtime deadtime27;
signalDeadtime deadtime14;

int a = 3 << 8;
uint64_t bitmask26 = 0b1 << 26;
uint64_t bitmask27 = 0b1 << 27;
uint64_t bitmask14 = 0b1 << 14;

void getWakeupIo(bool *IO25,bool *IO26,bool *IO27, bool *IO14);

int mode = 0;


void setup()
{
  Serial.begin(115200);
  // esp_sleep_enable_timer_wakeup(1000000);
  pinMode(25, INPUT_PULLUP);
  // gpio_wakeup_enable(GPIO_NUM_25, GPIO_INTR_LOW_LEVEL);
  // esp_sleep_enable_gpio_wakeup();
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_25, LOW);
  pinMode(26, INPUT_PULLDOWN);
  pinMode(27, INPUT_PULLDOWN);
  pinMode(14, INPUT_PULLDOWN);
  deadtime25.init(true, 5000);
  deadtime26.init(true, 5000);
  deadtime27.init(true, 5000);
  deadtime14.init(true, 5000);
}

void loop()
{
  if (mode == 0)
  {
    /* code */
    esp_sleep_enable_ext1_wakeup(BIT64(GPIO_NUM_26) | BIT64(GPIO_NUM_27), ESP_EXT1_WAKEUP_ANY_HIGH);
    mode = 1;
  }else
  {
    /* code */
    esp_sleep_enable_ext1_wakeup(BIT64(GPIO_NUM_26) | BIT64(GPIO_NUM_14), ESP_EXT1_WAKEUP_ANY_HIGH);
    mode = 0;
  }
  
  
  esp_light_sleep_start();
  Serial.println("wake");
  Serial.flush();

  bool IO25_signal = false; // LOW  = true
  bool IO26_signal = false; // HIGH = true
  bool IO27_signal = false; // HIGH = true
  bool IO14_signal = false; // HIGH = true

  getWakeupIo(&IO25_signal,&IO26_signal,&IO27_signal, &IO14_signal);

  Serial.printf("IO25:%d\n", IO25_signal);
  Serial.printf("IO26:%d\n", IO26_signal);
  Serial.printf("IO27:%d\n", IO27_signal);
  Serial.printf("IO14:%d\n", IO14_signal);
  Serial.printf("mode:%d\n",mode);
  Serial.flush();

  if (deadtime25.getState(IO25_signal, millis()) == true)
  {
    Serial.println("---------------------25 signal.");
    Serial.flush();
  }
  if (deadtime26.getState(IO26_signal, millis()) == true)
  {
    Serial.println("---------------------26 signal.");
    Serial.flush();
  }
}

void getWakeupIo(bool *IO25,bool *IO26,bool *IO27, bool *IO14)
{

  // 起動理由取得
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  uint64_t ext1_wakeup_status = esp_sleep_get_ext1_wakeup_status();
  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    *IO25 = true;
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    if (ext1_wakeup_status & bitmask26)
      *IO26 = true;
    if (ext1_wakeup_status & bitmask27)
      *IO27 = true;
    if (ext1_wakeup_status & bitmask14)
      *IO14 = true;
    break;
  default:
    break;
  }
}