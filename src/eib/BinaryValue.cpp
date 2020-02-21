/*
 * BinaryValue.cpp
 *
 *  Created on: Feb 14, 2020
 *      Author: pavkriz
 */

#include "BinaryValue.h"

BinaryValue::BinaryValue(int addr1, int addr2, int addr3, void (*changeListener)(bool newValue)) {
	this->groupAddress = addr1*256*16 + addr2*256 + addr3;
	this->changeListener = changeListener;
}

void BinaryValue::_callListener() {
	if (changeListener) {
		(changeListener)(value);
	}
}

void BinaryValue::setValue(bool newValue) {
	if (newValue != value) {
		value = newValue;
		transmissionRequestedFlag = true;
	}
}

bool BinaryValue::getValue() {
	return value;
}

int BinaryValue::getTelegramObjectSize() {
	return 0;
}

uint8_t* BinaryValue::getValuePtr() {
	return &value;
}