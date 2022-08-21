// stateTImeData.hpp
#ifndef STATE_TIME_DATA
#define STATE_TIME_DATA
#include <Arduino.h>
#include "stateNum.hpp"

struct stateTv
{
  stateNum state;
  timeval tv;
};

class stateTimeData
{
private:
  SemaphoreHandle_t mutex;
  stateTv stateTimeArray[512];
  int arrayQty;
  int index;
  int indexStore;
  const String deviceNameHead = "{\"deviceName\":\"";
  const String deviceNameFoot = "\"";
  const String timeHead = ",\"detectedTime\":[";
  const String timeFoot = "]";
  const String timeMsHead = ",\"detectedTime_ms\":[";
  const String timeMsFoot = "]";
  const String stateHead = ",\"state\":[";
  const String stateFoot = "]";
  const String macHead = ",\"macAddress\":\"";
  const String macFoot = "\"}";
    
public:
  stateTimeData(/* args */);
  int getArrayQty(void);
  void addData(stateTv);
  void serialPrint(void);
  int getIndex(void);
  void deleteData(int deleteBeforeIndex);
  void keepIndex(void);
  void deleteDataBeforeKeep(void);
  int requiredStringLength(void);
  bool getJsonString(String *buffString, String *deviceName, uint64_t macaddress);

  ~stateTimeData();
};
#endif