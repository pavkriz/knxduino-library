/*
 * ComObjects.h
 *
 *  Created on: Feb 15, 2020
 *      Author: pavkriz
 */

#ifndef SRC_EIB_COMOBJECTS_H_
#define SRC_EIB_COMOBJECTS_H_

#include "ComObject.h"

#define COM_OBJECTS_MAX_COUNT 255

class BCU;

class ComObjects {
public:
	ComObjects();
	void processGroupTelegram(BCU *bcu, int addr, int apci, uint8_t* tel);
	bool sendNextGroupTelegram(BCU *bcu);
	void addObject(ComObject* comObject);
	bool containsGroupAddress(int addr);
private:
	int comObjectsCount = 0;
	int sndStartIdx = 0;
	void processGroupWriteTelegram(BCU *bcu, ComObject *comObject, uint8_t* tel);
	void sendGroupReadTelegram(BCU *bcu, ComObject *comObject);
	void sendGroupWriteTelegram(BCU *bcu, ComObject *comObject, bool isResponse);
	ComObject* comObjectsArray[COM_OBJECTS_MAX_COUNT];
};



#endif /* SRC_EIB_COMOBJECTS_H_ */
