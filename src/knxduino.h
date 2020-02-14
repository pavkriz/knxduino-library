/*
 * knxduino.h
 *
 *  Created on: 15.07.2015
 *      Author: glueck, pavkriz
 */

#ifndef KNXDUINO_H_
#define KNXDUINO_H_

#ifdef STM32G0xx
#else
#error "Only STM32G0xx devices are supported"
#endif

#include "eib.h"
#include "eib/bus_hal.h"
//#include <sblib/io_pin_names.h>

#ifndef IS_BOOTLOADER

static BCU _bcu = BCU();
BcuBase& bcu = _bcu;

// The EIB bus access objects
BusHal knxBusHal;
Bus knxBus(knxBusHal);
//Bus bus(timer16_1, PIN_EIB_RX, PIN_EIB_TX, CAP0, MAT0);


/*
 * Global HAL IRQ Callbacks.
 * May lead to duplicate definition in case it will
 * be implemented in STM32duino Core in future.
 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim)
{
    // TODO filter particular timer ?
    knxBusHal.isrCallbackCapture(htim);
    knxBus.timerInterruptHandler();  
}

void knxBusIsrArduinoTimerUpdateCallback(stimer_t *obj)
{
    // TODO filter particular timer ?
    knxBusHal.isrCallbackUpdate(&obj->handle);
    knxBus.timerInterruptHandler();
}

void attachKnxBusTimerUpdateIntHandle() {
  attachIntHandle(&knxBusHal._timer, knxBusIsrArduinoTimerUpdateCallback);
}

//extern "C"
void yield(void)
{
  bcu.loop();
}

#else

extern BusHal knxBusHal;
extern Bus knxBus;

#endif

#endif /* KNXDUINO_H_ */
