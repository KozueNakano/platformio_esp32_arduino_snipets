#include <Arduino.h>
#include "stateNum.hpp"

String getStateNumCh(stateNum temp)
{
    String tempString;
    switch (temp)
    {
    case NP:
        tempString = "NP";
        break;
    case REST:
        tempString = "REST";
        break;
    case DO:
        tempString = "DO";
        break;
    default:
        break;
    }

    return tempString;
}