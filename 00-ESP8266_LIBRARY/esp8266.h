/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link    
 * @version v0.2
 * @ide     Keil uVision
 * @license GNU GPL v3
 * @brief   Library for ESP8266 module using AT commands for embedded systems
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
#ifndef ESP8266_H
#define ESP8266_H 020

/* C++ detection */
#ifdef __cplusplus
extern "C" {
#endif
	
/**
 * \mainpage
 *
 * ESP8266 AT Commands parser is a generic, platform independent,
 * library for communicating with ESP8266 Wi-Fi module using AT commands.
 * Module is written in ANSI C (C89) and is independent from used platform.
 * It's main targets are embedded system devices like ARM Cortex-M, AVR, PIC and so on.
 *
 * \section sect_features Features
 * 
\verbatim
- Supports official AT commands software from Espressif Systems (currently, version AT 0.52 is supported)
- Supports different platforms (written in ANSI C)
- Supports RAM limited embedded systems
- Event based system with callback functions. Almost none blocking functions except one which are needed
- Supports multiple connections at the same time
- Supports client and/or server mode
- Supports station and softAP mode
- Supports ping to other server
	
- Free to use
\endverbatim
 *
 * \section sect_requirements Requirements
 *
 * To use this library, you must:
 *
\verbatim
	- Have working ESP8266 module (I use ESP01 and ESP07 to test this library) with proper wiring to ESP8266 module with UART and RESET pin
	- Always update ESP8266 module with latest official AT commands software provided by Espressif Systems (Link for more info in Resources section)
	- Have a microcontroller which has U(S)ART capability to communicate with module
\endverbatim
 *
 * \section sect_changelog Changelog
 *
\verbatim
v0.2 (January , 2016)
	- Function ESP8266_RequestSendData has been improved to remove waiting for ESP8266 to answer with "> " before continue 
	- Added ESP8266_USE_PING macro to enable or disable ping feature on ESP8266 module
	- Added ESP8266_USE_FIRMWAREUPDATE macro to enable or disable updating ESP8266 firmware via network from official Espressif systems servers
	- Added ESP8266_USE_APSEARCH macro to enable or disable searching for access points with device
	- Added ESP8266_MAX_DETECTED_AP macro to set maximal number of devices stack will parse and report to user
	- Added ESP8266_MAX_CONNECTION_NAME macro to specify maximal length of connection name when creating new connection as client
	- Improved behaviour when connection buffer is less than packet buffer from ESP8266. In this case, received callback is called multiple time

v0.1 (January 24, 2016)
	- First stable release
\endverbatim
 *
 * \section sect_download Download and resources
 *
 * \par Download library
 *
 * Here are links to download links for library in all versions, current and older releases.
 *
 * If you want to download all examples done for this library, please follow <a href="https://github.com/MaJerle/ESP8266_AT_Commands_parser">Github</a> and download repository.
 *
 * Download latest library version: <a href="/download/esp8266-at-commands-parser-v0-1/">ESP8266 AT commands parser V0.1</a>
 *
 * \par External sources
 *
 * Here is a list of external sources you should keep in mind when using ESP8266 module:
 *   - <a href="http://bbs.espressif.com">ESP8266 official forum</a>
 *   - <a href="http://bbs.espressif.com/viewtopic.php?f=16&t=1613">Flashing ESP8266 to latest ESP AT firmware</a>
 *
 * \section sect_bugs Bugs report
 *
 * In case, you find any bug, please report it to official <a href="https://github.com/MaJerle/ESP8266_AT_Commands_parser">Github</a> website of this project and I will try to fix  it.
 *
 * \section sect_license License
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
 
/**
 * @defgroup ESP8266_API
 * @brief    High level, application part of module
 * @{
 *
 * \par Platform dependant implementation
 *
 * Library itself is platform independent, however, USART and GPIO things must be implemented by user.
 * 
 * Please follow instructions provided in @ref ESP8266_LL page.
 *
 * \par Dependencies
 *
\verbatim
 - string.h
 - stdio.h
 - stdint.h
 - buffer.h
 - esp8266_ll.h
 - esp8266_conf.h
\endverbatim
 */

/* Standard C libraries */
#include "string.h"
#include "stdio.h"
#include "stdint.h"

/* Low level based implementation */
#include "esp8266_ll.h"

/* Include configuration */
#include "esp8266_conf.h"

/* Buffer implementation */
#include "buffer.h"

/* Check values */
#if !defined(ESP8266_CONF_H) || ESP8266_CONF_H != ESP8266_H
#error Wrong configuration file!
#endif

/**
 * @defgroup ESP8266_Macros
 * @brief    Library defines
 * @{
 */

/* This settings should not be modified */
#define ESP8266_MAX_CONNECTIONS        5  /*!< Number of maximum active connections on ESP */
#define ESP8266_MAX_CONNECTEDSTATIONS  10 /*!< Number of AP stations saved to received data array */

 /* Check for GNUC */
#if defined (__GNUC__)
	#ifndef __weak		
		#define __weak   	__attribute__((weak))
	#endif	/* Weak attribute */
#endif

/**
 * @}
 */
 
/**
 * @defgroup ESP8266_Typedefs
 * @brief    Library Typedefs
 * @{
 */

/**
 * @brief  ESP8266 library possible return statements on function calls
 */
typedef enum {
	ESP_OK = 0x00,          /*!< Everything is OK */
	ESP_ERROR,              /*!< An error occurred */
	ESP_DEVICENOTCONNECTED, /*!< Device is not connected to UART */
	ESP_TIMEOUT,            /*!< Timeout was detected when sending command to ESP module */
	ESP_LINKNOTVALID,       /*!< Link for connection is not valid */
	ESP_NOHEAP,             /*!< Heap memory is not available */
	ESP_WIFINOTCONNECTED,   /*!< Wifi is not connected to network */
	ESP_BUSY                /*!< Device is busy, new command is not possible */
} ESP8266_Result_t;

/**
 * @brief  ESP8266 modes of operation enumeration
 */
typedef enum {
	ESP8266_Mode_STA = 0x01,   /*!< ESP in station mode */
	ESP8266_Mode_AP = 0x02,    /*!< ESP as software Access Point mode */
	ESP8266_Mode_STA_AP = 0x03 /*!< ESP in both modes */
} ESP8266_Mode_t;

/**
 * @brief  Security settings for wifi network
 */
typedef enum {
	ESP8266_Ecn_OPEN = 0x00,         /*!< Wifi is open */
	ESP8266_Ecn_WEP = 0x01,          /*!< Wired Equivalent Privacy option for wifi security. @note This mode can't be used when setting up ESP8266 wifi */
	ESP8266_Ecn_WPA_PSK = 0x02,      /*!< Wi-Fi Protected Access */
	ESP8266_Ecn_WPA2_PSK = 0x03,     /*!< Wi-Fi Protected Access 2 */
	ESP8266_Ecn_WPA_WPA2_PSK = 0x04, /*!< Wi-Fi Protected Access with both modes */
} ESP8266_Ecn_t;

/**
 * @brief  Wifi connection error enumeration
 */
typedef enum {
	ESP8266_WifiConnectError_Timeout = 0x01,       /*!< Connection timeout */
	ESP8266_WifiConnectError_WrongPassword = 0x02, /*!< Wrong password for AP */
	ESP8266_WifiConnectError_APNotFound = 0x03,    /*!< AP was not found */
	ESP8266_WifiConnectError_Fail = 0x04           /*!< Connection failed with unknown cause */
} ESP8266_WifiConnectError_t;

/**
 * @brief  Firmware update statuses
 */
typedef enum {
	ESP8266_FirmwareUpdate_ServerFound = 0x01, /*!< Server for update has been found */
	ESP8266_FirmwareUpdate_Connected = 0x02,   /*!< We are connected to server for firmware */
	ESP8266_FirmwareUpdate_GotEdition = 0x03,  /*!< We have firmware edition to download */
	ESP8266_FirmwareUpdate_StartUpdate = 0x04, /*!< Update has started */
} ESP8266_FirmwareUpdate_t;

/**
 * @brief  Sleep mode enumeration
 */
typedef enum {
	ESP8266_SleepMode_Disable = 0x00, /*!< Sleep mode disabled */
	ESP8266_SleepMode_Light = 0x01,   /*!< Light sleep mode */
	ESP8266_SleepMode_Modem = 0x02    /*!< Model sleep mode */
} ESP8266_SleepMode_t;

/**
 * @brief  IPD network data structure
 */
typedef struct {
	uint8_t InIPD;        /*!< Set to 1 when ESP is in IPD mode with data */
	uint16_t InPtr;       /*!< Input pointer to save data to buffer */
	uint16_t PtrTotal;    /*!< Total pointer to count all received data */
	uint8_t ConnNumber;   /*!< Connection number where IPD is active */
	uint8_t USART_Buffer; /*!< Set to 1 when data are read from USART buffer or 0 if from temporary buffer */
} ESP8266_IPD_t;

/**
 * @brief  Connection structure
 */
typedef struct {
	uint8_t Active;              /*!< Status if connection is active */
	uint8_t Number;              /*!< Connection number */
	uint8_t Client;              /*!< Set to 1 if connection was made as client */
	uint16_t RemotePort;         /*!< Remote PORT number */
	uint8_t RemoteIP[4];         /*!< IP address of device */
	uint32_t BytesReceived;      /*!< Number of bytes received in current +IPD data package. U
                                        Use @arg DataSize to detect how many data bytes are in current package when callback function is called for received data */
	uint32_t TotalBytesReceived; /*!< Number of bytes received in entire connection lifecycle */
	uint8_t WaitForWrapper;      /*!< Status flag, to wait for ">" wrapper on data sent */
	uint8_t WaitingSentRespond;  /*!< Set to 1 when we have sent data and we are waiting respond */
#if ESP8266_USE_SINGLE_CONNECTION_BUFFER == 1
	char* Data;                  /*<! Use pointer to data array */
#else
	char Data[ESP8266_CONNECTION_BUFFER_SIZE]; /*!< Data array */
#endif
	uint16_t DataSize;           /*!< Number of bytes in current data package.
                                        Becomes useful, when we have buffer size for data less than ESP8266 IPD statement has data for us.
                                        In this case, big packet from ESP8266 is split into several packages and this argument represent package size */
	uint8_t LastPart;            /*!< When connection buffer is less than ESP8266 max +IPD possible data length,
                                        this parameter can be used if received part of data is last on one +IPD packet.
                                        When data buffer is bigger, this parameter is always set to 1 */
	uint8_t CallDataReceived;    /*!< Set to 1 when we are waiting for commands to be inactive before we call callback function */
	uint32_t ContentLength;      /*!< Value of "Content-Length" header if it exists in +IPD data packet */
	char Name[ESP8266_MAX_CONNECTION_NAME]; /*!< Connection name, useful when using as client */
	void* UserParameters;        /*!< User parameters pointer. Useful when user wants to pass custom data which can later be used in callbacks */
	uint8_t HeadersDone;         /*!< User option flag to set when headers has been found in response */
	uint8_t FirstPacket;         /*!< Set to 1 when if first packet in connection received */
	uint8_t LastActivity;        /*!< Connection last activity time */
} ESP8266_Connection_t;

/**
 * @brief  Connected network structure
 */
typedef struct {
	char SSID[64];   /*!< SSID network name */
	uint8_t MAC[6];  /*!< MAC address of network */
	uint8_t Channel; /*!< Network channel */
	int16_t RSSI;    /*!< Signal strength */
} ESP8266_ConnectedWifi_t;

/**
 * @brief  AP station structure to use when searching for network
 */
typedef struct {
	uint8_t Ecn;         /*!< Security of Wi-Fi spot. This parameter has a value of @ref ESP8266_Ecn_t enumeration */
	char SSID[64];       /*!< Service Set Identifier value. Wi-Fi spot name */
	int16_t RSSI;        /*!< Signal strength of Wi-Fi spot */
	uint8_t MAC[6];      /*!< MAC address of spot */
	uint8_t Channel;     /*!< Wi-Fi channel */
	uint8_t Offset;      /*!< Frequency offset from base 2.4GHz in kHz */
	uint8_t Calibration; /*!< Frequency offset calibration */
} ESP8266_AP_t;

/**
 * @brief  List of AP stations found on network search
 */
typedef struct {
	ESP8266_AP_t AP[ESP8266_MAX_DETECTED_AP]; /*!< Each AP point data */
	uint8_t Count;                            /*!< Number of valid AP stations */
} ESP8266_APs_t;

/**
 * @brief  Structure for connected station to softAP to ESP module
 */
typedef struct {
	uint8_t IP[4];  /*!< IP address of connected station */
	uint8_t MAC[6]; /*!< MAC address of connected station */
} ESP8266_ConnectedStation_t;

/**
 * @brief  List of connected stations to softAP
 */
typedef struct {
	ESP8266_ConnectedStation_t Stations[ESP8266_MAX_CONNECTEDSTATIONS]; /*!< Array of connected stations to AP. Valid number of stations is in @ref Count variable */
	uint8_t Count;                                                      /*!< Number of connected stations to AP */
} ESP8266_ConnectedStations_t;

/**
 * @brief  Access point configuration
 */
typedef struct {
	char SSID[65];          /*!< Network public name for ESP AP mode */
	char Pass[65];          /*!< Network password for ESP AP mode */
	ESP8266_Ecn_t Ecn;      /*!< Security of Wi-Fi spot. This parameter can be a value of @ref ESP8266_Ecn_t enumeration */
	uint8_t Channel;        /*!< Channel Wi-Fi is operating at */
	uint8_t MaxConnections; /*!< Max number of stations that are allowed to connect to ESP AP, between 1 and 4 */
	uint8_t Hidden;         /*!< Set to 1 if network is hidden (not broadcast) or zero if noz */
} ESP8266_APConfig_t;

/**
 * @brief  Ping structure
 */
typedef struct {
	char Address[64]; /*!< Domain or IP to ping */
	uint32_t Time;    /*!< Time in milliseconds needed for pinging */
	uint8_t Success;  /*!< Status indicates if ping was successful */
} ESP8266_Ping_t;

/**
 * @brief  Main ESP8266 working structure
 */
typedef struct {
	uint32_t Baudrate;                                        /*!< Currently used baudrate for ESP module */
	uint32_t ActiveCommand;                                   /*!< Currently active AT command for module */
	char ActiveCommandResponse[5][64];                        /*!< List of responses we expect with AT command */
	uint32_t StartTime;                                       /*!< Time when command was sent */
	uint32_t Time;                                            /*!< Curent time in milliseconds */
	uint32_t LastReceivedTime;                                /*!< Time when last string was received from ESP module */
	uint32_t Timeout;                                         /*!< Timeout in milliseconds for command to return response */
	ESP8266_Connection_t Connection[ESP8266_MAX_CONNECTIONS]; /*!< Array of connections */
	uint8_t STAIP[4];                                         /*!< Assigned IP address for station for ESP module */
	uint8_t STAGateway[4];                                    /*!< Gateway address for station ESP is using */
	uint8_t STANetmask[4];                                    /*!< Netmask address for station ESP is using */
	uint8_t STAMAC[6];                                        /*!< MAC address for station of ESP module */
	uint8_t APIP[4];                                          /*!< Assigned IP address for softAP for ESP module */
	uint8_t APGateway[4];                                     /*!< Gateway address ESP for softAP is using */
	uint8_t APNetmask[4];                                     /*!< Netmask address ESP for softAP is using */
	uint8_t APMAC[6];                                         /*!< MAC address for softAP of ESP module */
	ESP8266_Mode_t SentMode;                                  /*!< AP/STA mode we sent to module. This parameter can be a value of @ref ESP8266_Mode_t enumeration */
	ESP8266_Mode_t Mode;                                      /*!< AT/STA mode which is currently active. This parameter can be a value of @ref ESP8266_Mode_t enumeration */
	ESP8266_APConfig_t AP;                                    /*!< Configuration settings for ESP when using as Access point mode */
	ESP8266_IPD_t IPD;                                        /*!< IPD status strucutre. Used when new data are available from module */
#if ESP8266_USE_PING
	ESP8266_Ping_t PING;                                      /*!< Pinging structure */
#endif
	ESP8266_ConnectedWifi_t ConnectedWifi;                    /*!< Informations about currently connected wifi network */
	ESP8266_WifiConnectError_t WifiConnectError;              /*!< Error code for connection to wifi network. This parameter can be a value of @ref ESP8266_WifiConnectError_t enumeration */
	int8_t StartConnectionSent;                               /*!< Connection number which has active CIPSTART command and waits response */
	ESP8266_ConnectedStations_t ConnectedStations;            /*!< Connected stations to ESP8266 module softAP */
	uint32_t TotalBytesReceived;                              /*!< Total number of bytes ESP8266 module has received from network and sent to our stack */
	uint32_t TotalBytesSent;                                  /*!< Total number of network data bytes we have sent to ESP8266 module for transmission */
	ESP8266_Connection_t* SendDataConnection;                 /*!< Pointer to currently active connection to sent data */
	union {
		struct {
			uint8_t STAIPIsSet:1;                             /*!< IP is set */
			uint8_t STANetmaskIsSet:1;                        /*!< Netmask address is set */
			uint8_t STAGatewayIsSet:1;                        /*!< Gateway address is set */
			uint8_t STAMACIsSet:1;                            /*!< MAC address is set */
			uint8_t APIPIsSet:1;                              /*!< IP is set */
			uint8_t APNetmaskIsSet:1;                         /*!< Netmask address is set */
			uint8_t APGatewayIsSet:1;                         /*!< Gateway address is set */
			uint8_t APMACIsSet:1;                             /*!< MAC address is set */
			uint8_t WaitForWrapper:1;                         /*!< We are waiting for wrapper */
			uint8_t LastOperationStatus:1;                    /*!< Last operations status was OK */
			uint8_t WifiConnected:1;                          /*!< Wifi is connected to network */
			uint8_t WifiGotIP:1;                              /*!< Wifi got IP address from network */
		} F;
		uint32_t Value;
	} Flags;
	ESP8266_Result_t Result;                                  /*!< Result status as returned from last function call. This parameter can be a value of @ref ESP8266_Result_t enumeration */
} ESP8266_t;

/**
 * @}
 */

/**
 * @defgroup ESP8266_Functions
 * @brief    Library Functions
 * @{
 */

/**
 * @brief  Initializes ESP8266 module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  baudrate: USART baudrate for ESP8266 module
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_Init(ESP8266_t* ESP8266, uint32_t baudrate);

/**
 * @brief  Deinitializes ESP8266 module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_DeInit(ESP8266_t* ESP8266);

/**
 * @brief  Waits for ESP8266 to be ready to accept new command
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_WaitReady(ESP8266_t* ESP8266);

/**
 * @brief  Checks if ESP module can accept new AT command
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_IsReady(ESP8266_t* ESP8266);

/**
 * @brief  Update function which does entire work.
 * @note   This function must be called periodically inside main loop to process all events
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_Update(ESP8266_t* ESP8266);

/**
 * @brief  Updates current time
 * @note   This function must be called periodically, best if from interrupt handler, like Systick or other timer based irq
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  time_increase: Number of milliseconds timer will be increased
 * @return None
 */
void ESP8266_TimeUpdate(ESP8266_t* ESP8266, uint32_t time_increase);

/**
 * @brief  Restores default values from ESP8266 module flash memory
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_RestoreDefault(ESP8266_t* ESP8266);

/**
 * @brief  Starts firmware module update over the air (OTA)
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_FirmwareUpdate(ESP8266_t* ESP8266);

/**
 * @brief  Sets baudrate for ESP8266 module
 * @note   Module has some issues on returning OK to this command so I don't recommend UART change
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  baudrate: Baudrate to use with module
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetUART(ESP8266_t* ESP8266, uint32_t baudrate);

/**
 * @brief  Sets baudrate for ESP8266 module and stores it to ESP flash for future use
 * @note   Module has some issues on returning OK to this command so I don't recommend UART change
 *             if you really want to change it, use this function and later reconfigure your program to start with changed UART for ESP USART BAUDRATE
 *             
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  baudrate: Baudrate to use with module
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetUARTDefault(ESP8266_t* ESP8266, uint32_t baudrate);

/**
 * @brief  Sets sleep mode for ESP8266 module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  SleepMode: Sleep mode type. This parameter can be a value of @ref ESP8266_SleepMode_t enumeration
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetSleepMode(ESP8266_t* ESP8266, ESP8266_SleepMode_t SleepMode);

/**
 * @brief  Puts ESP8266 to sleep for specific amount of milliseconds
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  Milliseconds: Number of milliseconds ESP will be in sleep mode
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_Sleep(ESP8266_t* ESP8266, uint32_t Milliseconds);

/**
 * @brief  Connects to wifi network
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *ssid: SSID name to connect to
 * @param  *pass: Password for SSID. Set to "" if there is no password required
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_WifiConnect(ESP8266_t* ESP8266, char* ssid, char* pass);

/**
 * @brief  Connects to wifi network and saves setting to internal flash of ESP for auto connect to network
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *ssid: SSID name to connect to
 * @param  *pass: Password for SSID. Set to "" if there is no password required
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_WifiConnectDefault(ESP8266_t* ESP8266, char* ssid, char* pass);

/**
 * @brief  Gets AP settings of connected network
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure where data about AP will be stored
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_WifiGetConnected(ESP8266_t* ESP8266);

/**
 * @brief  Disconnects from connected AP if any
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_WifiDisconnect(ESP8266_t* ESP8266);

/**
 * @brief  Sets mode for ESP8266, either STA, AP or both
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  Mode: ESP8266 working mode. This parameter can be a value of @ref ESP8266_Mode_t enumeration
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetMode(ESP8266_t* ESP8266, ESP8266_Mode_t Mode);

/**
 * @brief  Sets multiple connections for ESP8266 device.
 * @note   This setting is enabled by default
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  mux: Set to 0 to disable feature or 1 to enable
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetMux(ESP8266_t* ESP8266, uint8_t mux);

/**
 * @brief  Sets data info on network data receive from module
 * @note   This setting is enabled by default to get proper working state
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  dinfo: Set to 1 to enable it, or zero to disable
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_Setdinfo(ESP8266_t* ESP8266, uint8_t dinfo);

/**
 * @brief  Enables server mode on ESP8266 module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  port: Port number ESP will be visible on
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_ServerEnable(ESP8266_t* ESP8266, uint16_t port);

/**
 * @brief  Disables server mode on ESP8266 module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_ServerDisable(ESP8266_t* ESP8266);

/**
 * @brief  Sets server timeout value for connections waiting ESP to respond. This applies for all clients which connects to ESP module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  timeout: Timeout value in unit of seconds
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetServerTimeout(ESP8266_t* ESP8266, uint16_t timeout);

/**
 * @brief  Gets current IP of ESP module connected to other wifi network as station
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetSTAIP(ESP8266_t* ESP8266);

/**
 * @brief  Gets current IP of ESP module connected to other wifi network as station and waits for response from module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetSTAIPBlocking(ESP8266_t* ESP8266);

/**
 * @brief  Gets ESP MAC address when acting like station
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetSTAMAC(ESP8266_t* ESP8266);

/**
 * @brief  Sets ESP MAC address when acting like station
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetSTAMAC(ESP8266_t* ESP8266, uint8_t* addr);

/**
 * @brief  Sets ESP MAC address when acting like station and stores value to ESP flash memory
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetSTAMACDefault(ESP8266_t* ESP8266, uint8_t* addr);

/**
 * @brief  Gets current IP of ESP module acting like softAP
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetAPIP(ESP8266_t* ESP8266);

/**
 * @brief  Gets current IP of ESP module acting like softAP and waits for response from module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetAPIPBlocking(ESP8266_t* ESP8266);

/**
 * @brief  Gets ESP MAC address when acting like softAP
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetAPMAC(ESP8266_t* ESP8266);

/**
 * @brief  Sets ESP MAC address when acting like softAP
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetAPMAC(ESP8266_t* ESP8266, uint8_t* addr);

/**
 * @brief  Sets ESP MAC address when acting like softAP and stores value to flash memory
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetAPMACDefault(ESP8266_t* ESP8266, uint8_t* addr);

/**
 * @brief  Lists for all available AP stations ESP can connect to
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_ListWifiStations(ESP8266_t* ESP8266);

/**
 * @brief  Gets current AP settings of ESP module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetAP(ESP8266_t* ESP8266);

/**
 * @brief  Sets AP config values for ESP module
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *ESP8266_Config: Pointer to @ref ESP8266_APConfig_t structure with settings
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_SetAP(ESP8266_t* ESP8266, ESP8266_APConfig_t* ESP8266_Config);

/**
 * @brief  Starts ping operation to another server
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *addr: Address to ping. Can be either domain name or IP address as string
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_Ping(ESP8266_t* ESP8266, char* addr);

/**
 * @brief  Starts new connection as ESP client and connects to given address and port
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *name: Identification connection name for callback functions to detect proper connection
 * @param  *location: Domain name or IP address to connect to as string
 * @param  port: Port to connect to
 * @param  *user_parameters: Pointer to custom user parameters (if needed) which will later be passed to callback functions for client connection
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_StartClientConnection(ESP8266_t* ESP8266, char* name, char* location, uint16_t port, void* user_parameters);

/**
 * @brief  Closes all opened connections
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_CloseAllConnections(ESP8266_t* ESP8266);

/**
 * @brief  Closes specific previously opened connection
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *Connection: Pointer to @ref ESP8266_Connection_t structure to close it
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_CloseConnection(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  Checks if all connections are closed
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_AllConectionsClosed(ESP8266_t* ESP8266);

/**
 * @brief  Makes a request to send data to specific open connection
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @param  *Connection: Pointer to @ref ESP8266_Connection_t structure to close it
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_RequestSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  Gets a list of connected station devices to softAP on ESP module
 * @note   If function succedded, @ref ESP8266_Callback_ConnectedStationsDetected will be called when data are available
 * @param  *ESP8266: Pointer to working @ref ESP8266_t structure
 * @return Member of @ref ESP8266_Result_t enumeration
 */
ESP8266_Result_t ESP8266_GetConnectedStations(ESP8266_t* ESP8266);
 
/**
 * @brief  Writes data from user defined USART RX interrupt handler to module stack
 * @note   This function should be called from USART RX interrupt handler to write new data
 * @param  *ch: Pointer to data to be written to module buffer
 * @param  count: Number of data bytes to write to module buffer
 * @retval Number of bytes written to buffer
 */
uint16_t ESP8266_DataReceived(uint8_t* ch, uint16_t count);

/**
 * @}
 */

/**
 * @defgroup ESP8266_Callback_Functions
 * @brief    Library callback functions
 *           
 *           Callback functions are called from ESP stack to user which should implement it when needs it.
 * @{
 */
 
/**
 * @brief  Device is ready callback
 *         
 *         Function is called when device has ready string sent to stack
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_DeviceReady(ESP8266_t* ESP8266);
 
/**
 * @brief  Watchdog reset detected on device
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WatchdogReset(ESP8266_t* ESP8266);


 
/**
 * @brief  Device has disconnected from wifi network
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiDisconnected(ESP8266_t* ESP8266);

/**
 * @brief  Device has connected to wifi network
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiConnected(ESP8266_t* ESP8266);

/**
 * @brief  Device did not succeed with connection to wifi network
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiConnectFailed(ESP8266_t* ESP8266);

/**
 * @brief  Device has received IP address as station (when connected to another access point) from connected access point (AP)
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiGotIP(ESP8266_t* ESP8266);

 
/**
 * @brief  Device has received station IP.
 * @note   Function is called in case you try to get IP with \ref ESP8266_GetSTAIP function
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiIPSet(ESP8266_t* ESP8266);

/**
 * @brief  Device failed to retrieve IP address via DHCP
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_DHCPTimeout(ESP8266_t* ESP8266);

/**
 * @brief  Device has detected wifi access point where we can connect to.
 * @note   Function is called when you use \ref ESP8266_ListWifiStations function
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_WifiDetected(ESP8266_t* ESP8266, ESP8266_APs_t* ESP8266_AP);


/**
 * @brief  ESP8266 has a new connection active, acting like server
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ServerConnectionActive(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 connection closed, acting like server
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ServerConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 has a data received on active connection when acting like server
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ServerConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer);

/**
 * @brief  ESP8266 is ready to accept data to be sent when connection is active as server
 * @note   This function is called in case \ref ESP8266_RequestSendData is called by user
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @param  *Buffer: Pointer to buffer where data should be stored
 * @param  max_buffer_size: Buffer size in units of bytes
 * @retval Number of bytes written into buffer
 * @note   With weak parameter to prevent link errors if not defined by user
 */
uint16_t ESP8266_Callback_ServerConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size);

/**
 * @brief  ESP8266 has successfully sent data for active connection
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ServerConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 has not sent data for active connection
 * @note   When this happen, you can use \ref ESP8266_RequestSendData again to request new data sent
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ServerConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  Connection is active when ESP8266 starts new connection using \ref ESP8266_StartClientConnection
 * @note   When this function is called, use \ref ESP8266_RequestSendData if you want to send any data to connection
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionConnected(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP826 returns error when trying to connect to external server as client
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP826 has not return any response in defined amount of time when connection to external server as client
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionTimeout(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  Connection as client has been successfully closed
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 is ready to accept data to be sent when connection is active as client
 * @note   This function is called in case \ref ESP8266_RequestSendData is called by user
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @param  *Buffer: Pointer to buffer where data should be stored
 * @param  max_buffer_size: Buffer size in units of bytes
 * @retval Number of bytes written into buffer
 * @note   With weak parameter to prevent link errors if not defined by user
 */
uint16_t ESP8266_Callback_ClientConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size);

/**
 * @brief  ESP8266 has successfully sent data for active connection as client
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 has not sent data for active connection as client
 * @note   When this happen, you can use \ref ESP8266_RequestSendData again to request new data sent
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection);

/**
 * @brief  ESP8266 received network data and sends it to microcontroller. Function is called when when entire package of data is parsed
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Connection: Pointer to \ref ESP8266_Connection_t connection 
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ClientConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer);

/**
 * @brief  Pinging to external server has started
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *address: Pointer to address string where ping started
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_PingStarted(ESP8266_t* ESP8266, char* address);

/**
 * @brief  Pinging to external server has started
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *PING: Pointer to \ref ESP8266_Ping_t structure with information
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_PingFinished(ESP8266_t* ESP8266, ESP8266_Ping_t* PING);

/**
 * @brief  Firmware update status checking
 * @note   You must use \ref ESP8266_FirmwareUpdate function to start updating
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_FirmwareUpdateStatus(ESP8266_t* ESP8266, ESP8266_FirmwareUpdate_t status);

/**
 * @brief  Firmware update has been successful
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_FirmwareUpdateSuccess(ESP8266_t* ESP8266);

/**
 * @brief  Firmware update has failed
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_FirmwareUpdateError(ESP8266_t* ESP8266);

/**
 * @brief  ESP8266 returns new data about connected stations to our softAP
 * @note   This function is called in case \ref is used for detection connected stations
 * @param  *ESP8266: Pointer to working \ref ESP8266_t structure
 * @param  *Stations: Pointer to @ref ESP8266_ConnectedStations_t structure with data
 * @retval None
 * @note   With weak parameter to prevent link errors if not defined by user
 */
void ESP8266_Callback_ConnectedStationsDetected(ESP8266_t* ESP8266, ESP8266_ConnectedStations_t* Stations);

/**
 * @}
 */
 
/**
 * @}
 */

/* C++ detection */
#ifdef __cplusplus
}
#endif

#endif
