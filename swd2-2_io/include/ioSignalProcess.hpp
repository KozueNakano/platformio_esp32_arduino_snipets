#include <Arduino.h>

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