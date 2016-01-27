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
#define ESP8266_CONF_H 020

/**
 * @defgroup ESP8266_CONF
 * @brief    Configuration parameters for ESP8266 library
 * @{
 */

/**
 * @brief   USART input buffer size. This buffer stores data received from USART RX interrupt 
 *          before stack use them.
 *
 *          Size of buffer depends on speed of calling @ref ESP8266_Update function, ESP baudrate and
 *          max ESP receive string size.
 *
 * @note    When possible, buffer should be at least 1024 bytes.
 */
#define ESP8266_USARTBUFFER_SIZE                 1024

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
#define ESP8266_TMPBUFFER_SIZE                   512

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
 *
 * @note    Buffer must be at least 256 bytes or there might be unexpected results
 */
#define ESP8266_CONNECTION_BUFFER_SIZE             5842

/**
 * @brief   Enables (1) or disables (0) pinging functionality to other servers
 *
 *          When enabled, additional structure for pinging is added to main structure,
 *          but it is removed if feature is disabled
 */
#define ESP8266_USE_PING                           1

/**
 * @brief   Enables (1) or disables (0) update ESP8288 AT software via network using AT commands.
 *
 *          When using ESP8266 with at least 8Mbit flash memory and proper AT software flashed on it (AT with boot support),
 *          ESP8266 can self update firmware if proper AT command is used.
 *
 *          This macro enables/disables functions for firmware updating
 */
#define ESP8266_USE_FIRMWAREUPDATE                 1

/**
 * @brief   Enables (1) or disables (0) feature to search network for Acces points
 *
 *          You can disable this feature if you know exact network where you will connect to save some memory
 */
#define ESP8266_USE_APSEARCH                       0

/**
 * @brief   Sets how many AP stations stack will parse which are received from ESP8266 module
 *
 *          Set to 10 by default.
 *
 * @note    @ref ESP8266_USE_APSEARCH must be enabled for this feature
 */
#define ESP8266_MAX_DETECTED_AP                    10

/**
 * @brief   Sets number of bytes each connection can use for identification name used by user when creating client connection
 *          using @ref ESP8266_CreateClientConnection.
 */
#define ESP8266_MAX_CONNECTION_NAME                32

/**
 * @}
 */

#endif
