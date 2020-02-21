#include "ComObject.h"

bool ComObject::getAndClearTransmissionRequestedFlag() {
    bool v = transmissionRequestedFlag;
    transmissionRequestedFlag = false;
    return v;
}

bool ComObject::getAndClearDataRequestedFlag() {
    bool v = dataRequestedFlag;
    dataRequestedFlag = false;
    return v;
}

uint16_t ComObject::getGroupAddress() {
    return groupAddress;
}

uint8_t ComObject::getConfig() {
    return config;
}

void ComObject::requestData() {
    dataRequestedFlag = true;
}