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

/**************************************************************************/
/**************************************************************************/
/**************************************************************************/
/*                                                                        */
/*    Edit file name to esp8266_ll.h and edit values for your platform    */
/*                                                                        */
/**************************************************************************/
/**************************************************************************/
/**************************************************************************/

/**
 * @defgroup ESP8266_LL
 * @brief    Low level, platform dependant, part for communicate with ESP module and platform.
 * @{
 */

/* Include ESP layer */
#include "esp8266.h"

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
#define ESP8266_RESET_INIT    (void)0
	
/**
 * @brief  Sets reset pin LOW
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_LOW     (void)0

/**
 * @brief  Sets reset pin HIGH
 * @note   Function is called from ESP stack module when needed
 * @note   Declared as macro 
 */
#define ESP8266_RESET_HIGH    (void)0

/**
 * @}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
