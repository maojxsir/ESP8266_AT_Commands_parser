/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link    
 * @license GNU GPL v3
 * @brief   Configuration for ESP8266 library
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
#ifndef ESP8266_CONF_H
#define ESP8266_CONF_H 010

/**
 * @defgroup ESP8266_CONF
 * @brief    Configuration parameters for ESP8266 library
 * @{
 */

/**
 * @brief   USART input buffer size. This buffer stores data received from USART RX interrupt 
 *          before stack use them
 *
 *          Size of buffer depends on speed of calling @ref ESP8266_Update function, ESP baudrate and
 *          max ESP receive string size.
 *
 * @note    When possible, buffer should be at least 1024 bytes.
 */
#define ESP8266_USARTBUFFER_SIZE       1024

/**
 * @brief   Temporary buffer size. 
 *
 *          When stack sends command, and it waits for "OK" or "ERROR" or something else and if there are data received,
 *          which are not part of this command, then all that data are stored in temporary buffer before they are processed.
 *
 *          Size of buffer depends on speed of calling @ref ESP8266_Update function, ESP baudrate and
 *          max ESP receive string size.
 *
 * @note    When possible, buffer should be at least 512 bytes for safety reasons.
 */
#define ESP8266_TMPBUFFER_SIZE         512

/**
 * @brief   This options allows you to specify if you will use single buffer which will be shared between
 *          all connections together. You can use this option on small embedded systems where you have limited RAM resource.
 *
 *          If option is set to 0, then each connection will have separate data buffer which decreases option for data loss.
 */
#define ESP8266_USE_SINGLE_CONNECTION_BUFFER      1

/**
 * @brief   Buffer size for connection data.
 *
 *          Data packs from ESP module (+IPD statements) have normally up to 1460 bytes on one packet.
 *
 *          However, it may happen, tnat ESP will send you a pack of data with 4x normal size (4 * 1460) bytes.
 *
 *          If you want all that data, you should have at least 4 * 1460 bytes of data available, otherwise,
 *          data will be ignored if buffer will have overflow
 */
#define ESP8266_CONNECTION_BUFFER_SIZE             5842

/**
 * @}
 */

#endif
