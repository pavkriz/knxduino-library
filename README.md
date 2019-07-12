# KNXduino library for KNXduino project

* See [hardware and instructions](https://github.com/pavkriz/knxduino)
* Only STM32G0 MCUs are supported. This library will not work with Arduino Uno and other non-STM32 boards!

## Hardwired pin-mappings

* For KNXduino One Board = KNXDUINO_ONE_PINMAPPING
    * PA0 (COMP1_OUT) - connect to PA2 (TIM15_CH1)
    * PA1 (COMP1_INP) - KNX_RX_AN (bus voltage/34)
    * PA2 (TIM15_CH1)
    * PA3 (TIM15_CH2) - KNX_TX
    * PA9 (USART1_TX) - Debug UART (Serial) TX
    * PA10 (USART1_RX) - Debug UART (Serial) RX
* For NUCLEO-G071RB Board = KNXDUINO_NUCLEO_PINMAPPING
    * PA0 (COMP1_OUT) - connect to PC1 (TIM15_CH1)
    * PA1 (COMP1_INP) - KNX_RX_AN (bus voltage/34)
    * PC1 (TIM15_CH1)
    * PC2 (TIM15_CH2) - KNX_TX
    * PA2 (USART2_TX) - Debug UART (Serial) TX
    * PA3 (USART2_RX) - Debug UART (Serial) RX
    

## Acknowledgement

* This project is based on [Selfbus project](http://www.selfbus.org).
* This work is supported by [hkfree.org](http://www.hkfree.org) community network.