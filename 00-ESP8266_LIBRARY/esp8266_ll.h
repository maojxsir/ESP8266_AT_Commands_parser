/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link    
 * @version v1.0
 * @ide     Keil uVision
 * @license GNU GPL v3
 * @brief   Low level, platform dependant, part for communicate with ESP module and platform.
 *	
\verbatim
   ----------------------------------------------------------------------
    Copyright (C) Tilen Majerle, 2016
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.
     
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
\endverbatim
 */
#ifndef ESP8266_LL_H
#define ESP8266_LL_H 100

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup ESP8266_LL
 * @brief    Low level, platform dependant, part for communicate with ESP module and platform.
 * @{
 *
 * This is a low-level part of ESP module library.
 *
 * It provides communication between ESP module and platform. There are some function, which needs to be implemented by user and with care.
 *
 * \par U(S)ART configuration
 *
 * ESP8266 module works with U(S)ART communication with device. For this purpose, library supposes 2, USART based, functions, which are called from ESP module stack when needed:
 *
 * - \ref ESP8266_LL_USARTInit: Function, which is called when USART should be initialized
 * - \ref ESP8266_LL_USARTSend: Function, which is called when data should be sent to ESP8266 device
 *
 * ESP stack module does not check for any incoming data from ESP8266 module to USART of your device.
 *
 * Most microcontrollers have USART RX capability, so when USART RX interrupt happens,
 * you should send this received byte to ESP8266 module using @ref ESP8266_DataReceived function to notify new incoming data.
 * Use interrupt handler routing to notify new data using previous mentioned function.
 * 
 * \par Reset configuration
 *
 * ESP8266 module can be reset using AT commands. However, it may happen that ESP module ignores AT commands for some reasons.
 *
 * You need to implement 3 macros or functions for GPIO capability for reset purpose.
 *
 * Take a look for \ref ESP8266_RESET_INIT, \ref ESP8266_RESET_LOW and \ref ESP8266_RESET_HIGH functions.
 *
 * \par Time configuration
 *
 * ESP module part needs current time in milliseconds to take care of any possible timeouts on connections and similar things.
 *
 * You need to implement your own time source (systick interrupt, normal timer with interrupt capability, etc) to tell ESP stack current time.
 *
 * Use @ref ESP8266_TimeUpdate to notify ESP stack with new time.
 *
 * \par Changelog
 *
\verbatim
 Version 1.0
  - First release
\endverbatim
 *
 * \par Dependencies
 *
 * Platform based dependencies
 *
\verbatim

\endverbatim
 */

/* Include ESP layer */
#include "esp8266.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/* Include platform dependant libraries */
#include "stm32fxxx_hal.h"
#include "tm_stm32_usart.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_gpio.h"

/* USART and GPIO settings */
#define ESP8266_USART         USART1
#define ESP8266_USART_PP      TM_USART_PinsPack_1
#define ESP8266_RESET_PORT    GPIOA
#define ESP8266_RESET_PIN     GPIO_PIN_0 

/**
 * @brief   Provides delay for amount of milliseconds
 * @param   x: Number of milliseconds for delay
 * @retval  None
 */
#define ESP8266_DELAYMS(x)         Delayms(x)

/**
 * @brief  Initializes U(S)ART peripheral for ESP8266 communication
 * @note   This function is called from ESP stack
 * @param  baudrate: baudrate for USART you have to use when initializing USART peripheral
 * @retval Initialization status:
 *           - 0: Initialization OK
 *           - > 0: Initialization failed
 */
uint8_t ESP8266_LL_USARTInit(uint32_t baudrate);

/**
 * @brief  Sends data from ESP stack to ESP8266 module using USART
 * @param  *data: Pointer to data array which should be sent
 * @param  count: Number of data bytes to sent to module
 * @retval Sent status:
 *           - 0: Sent OK
 *           - > 0: Sent error
 */
uint8_t ESP8266_LL_USARTSend(uint8_t* data, uint16_t count);

/**
 * @brief  Initializes reset pin on platform
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_INIT    TM_GPIO_Init(ESP8266_RESET_PORT, ESP8266_RESET_PIN, TM_GPIO_Mode_OUT, TM_GPIO_OType_PP, TM_GPIO_PuPd_UP, TM_GPIO_Speed_Low)
	
/**
 * @brief  Sets reset pin LOW
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_LOW     TM_GPIO_SetPinLow(ESP8266_RESET_PORT, ESP8266_RESET_PIN)

/**
 * @brief  Sets reset pin HIGH
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_HIGH    TM_GPIO_SetPinHigh(ESP8266_RESET_PORT, ESP8266_RESET_PIN)
#endif /*!< DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
