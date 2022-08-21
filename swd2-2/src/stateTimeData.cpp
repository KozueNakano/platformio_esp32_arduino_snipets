#include "stateTimeData.hpp"

stateTimeData::stateTimeData(/* args */)
{
    arrayQty = sizeof(stateTimeArray) / sizeof(stateTimeArray[0]);
    index = -1;
    indexStore = -1;
    mutex = xSemaphoreCreateMutex();
}
int stateTimeData::getArrayQty(void)
{
    return arrayQty;
}
void stateTimeData::addData(stateTv argStr)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (index < arrayQty-1)//[|][|][|][-]
    {
        index++;
        stateTimeArray[index] = argStr;
    }
    
    xSemaphoreGive(mutex);
}

void stateTimeData::serialPrint(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    Serial.println();
    for (int i = 0; i < arrayQty; i++)
    {
        /*
        if (stateTimeArray[i].tv.tv_sec != 0)
        {*/
        Serial.print("pos:");
        Serial.print(i);
        Serial.print(" ,sec:");
        Serial.print(stateTimeArray[i].tv.tv_sec);
        Serial.print(",state:");
        Serial.println(stateTimeArray[i].state);
        /*}*/
    }
    Serial.print("index:");
    Serial.println(index);
    xSemaphoreGive(mutex);
}

int stateTimeData::getIndex(void)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    return index;
    xSemaphoreGive(mutex);
}

void stateTimeData::deleteData(int deleteIndex)
{
    //ガード
    if (deleteIndex >= arrayQty)
    {
        deleteIndex = arrayQty -1;
    }
    if (deleteIndex < 0)
    {
        deleteIndex = -1;
    }
    
    
    xSemaphoreTake(mutex, portMAX_DELAY);
    for (int i = 0; i < arrayQty; i++)
    {
        int readIndex = deleteIndex+1+i;
        if (readIndex < arrayQty)
        {
            stateTimeArray[i] = stateTimeArray[readIndex];
        }
        else
        {
            stateTimeArray[i].state = NP;
            stateTimeArray[i].tv.tv_sec = 0;
            stateTimeArray[i].tv.tv_usec = 0;
        }
    }
     
    if (deleteIndex <= index)
    {
        index = index-deleteIndex-1;
    }else{
        index = -1;
    }
       
    xSemaphoreGive(mutex);
}
void stateTimeData::keepIndex(void)
{
    indexStore = index;
}

void stateTimeData::deleteDataBeforeKeep(void)
{
    deleteData(indexStore);
    indexStore = -1;
}

int stateTimeData::requiredStringLength(void)
{
    int reserveLength = deviceNameHead.length() + /*SWDxxxx*/ 7 + deviceNameFoot.length() + timeHead.length() + 11 * arrayQty /*,4294967295*/ + timeFoot.length() + timeMsHead.length() + 11 * arrayQty /*,4294967295*/ + timeMsFoot.length() + stateHead.length() + 2 * arrayQty /*0,1,2*/ + stateFoot.length() + macHead.length() + 12 /*0xFFFFFFFFFFFF*/ + macFoot.length();
    return reserveLength;
}
bool stateTimeData::getJsonString(String *buffString, String *deviceName, uint64_t macaddress_arg)
{
    if (indexStore > -1)
    {
        xSemaphoreTake(mutex, portMAX_DELAY);
        buffString->clear();
        buffString->concat(deviceNameHead);
        buffString->concat(*deviceName);
        buffString->concat(deviceNameFoot);
        buffString->concat(timeHead);
        for (int i = 0; i <= indexStore; i++)
        {
            if (i != 0)
                buffString->concat(",");
            buffString->concat(String((stateTimeArray[i].tv.tv_sec), DEC));
        }
        buffString->concat(timeFoot);
        buffString->concat(timeMsHead);
        for (int i = 0; i <= indexStore; i++)
        {
            if (i != 0)
                buffString->concat(",");
            buffString->concat(String((stateTimeArray[i].tv.tv_usec / 1000), DEC));
        }
        buffString->concat(timeMsFoot);
        buffString->concat(stateHead);
        for (int i = 0; i <= indexStore; i++)
        {
            if (i != 0)
                buffString->concat(",");
            buffString->concat(String((stateTimeArray[i].state), DEC));
        }
        buffString->concat(stateFoot);
        buffString->concat(macHead);
        char macChar[13] = "";
        sprintf(macChar, "%012llx", macaddress_arg);
        buffString->concat(String(macChar));
        buffString->concat(macFoot);
        xSemaphoreGive(mutex);
        return true;
    }
    else
    {
        return false;
    }
}
stateTimeData::~stateTimeData()
{
}