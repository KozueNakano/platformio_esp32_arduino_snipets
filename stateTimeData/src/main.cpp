#include <Arduino.h>
#include "stateTimeData.hpp"

stateTimeData stateTime;
String jsonStringBuffer;

void setup()
{

  Serial.begin(115200);
}

void loop()
{
  // put your main code here, to run repeatedly:
  int qty = 10;
  int temp = 15 + (7 /*SWDxxxx*/) + 1 + 17 + (11 * qty /*,4294967295*/) + 1 + 20 + (11 * qty /*,4294967295*/) + 1 + 10 + (2 * qty /*0,1,2*/) + 1 + 15 + 12 /*0xFFFFFFFFFFFF*/ + 2;
  Serial.print("length get Test:exp=");
  Serial.print(temp);
  Serial.print(" , res=");
  Serial.println(stateTime.requiredStringLength());

  Serial.print("reserve Test:exp= reaserve sucsess");
  Serial.print(true);
  Serial.print(" , res=");
  Serial.println(jsonStringBuffer.reserve(stateTime.requiredStringLength()));

  stateTv temptv;
  temptv.state = DO;
  temptv.tv.tv_sec = 2147483647;
  temptv.tv.tv_usec = 2147483647;
  Serial.println("add and print Test");
  for (int i = 0; i < 10; i++)
  {
    temptv.tv.tv_sec = i + 1;
    stateTime.addData(temptv);
  }
  stateTime.serialPrint();
  Serial.flush();

  Serial.println();
  Serial.println("delete and print Test");
  stateTime.deleteData(5);
  stateTime.serialPrint();
  Serial.flush();

  Serial.println();
  Serial.println("delete and print Test2");
  stateTime.deleteData(10);
  stateTime.serialPrint();
  Serial.flush();

  temptv.state = DO;
  temptv.tv.tv_sec = 0;
  temptv.tv.tv_usec = 0;
  Serial.println();
  Serial.println("add and keepIndex and deleteBeforeKeepTest");
  for (int i = 0; i < 5; i++)
  {
    temptv.tv.tv_sec = i;
    stateTime.addData(temptv);
  }
  stateTime.serialPrint();
  Serial.flush();

  Serial.print("keep");
  stateTime.keepIndex();
  Serial.println("add");
  for (int i = 0; i < 2; i++)
  {
    temptv.tv.tv_sec = 10 + i;
    stateTime.addData(temptv);
  }
  stateTime.serialPrint();
  Serial.flush();

  Serial.print("keep2");
  stateTime.keepIndex();
  Serial.println("add");
  for (int i = 0; i < 3; i++)
  {
    temptv.tv.tv_sec = 20 + i;
    stateTime.addData(temptv);
  }
  stateTime.serialPrint();
  Serial.flush();

  Serial.println("delete before keep");
  stateTime.deleteDataBeforeKeep();
  stateTime.serialPrint();
  Serial.flush();

  Serial.println("over Add Test");
  Serial.println("over delete.");
  stateTime.deleteData(20);
  stateTime.serialPrint();
  Serial.flush();
  Serial.println("over add.");
  for (int i = 0; i < 30; i++)
  {
    temptv.tv.tv_sec = i;
    stateTime.addData(temptv);
  }
  stateTime.serialPrint();
  Serial.flush();

  Serial.println();
  Serial.println("dataAvailableTest Public IF");
  Serial.println("delete before keep");
  stateTime.keepIndex();
  stateTime.deleteDataBeforeKeep();
  stateTime.serialPrint();
  Serial.flush();
  Serial.println("availability : ");
  String deviceName = "SWD_T";
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));

  Serial.println("add ones.");
  temptv.tv.tv_sec = 0;
  stateTime.addData(temptv);
  stateTime.serialPrint();
  Serial.flush();
  Serial.println("availability : ");
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));

  Serial.println("keep.");
  stateTime.keepIndex();
  Serial.println("availability : ");
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));
  Serial.print(jsonStringBuffer);

  Serial.println("add twis.");
  temptv.tv.tv_sec = 1;
  stateTime.addData(temptv);
  stateTime.serialPrint();
  Serial.flush();
  Serial.println("availability : ");
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));

  Serial.println("delete before keep");
  stateTime.deleteDataBeforeKeep();
  stateTime.serialPrint();
  Serial.flush();
  Serial.println("availability : ");
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));

  Serial.println("keep.");
  stateTime.keepIndex();
  Serial.println("availability : ");
  Serial.println(stateTime.getJsonString(&jsonStringBuffer,&deviceName, 0xFFFFFFFFFFFF));
  Serial.print(jsonStringBuffer);


  delay(10000);
}