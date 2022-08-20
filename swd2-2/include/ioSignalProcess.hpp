#include <Arduino.h>
#include "driver/pcnt.h"

enum wakeupCause
{
  UNDEFINDE,
  SIG,
  TIMER,
  SEND,
  MODE,
};

enum stateNum
{
  NP,
  REST,
  DO
};

void swIoSetting(void);
int updateModeSwitch(void);
wakeupCause getWakeupCause(void);
void enterLightSleep(uint64_t sleepUs, unsigned long swWakeQuietUs);
void counterInit(void);
void counterClear(void);
int16_t getCount(void);
void updateModeState(void);
void updateModeToggleState(void);
stateNum getState(void);