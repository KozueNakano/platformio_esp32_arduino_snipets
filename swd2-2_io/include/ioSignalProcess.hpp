#include <Arduino.h>
#include "driver/pcnt.h"

enum wakeupCause
{
  UNDEFINDE,
  SIG,
  SEND,
  MODE
};

void swIoSetting(void);
int updateModeSwitch(void);
wakeupCause getWakeupCause(void);
void enterLightSleep(uint64_t sleepUs,unsigned long swWakeQuietUs);
void counterInit(void);
void counterClear(void);
int16_t getCount(void);