/*
 * ComObjects.cpp
 *
 *  Created on: Feb 15, 2020
 *      Author: pavkriz
 */

#include "ComObjects.h"
#include "ComObject.h"
#include "types.h"
#include "apci.h"
#include "../utils.h"
#include "bcu.h"

ComObjects::ComObjects() {
}

void ComObjects::processGroupWriteTelegram(BCU *bcu, ComObject *comObject, uint8_t* tel) {
	byte* valuePtr = comObject->getValuePtr();
    int sz = comObject->getTelegramObjectSize();

	if (sz > 0) reverseCopy(valuePtr, tel + 8, sz);
	else *valuePtr = tel[7] & 0x3f;

	// TODO call listener only in case the value has actually changed
	comObject->_callListener();
}

void ComObjects::processGroupTelegram(BCU *bcu, int addr, int apci, uint8_t* tel) {
	for (int i = 0; i < comObjectsCount; i++) {
		ComObject *o = comObjectsArray[i];
		if (addr == o->getGroupAddress()) {
			if (apci == APCI_GROUP_VALUE_WRITE_PDU || apci == APCI_GROUP_VALUE_RESPONSE_PDU) {
				// Check if communication and write are enabled
				if ((o->getConfig() & COMCONF_WRITE_COMM) == COMCONF_WRITE_COMM)
					processGroupWriteTelegram(bcu, o, tel);
			} else if (apci == APCI_GROUP_VALUE_READ_PDU) {
				// Check if communication and read are enabled
				if ((o->getConfig() & COMCONF_READ_COMM) == COMCONF_READ_COMM)
					sendGroupWriteTelegram(bcu, o, true);
			}
		}
	}	    
}

bool ComObjects::sendNextGroupTelegram(BCU *bcu) {
	for (int i = sndStartIdx; i < comObjectsCount; i++) {
		ComObject *o = comObjectsArray[i];
		if (o->getAndClearTransmissionRequestedFlag()) {
			if (o->getGroupAddress() == 0 || !(o->getConfig() & COMCONF_COMM)) {
				continue; // skip, no group address associated or communication disabled
			}
			if (o->getAndClearDataRequestedFlag()) {
				sendGroupReadTelegram(bcu, o);	// send Group Read Request since com object value update was requested by the application
			} else if (o->getConfig() & COMCONF_TRANS) {
				sendGroupWriteTelegram(bcu, o, false);	// send Group Write Request since com object was modified and the new value should be sent to bus
			} else {
				continue;
			}

			// this happens when we've send a telegram (read or write request)
			sndStartIdx = i + 1;	// in next iteration continue with next com objact ("round robin")
			return true;
		}
	}
	sndStartIdx = 0;
	return false;
}

void ComObjects::sendGroupReadTelegram(BCU *bcu, ComObject *comObject)
{
    bcu->sendTelegram[0] = 0xBC; // Control byte
    // 1+2 contain the sender address, which is set by knxBus.sendTelegram()
    bcu->sendTelegram[3] = comObject->getGroupAddress() >> 8;
    bcu->sendTelegram[4] = comObject->getGroupAddress();
    bcu->sendTelegram[5] = 0xE1;
    bcu->sendTelegram[6] = 0;
    bcu->sendTelegram[7] = 0x00;

    knxBus.sendTelegram(bcu->sendTelegram, 8);
}

void ComObjects::sendGroupWriteTelegram(BCU *bcu, ComObject *comObject, bool isResponse)
{
    byte* valuePtr = comObject->getValuePtr();
    int sz = comObject->getTelegramObjectSize();

    bcu->sendTelegram[0] = 0xBC; // Control byte
    // 1+2 contain the sender address, which is set by knxBus.sendTelegram()
    bcu->sendTelegram[3] = comObject->getGroupAddress() >> 8;
    bcu->sendTelegram[4] = comObject->getGroupAddress();
    bcu->sendTelegram[5] = 0xE0 | ((sz + 1) & 15);
    bcu->sendTelegram[6] = 0;
    bcu->sendTelegram[7] = isResponse ? 0x40 : 0x80;

    if (sz) reverseCopy(bcu->sendTelegram + 8, valuePtr, sz);
    else bcu->sendTelegram[7] |= *valuePtr & 0x3F;

    // Process this telegram in the receive queue (if there is a local receiver of this group address)
    processGroupTelegram(bcu, comObject->getGroupAddress(), APCI_GROUP_VALUE_WRITE_PDU, bcu->sendTelegram);

    knxBus.sendTelegram(bcu->sendTelegram, 8 + sz);
}

void ComObjects::addObject(ComObject* comObject) {
	if (comObjectsCount < COM_OBJECTS_MAX_COUNT) {
		comObjectsArray[comObjectsCount++] = comObject;
	}
}

bool ComObjects::containsGroupAddress(int addr) {
	for (int i = 0; i < comObjectsCount; i++) {
		ComObject *o = comObjectsArray[i];
		if (o->getGroupAddress() == addr) {
			return true;
		}
	}
	return false;
}