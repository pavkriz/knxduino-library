/*
 *  bus_hal.h - Hardware abstraction layer for Low level EIB bus access.
 *              It encapsulates timer and GPIO-related operations.
 *              Intended to be re-implemented for different platforms.
 *              In general, it requires a 16-bit timer with capture input (+interrupt), 
 *              match to output (PWM), and match to interrupt capabilities.
 
 *              We use the preprocessor here in order to avoid virtual methods 
 *              (and their performance penalty). May be a subject to reconsider.
 *              Probably make the Bus class a template in order to hint the compiler
 *              which BusHal implementation is used.
 *              
 *
 *  Copyright (c) 2018 Pavel Kriz <pavkriz@hkfree.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3 as
 *  published by the Free Software Foundation.
 */
#ifndef sblib_bus_hal_h
#define sblib_bus_hal_h

#if defined(STM32G071xx)

#include "../platform.h"
#include "../timer.h"

#ifdef IS_BOOTLOADER

typedef struct timer_s stimer_t;

struct timer_s{
  /*  The 1st 2 members TIM_TypeDef *timer
     *  and TIM_HandleTypeDef handle should
     *  be kept as the first members of this struct
     *  to have get_timer_obj() function work as expected
     */
  TIM_TypeDef *timer;
  TIM_HandleTypeDef handle;
  uint8_t idx;
  void (*irqHandle)(stimer_t *);
  void (*irqHandleOC)(stimer_t *, uint32_t);
  //PinName pin;
  //volatile timerPinInfo_t pinInfo;
};

#else

#include <Arduino.h>

#endif

/*
 *    STM32 implementation
 */

class BusHal
{
public:
    BusHal();
    void begin();
    void idleState();
    void waitBeforeSending(unsigned int timeValue);
    bool isCaptureChannelFlag();
    bool isTimeChannelFlag();
    void setupTimeChannel(unsigned int value);
    unsigned int getCaptureValue();
    void setTimeMatch(unsigned int value);
    void setupCaptureWithInterrupt();
    void setupCaptureWithoutInterrupt();
    void resetFlags();
    void disableInterrupts();
    void enableInterrupts();
    void startSending();
    void setupStartBit(int pwmMatch, int timeMatch);
    unsigned int getTimerValue();
    unsigned int getPwmMatch();
    void setPwmMatch(unsigned int pwmMatch);
    void isrCallbackCapture(TIM_HandleTypeDef* ahtim);
    void isrCallbackUpdate(TIM_HandleTypeDef* ahtim);
    void _Error_Handler(char* filename, int line);
    stimer_t _timer; // TODO static?
protected:
    int rxTimerValue = 0;
    bool captureChannelFlag = false;
    bool timeChannelFlag = false;

};

inline void BusHal::begin()
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    DAC_HandleTypeDef hdac1;

    GPIO_InitTypeDef GPIO_InitStruct = {0};


    __HAL_RCC_DAC1_CLK_ENABLE();
  
    DAC_ChannelConfTypeDef sConfig = {0};

    /** DAC Initialization 
    */
    hdac1.Instance = DAC1;
    if (HAL_DAC_Init(&hdac1) != HAL_OK)
    {
      Error_Handler();
    }
    /** DAC channel OUT1 config 
    */
    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_ENABLE;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
    {
      Error_Handler();
    }

    HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
    HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_8B_R, 62);  // arbitrary value to detect falling edge on KNX bus

    /**COMP1 GPIO Configuration    
    PA0     ------> COMP1_OUT
    PA1     ------> COMP1_INP 
    */
    // TODO migrate HAL_GPIO_Init to LL

    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_COMP1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    LL_COMP_ConfigInputs(COMP1, LL_COMP_INPUT_MINUS_DAC1_CH1, LL_COMP_INPUT_PLUS_IO3);
    LL_COMP_SetInputHysteresis(COMP1, LL_COMP_HYSTERESIS_HIGH);
    LL_COMP_SetOutputPolarity(COMP1, LL_COMP_OUTPUTPOL_NONINVERTED);
    LL_COMP_SetOutputBlankingSource(COMP1, LL_COMP_BLANKINGSRC_NONE);
    LL_COMP_SetPowerMode(COMP1, LL_COMP_POWERMODE_HIGHSPEED);
    LL_COMP_SetCommonWindowMode(__LL_COMP_COMMON_INSTANCE(COMP1), LL_COMP_WINDOWMODE_DISABLE);
    LL_COMP_SetCommonWindowOutput(__LL_COMP_COMMON_INSTANCE(COMP1), LL_COMP_WINDOWOUTPUT_EACH_COMP);

    /* Wait loop initialization and execution */
    /* Note: Variable divided by 2 to compensate partially CPU processing cycles */
    __IO uint32_t wait_loop_index = 0;
    wait_loop_index = (LL_COMP_DELAY_VOLTAGE_SCALER_STAB_US * (SystemCoreClock / (1000000 * 2)));
    while(wait_loop_index != 0)
    {
      wait_loop_index--;
    }

    LL_COMP_Enable(COMP1);

    // TIM init

    // Arduino interfacing for UPDATE event callback that is already handled by Arduino STM32 core,
    // beware, may interfere with libraries using STM32 hardware timers (PWM, Servo, SoftSerial,...)
    _timer.timer = TIM15;
    // mimics TimerHandleInit:
    TIM_HandleTypeDef *htim = &(_timer.handle);

    // HAL stuff
    __HAL_RCC_TIM15_CLK_ENABLE();
  
    /**TIM15 GPIO Configuration    
    PA2     ------> TIM15_CH1 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /**TIM15 GPIO Configuration    
    PA3     ------> TIM15_CH2 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_TIM15;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(TIM15_IRQn);

    htim->Instance = TIM15;
    htim->Init.Prescaler = 64; // 64MHz/64 = 1MHz (ie. 1us tick) TODO calc base on real CLK
    htim->Init.CounterMode = TIM_COUNTERMODE_UP;
    htim->Init.Period = 65535; // 2^16-1
    htim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim->Init.RepetitionCounter = 0;
    htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(htim) != HAL_OK) {
        _Error_Handler(__FILE__, __LINE__);
    }
    if (HAL_TIM_IC_Init(htim) != HAL_OK)
    {
      Error_Handler();
    }
    if (HAL_TIM_PWM_Init(htim) != HAL_OK)
    {
      Error_Handler();
    }

    TIM_IC_InitTypeDef sConfigIC;
    TIM_OC_InitTypeDef sConfigOC;

    sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
    sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
    sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
    sConfigIC.ICFilter = 4;
    if (HAL_TIM_IC_ConfigChannel(htim, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
    {
      Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(htim, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
      Error_Handler();
    }
    // the stupid HAL_TIM_PWM_ConfigChannel automaticaly enables TIM_CCMR1_OC2PE preload bit, but we dont want to use it
    htim->Instance->CCMR1 &= ~TIM_CCMR1_OC2PE;  // disable TIM_CCMR1_OC2PE
    
    if (HAL_TIM_Base_Start(htim) != HAL_OK) {
      Error_Handler();
    }

    if (HAL_TIM_IC_Start_IT(htim, TIM_CHANNEL_1) != HAL_OK) {
      Error_Handler();
    }
    // TODO? make sure we don't make any unintentional pulse here
    if (HAL_TIM_PWM_Start(htim, TIM_CHANNEL_2) != HAL_OK) {
      Error_Handler();
    }

}

inline void BusHal::idleState()
{
    _timer.handle.Instance->ARR = 0xfffe;
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_CC1);
    __HAL_TIM_DISABLE_IT(&_timer.handle, TIM_IT_UPDATE);
    _timer.handle.Instance->CCR2 = 0xffff;   // no pulses
}

inline void BusHal::waitBeforeSending(unsigned int timeValue) 
{
    _timer.handle.Instance->ARR = (uint32_t)timeValue;
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_UPDATE); // clear UPDATE interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_UPDATE); // enable UPDATE interrupt
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_CC1); // clear Capture Channel 1 interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_CC1); // enable Channel 1 interrupt flag here
}

inline bool BusHal::isCaptureChannelFlag()
{
    return captureChannelFlag;
}

inline bool BusHal::isTimeChannelFlag()
{
    return timeChannelFlag;
}

inline void BusHal::setupTimeChannel(unsigned int value)
{
    _timer.handle.Instance->ARR = (uint32_t)value; // new period
    _timer.handle.Instance->CNT = 0; // reset counter
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_UPDATE); // clear UPDATE interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_UPDATE); // enable UPDATE interrupt
}

inline unsigned int BusHal::getCaptureValue()
{
    return _timer.handle.Instance->CCR1;
}

inline void BusHal::setupStartBit(int pwmMatch, int timeMatch) 
{
    _timer.handle.Instance->CCR2 = (uint32_t)pwmMatch;
    _timer.handle.Instance->ARR = (uint32_t)timeMatch; // new period
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_UPDATE); // clear UPDATE interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_UPDATE); // enable UPDATE interrupt
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_CC1); // clear Capture Channel 1 interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_CC1); // enable Channel 1 interrupt flag here
}

inline unsigned int BusHal::getTimerValue()
{
    return _timer.handle.Instance->CNT;
}

inline unsigned int BusHal::getPwmMatch()
{
    return _timer.handle.Instance->CCR2;
}

inline void BusHal::setPwmMatch(unsigned int pwmMatch)
{
    _timer.handle.Instance->CCR2 = (uint32_t)pwmMatch;
}

inline void BusHal::setTimeMatch(unsigned int timeMatch)
{
    _timer.handle.Instance->ARR = (uint32_t)timeMatch; // new period
}

inline void BusHal::setupCaptureWithInterrupt()
{
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_CC1); // clear Capture Channel 1 interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_CC1); // enable Channel 1 interrupt flag here
}

inline void BusHal::setupCaptureWithoutInterrupt()
{
    __HAL_TIM_DISABLE_IT(&_timer.handle, TIM_IT_CC1); // disable Channel 1 interrupt flag here
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_CC1); // clear Capture Channel 1 interrupt flag here
}

inline void BusHal::disableInterrupts()
{
    __disable_irq();
}

inline void BusHal::enableInterrupts()
{
    __enable_irq();
}

inline void BusHal::startSending()
{
    _timer.handle.Instance->ARR = 1;
    __HAL_TIM_CLEAR_FLAG(&_timer.handle, TIM_FLAG_UPDATE); // clear UPDATE interrupt flag here
    __HAL_TIM_ENABLE_IT(&_timer.handle, TIM_IT_UPDATE); // enable UPDATE interrupt
    _timer.handle.Instance->CNT = 0; // reset counter
}

#else

// Mask for the timer flag of the capture channel
#define CAPTURE_FLAG (8 << captureChannel)

// Mask for the timer flag of the time channel
#define TIME_FLAG (8 << timeChannel)

/*
 *    original LPC implementation
 */

class BusHal
{
public:
    BusHal(Timer& timer, int rxPin, int txPin, TimerCapture captureChannel, TimerMatch matchChannel);
    void begin();
    void idleState();
    void waitBeforeSending();
    bool isCaptureChannelFlag();
    bool isTimeChannelFlag();
    void setupTimeChannel(unsigned int value);
    unsigned int getCaptureValue();
    void setTimeMatch(unsigned int value);
    void setupCaptureWithInterrupt();
    void setupCaptureWithoutInterrupt();
    void resetFlags();
    void disableInterrupts();
    void enableInterrupts();
    void startSending();
    void setupStartBit(int pwmMatch, int timeMatch);
    unsigned int getTimerValue();
    unsigned int getPwmMatch();
    void setPwmMatch(unsigned int pwmMatch);
protected:
    Timer& timer;                //!< The timer
    int rxPin, txPin;            //!< The pins for bus receiving and sending
    TimerCapture captureChannel; //!< The timer channel that captures the timer value on the bus-in pin
    TimerMatch pwmChannel;       //!< The timer channel for PWM for sending
    TimerMatch timeChannel;      //!< The timer channel for timeouts

};

BusHal::BusHal(Timer& aTimer, int aRxPin, int aTxPin, TimerCapture aCaptureChannel, TimerMatch aPwmChannel)
:timer(aTimer)
,rxPin(aRxPin)
,txPin(aTxPin)
,captureChannel(aCaptureChannel)
,pwmChannel(aPwmChannel)
{
    timeChannel = (TimerMatch) ((pwmChannel + 2) & 3);  // +2 to be compatible to old code during refactoring
}

inline void BusHal::begin()
{
    timer.begin();
    timer.pwmEnable(pwmChannel);
    timer.captureMode(captureChannel, FALLING_EDGE | INTERRUPT);
    timer.start();
    timer.interrupts();
    timer.prescaler(TIMER_PRESCALER);

    timer.match(timeChannel, 0xfffe);
    timer.matchMode(timeChannel, RESET);
    timer.match(pwmChannel, 0xffff);

    // wait until output is driven low before enabling output pin.
    // Using digitalWrite(txPin, 0) does not work with MAT channels.
    timer.value(0xffff); // trigger the next event immediately
    while (timer.getMatchChannelLevel(pwmChannel) == true);
    pinMode(txPin, OUTPUT_MATCH);   // Configure bus output
    pinMode(rxPin, INPUT_CAPTURE | HYSTERESIS);  // Configure bus input
}

inline void BusHal::idleState()
{
    timer.captureMode(captureChannel, FALLING_EDGE | INTERRUPT);

    timer.matchMode(timeChannel, RESET);
    timer.match(timeChannel, 0xfffe);
    timer.match(pwmChannel, 0xffff);
}

inline void BusHal::waitBeforeSending() 
{
    timer.match(timeChannel, sendAck ? SEND_ACK_WAIT_TIME - PRE_SEND_TIME : SEND_WAIT_TIME - PRE_SEND_TIME);
    timer.matchMode(timeChannel, INTERRUPT | RESET);

    timer.captureMode(captureChannel, FALLING_EDGE | INTERRUPT);

}

inline bool BusHal::isCaptureChannelFlag()
{
    return timer.flag(captureChannel);
}

inline bool BusHal::isTimeChannelFlag()
{
    return timer.flag(timeChannel);
}

inline void BusHal::setupTimeChannel(unsigned int value)
{
    timer.match(timeChannel, value);
    timer.restart();
    timer.matchMode(timeChannel, INTERRUPT | RESET);
}

inline unsigned int BusHal::getCaptureValue()
{
    return timer.capture(captureChannel);
}

inline void BusHal::setupStartBit(int pwmMatch, int timeMatch)
{
    timer.match(pwmChannel, pwmMatch);
    timer.match(timeChannel, timeMatch);
    timer.matchMode(timeChannel, RESET | INTERRUPT);
    timer.captureMode(captureChannel, FALLING_EDGE | INTERRUPT);
}

inline unsigned int BusHal::getTimerValue()
{
    return timer.value();
}

inline unsigned int BusHal::getPwmMatch()
{
    return timer.match(pwmChannel);
}

inline void BusHal::setPwmMatch(unsigned int pwmMatch)
{
    timer.match(pwmChannel, pwmMatch);
}

inline void BusHal::setTimeMatch(unsigned int timeMatch)
{
    timer.match(timeChannel, timeMatch);
}

inline void BusHal::setupCaptureWithInterrupt()
{
    timer.captureMode(captureChannel, FALLING_EDGE | INTERRUPT);
}

inline void BusHal::setupCaptureWithoutInterrupt()
{
    timer.captureMode(captureChannel, FALLING_EDGE);
}

inline void BusHal::resetFlags()
{
    timer.resetFlags();
}

inline void BusHal::disableInterrupts()
{
    noInterrupts();
}

inline void BusHal::enableInterrupts()
{
    interrupts();
}

inline void BusHal::startSending()
{
    timer.match(timeChannel, 1);
    timer.matchMode(timeChannel, INTERRUPT | RESET);
    timer.value(0);
}

#endif

#endif /*sblib_bus_hal_h*/
