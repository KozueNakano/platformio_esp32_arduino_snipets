// ioSignalProcess.hpp
#ifndef IO_SIGNAL_PROCESS
#define IO_SIGNAL_PROCESS
#include <Arduino.h>
#include "driver/pcnt.h"
#include "stateNum.hpp"

enum wakeupCause
{
  UNDEFINDE,
  SIG,
  TIMER,
  SEND,
  MODE,
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

#endif