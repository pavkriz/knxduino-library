/*
 * BinaryValue.h
 *
 *  Created on: Feb 14, 2020
 *      Author: pavkriz
 */

#ifndef SRC_EIB_BINARYVALUE_H_
#define SRC_EIB_BINARYVALUE_H_

#include <eib/ComObject.h>

// TODO make macro KNX_GROUP(1,2,2) returning 16bit address

class BinaryValue : public ComObject {
public:
	BinaryValue(int addr1, int addr2, int addr3, void (*changeListener)(bool newValue));
	void setValue(bool newValue);
	bool getValue();
	virtual void _callListener();
	virtual int getTelegramObjectSize();
	virtual uint8_t* getValuePtr();
private:
	uint8_t value = 0;
	void (*changeListener)(bool newValue);
};

#endif /* SRC_EIB_BINARYVALUE_H_ */
