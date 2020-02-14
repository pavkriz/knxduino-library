#ifndef sblib_platform_h
#define sblib_platform_h

#ifdef STM32G0xx
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_flash_ex.h"
#include "stm32g0xx_ll_comp.h"
#else
#error "Only STM32G0xx devices are supported"
#endif

void Error_Handler();

#endif
