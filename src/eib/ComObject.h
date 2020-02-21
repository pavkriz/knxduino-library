/*
 * CommObject.h
 *
 *  Created on: Feb 14, 2020
 *      Author: pavkriz
 */

#ifndef SRC_EIB_COMOBJECT_H_
#define SRC_EIB_COMOBJECT_H_

#include <stdint.h>
#include "types.h"

class ComObject {
public:
	/**
	 * Request the read of a communication object. 
	 * Calling this function triggers the sending of a read-group-value telegram, to read the value of the communication object from the bus.  
	 * When the answer is received, the communication object's value will be updated and the change handler called.
	 */
	void requestData();

	bool getAndClearTransmissionRequestedFlag();
	bool getAndClearDataRequestedFlag();
	uint16_t getGroupAddress();
	uint8_t getConfig();
	virtual void _callListener() = 0; // abstract
	virtual int getTelegramObjectSize() = 0; // abstract
	virtual uint8_t* getValuePtr() = 0; // abstract
protected:
	uint16_t groupAddress;	// group address associated
	uint8_t config = COMCONF_TRANS_COMM | COMCONF_READ_COMM | COMCONF_WRITE_COMM; // config flags such as eg. COMCONF_READ_COMM
	bool transmissionRequestedFlag = false;	// dirty flag indicating the value has been changed from the application and should be sent via bus
	bool dataRequestedFlag = false;		// flag from application indicating the data has been requested (group read request should be sent to bus)
};



#endif /* SRC_EIB_COMOBJECT_H_ */
