/**	
 * |----------------------------------------------------------------------
 * | Copyright (C) Tilen Majerle, 2016
 * | 
 * | This program is free software: you can redistribute it and/or modify
 * | it under the terms of the GNU General Public License as published by
 * | the Free Software Foundation, either version 3 of the License, or
 * | any later version.
 * |  
 * | This program is distributed in the hope that it will be useful,
 * | but WITHOUT ANY WARRANTY; without even the implied warranty of
 * | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * | GNU General Public License for more details.
 * | 
 * | You should have received a copy of the GNU General Public License
 * | along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * |----------------------------------------------------------------------
 */
#include "esp8266.h"

/* Commands list */
#define ESP8266_COMMAND_IDLE           0
#define ESP8266_COMMAND_CWQAP          1
#if ESP8266_USE_APSEARCH
#define ESP8266_COMMAND_CWLAP          2
#endif
#define ESP8266_COMMAND_CWJAP          3
#if ESP8266_USE_FIRMWAREUPDATE
#define ESP8266_COMMAND_CIUPDATE       4
#endif
#define ESP8266_COMMAND_CWMODE         5
#define ESP8266_COMMAND_CIPSERVER      6
#define ESP8266_COMMAND_CIPDINFO       7
#define ESP8266_COMMAND_SEND           8
#define ESP8266_COMMAND_CLOSE          9
#define ESP8266_COMMAND_CIPSTART       10
#define ESP8266_COMMAND_CIPMUX         11
#define ESP8266_COMMAND_CWSAP          12
#define ESP8266_COMMAND_ATE            13
#define ESP8266_COMMAND_AT             14
#define ESP8266_COMMAND_RST            15
#define ESP8266_COMMAND_RESTORE        16
#define ESP8266_COMMAND_UART           17
#define ESP8266_COMMAND_CWJAP_GET      19
#define ESP8266_COMMAND_SLEEP          20
#define ESP8266_COMMAND_GSLP           21
#define ESP8266_COMMAND_CIPSTA         22
#define ESP8266_COMMAND_CIPAP          23
#define ESP8266_COMMAND_CIPSTAMAC      24
#define ESP8266_COMMAND_CIPAPMAC       25
#define ESP8266_COMMAND_CIPSTO         26
#define ESP8266_COMMAND_CWLIF          27
#define ESP8266_COMMAND_CIPSTATUS      28
#define ESP8266_COMMAND_SENDDATA       29

#if ESP8266_USE_PING
#define ESP8266_COMMAND_PING           18
#endif

#define ESP8266_DEFAULT_BAUDRATE       115200 /*!< Default ESP8266 baudrate */
#define ESP8266_TIMEOUT                30000  /*!< Timeout value is milliseconds */

/* Debug */
#define ESP8266_DEBUG(x)               printf("%s", x)

/* Maximum number of return data size in one +IPD from ESP8266 module */
#define ESP8255_MAX_BUFF_SIZE          5842

/* Temporary buffer */
static BUFFER_t TMP_Buffer;
static BUFFER_t USART_Buffer;
static uint8_t TMPBuffer[ESP8266_TMPBUFFER_SIZE];
static uint8_t USARTBuffer[ESP8266_USARTBUFFER_SIZE];

#if ESP8266_USE_APSEARCH
/* AP list */
static ESP8266_APs_t ESP8266_APs;
#endif

/* Create data array for connections */
#if ESP8266_USE_SINGLE_CONNECTION_BUFFER == 1
static char ConnectionData[ESP8266_CONNECTION_BUFFER_SIZE]; /*!< Data array */
#endif

/* Private functions */
#if ESP8266_USE_APSEARCH
static void ParseCWLAP(ESP8266_t* ESP8266, char* Buffer);
#endif
static void ParseCIPSTA(ESP8266_t* ESP8266, char* Buffer);
static void ParseCWSAP(ESP8266_t* ESP8266, char* Buffer);
static void ParseCWJAP(ESP8266_t* ESP8266, char* Buffer);
static void ParseCWLIF(ESP8266_t* ESP8266, char* Buffer);
static void ParseIP(char* ip_str, uint8_t* arr, uint8_t* cnt);
static void ParseMAC(char* ptr, uint8_t* arr, uint8_t* cnt);
static void ParseReceived(ESP8266_t* ESP8266, char* Received, uint8_t from_usart_buffer, uint16_t bufflen);
void CopyCharacterUSART2TMP(ESP8266_t* ESP8266);
static char* EscapeString(char* str);
char* ReverseEscapeString(char* str);
static ESP8266_Result_t SendCommand(ESP8266_t* ESP8266, uint8_t Command, char* CommandStr, char* StartRespond);
static ESP8266_Result_t SendUARTCommand(ESP8266_t* ESP8266, uint32_t baudrate, char* cmd);
static ESP8266_Result_t SendMACCommand(ESP8266_t* ESP8266, uint8_t* addr, char* cmd, uint8_t command);
static void CallConnectionCallbacks(ESP8266_t* ESP8266);
static void ProcessSendData(ESP8266_t* ESP8266);
void* mem_mem(void* haystack, size_t haystacksize, void* needle, size_t needlesize);

#define CHARISNUM(x)    ((x) >= '0' && (x) <= '9')
static uint8_t CHARISHEXNUM(char a);
#define CHAR2NUM(x)     ((x) - '0')
static uint8_t Hex2Num(char a);
static int32_t ParseNumber(char* ptr, uint8_t* cnt);
static uint32_t ParseHexNumber(char* ptr, uint8_t* cnt);

/* List of possible ESP8266 baudrates */
static uint32_t ESP8266_Baudrate[] = {
	9600, 57600, 115200, 921600
};

/* Check IDLE */
#define ESP8266_CHECK_IDLE(ESP8266)                         \
do {                                                        \
	if (                                                    \
		(ESP8266)->ActiveCommand != ESP8266_COMMAND_IDLE    \
	) {                                                     \
		ESP8266_Update(ESP8266);                            \
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_BUSY);        \
	}                                                       \
} while (0);

/* Check if device is connected to WIFI */
#define ESP8266_CHECK_WIFICONNECTED(ESP8266)                \
do {                                                        \
	if (                                                    \
		!(ESP8266)->Flags.F.WifiConnected                   \
	) {                                                     \
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_WIFINOTCONNECTED); \
	}                                                       \
} while (0);

/* Return from function with desired status */
#define ESP8266_RETURNWITHSTATUS(ESP8266, status)           \
do {                                                        \
	(ESP8266)->Result = status;                             \
	return status;                                          \
} while (0); 

/* Reset ESP connection */
#define ESP8266_RESETCONNECTION(ESP8266, conn)              \
do {                                                        \
	(conn)->Active = 0;                                     \
	(conn)->Client = 0;                                     \
	(conn)->FirstPacket = 0;                                \
	(conn)->HeadersDone = 0;                                \
} while (0);                                                \

/* Reset all connections */
#define ESP8266_RESET_CONNECTIONS(ESP8266)                  memset(ESP8266->Connection, 0, sizeof(ESP8266->Connection));

/******************************************/
/*          Basic AT commands Set         */
/******************************************/
ESP8266_Result_t ESP8266_Init(ESP8266_t* ESP8266, uint32_t baudrate) {
	uint8_t i;
	
	/* Save settings */
	ESP8266->Timeout = 0;
	
	/* Init temporary buffer */
	if (BUFFER_Init(&TMP_Buffer, ESP8266_TMPBUFFER_SIZE, TMPBuffer)) {
		/* Return from function */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_NOHEAP);
	}
	
	/* Init USART working */
	if (BUFFER_Init(&USART_Buffer, ESP8266_USARTBUFFER_SIZE, USARTBuffer)) {
		/* Return from function */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_NOHEAP);
	}

	/* Init RESET pin */
	ESP8266_RESET_INIT;
	
	/* Set pin low */
	ESP8266_RESET_LOW;
	
	/* Delay for while */
	ESP8266_DELAYMS(100);
	
	/* Set pin high */
	ESP8266_RESET_HIGH;
	
	/* Delay for while */
	ESP8266_DELAYMS(100);
	
	/* Save current baudrate */
	ESP8266->Baudrate = baudrate;
	
	/* Init USART */
	ESP8266_LL_USARTInit(ESP8266->Baudrate);
	
	/* Set allowed timeout */
	ESP8266->Timeout = 1000;
	
	/* Reset device */
	SendCommand(ESP8266, ESP8266_COMMAND_RST, "AT+RST\r\n", "ready\r\n");
	
	/* Wait till idle */
	ESP8266_WaitReady(ESP8266);

	/* Check status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Check for baudrate, try with predefined baudrates */
		for (i = 0; i < sizeof(ESP8266_Baudrate) / sizeof(ESP8266_Baudrate[0]); i++) {
			/* Init USART */
			ESP8266_LL_USARTInit(ESP8266->Baudrate);
			
			/* Set allowed timeout */
			ESP8266->Timeout = 1000;
			
			/* Reset device */
			SendCommand(ESP8266, ESP8266_COMMAND_RST, "AT+RST\r\n", "ready\r\n");
			
			/* Wait till idle */
			ESP8266_WaitReady(ESP8266);
		
			/* Check status */
			if (ESP8266->Flags.F.LastOperationStatus) {
				/* Save current baudrate */
				ESP8266->Baudrate = ESP8266_Baudrate[i];
				
				break;
			}
		}
	}
	
	/* Check status */
	if (!ESP8266->Flags.F.LastOperationStatus) {		
		/* Device is not connected */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_DEVICENOTCONNECTED);
	}
	
	/* Set allowed timeout to 30sec */
	ESP8266->Timeout = ESP8266_TIMEOUT;
	
	/* Test device */
	SendCommand(ESP8266, ESP8266_COMMAND_AT, "AT\r\n", "OK\r\n");
	
	/* Wait till idle */
	ESP8266_WaitReady(ESP8266);
	
	/* Check status */
	if (!ESP8266->Flags.F.LastOperationStatus) {		
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_DEVICENOTCONNECTED);
	}
	
	/* Enable echo if not already */
	SendCommand(ESP8266, ESP8266_COMMAND_ATE, "ATE1\r\n", "ATE1");
	
	/* Wait till idle */
	ESP8266_WaitReady(ESP8266);
	
	/* Enable multiple connections */
	while (ESP8266_SetMux(ESP8266, 1) != ESP_OK);
	
	/* Enable IP and PORT to be shown on +IPD statement */
	while (ESP8266_Setdinfo(ESP8266, 1) != ESP_OK);
	
	/* Get station MAC */
	while (ESP8266_GetSTAMAC(ESP8266) != ESP_OK);
	
	/* Get softAP MAC */
	while (ESP8266_GetAPMAC(ESP8266) != ESP_OK);
	
	/* Get softAP MAC */
	while (ESP8266_GetAPIP(ESP8266) != ESP_OK);
	
	/* Return OK */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_DeInit(ESP8266_t* ESP8266) {
	/* Clear temporary buffer */
	BUFFER_Free(&TMP_Buffer);
	
	/* Return OK from function */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_RestoreDefault(ESP8266_t* ESP8266) {
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_RESTORE, "AT+RESTORE\r\n", "ready\r\n") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Little delay */
	ESP8266_DELAYMS(2);
	
	/* Reset USART to default ESP baudrate */
	ESP8266_LL_USARTInit(ESP8266_DEFAULT_BAUDRATE);
	
	/* Wait till ready, ESP will send data in default baudrate after reset */
	ESP8266_WaitReady(ESP8266);
	
	/* Reset USART buffer */
	BUFFER_Reset(&USART_Buffer);
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

#if ESP8266_USE_FIRMWAREUPDATE
ESP8266_Result_t ESP8266_FirmwareUpdate(ESP8266_t* ESP8266) {
	/* Check connected */
	ESP8266_CHECK_WIFICONNECTED(ESP8266);
	
	/* Send command if possible */
	return SendCommand(ESP8266, ESP8266_COMMAND_CIUPDATE, "AT+CIUPDATE\r\n", "+CIPUPDATE:");
}
#endif

ESP8266_Result_t ESP8266_SetUART(ESP8266_t* ESP8266, uint32_t baudrate) {
	/* Set current baudrate */
	return SendUARTCommand(ESP8266, baudrate, "AT+UART_CUR");
}

ESP8266_Result_t ESP8266_SetUARTDefault(ESP8266_t* ESP8266, uint32_t baudrate) {
	/* Set default baudrate */
	return SendUARTCommand(ESP8266, baudrate, "AT+UART_DEF");
}

ESP8266_Result_t ESP8266_SetSleepMode(ESP8266_t* ESP8266, ESP8266_SleepMode_t SleepMode) {
	char tmp[20];
	
	/* Check idle mode */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Format command */
	sprintf(tmp, "AT+SLEEP=%d\r\n", SleepMode);
	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_SLEEP, tmp, "+SLEEP");
	
	/* Wait ready */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_Sleep(ESP8266_t* ESP8266, uint32_t Milliseconds) {
	char tmp[20];
	
	/* Check idle mode */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Format command */
	sprintf(tmp, "AT+GSLP=%u\r\n", Milliseconds);
	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_GSLP, tmp, NULL);
	
	/* Wait ready */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_Update(ESP8266_t* ESP8266) {
	char Received[256];
	char ch;
	uint8_t lastcmd;
	uint16_t stringlength;
	
	/* If timeout is set to 0 */
	if (ESP8266->Timeout == 0) {
		ESP8266->Timeout = 30000;
	}
	
	/* Check timeout */
	if ((ESP8266->Time - ESP8266->StartTime) > ESP8266->Timeout) {
		/* Save temporary active command */
		lastcmd = ESP8266->ActiveCommand;
		
		/* Timeout reached, reset command */
		ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
		
		/* Timeout reached */
		if (lastcmd == ESP8266_COMMAND_CIPSTART) {
			/* We get timeout on cipstart */
			ESP8266_RESETCONNECTION(ESP8266, &ESP8266->Connection[ESP8266->StartConnectionSent]);
			
			/* Call user function */
			ESP8266_Callback_ClientConnectionTimeout(ESP8266, &ESP8266->Connection[ESP8266->StartConnectionSent]);
		}
	}
	
	/* We are waiting to send data */
	if (ESP8266_COMMAND_SENDDATA) {
		/* Check what we are searching for */
		if (ESP8266->Flags.F.WaitForWrapper) {
			int16_t found;
			uint8_t dummy[2];
			/* Wait for character */
			if ((found = BUFFER_Find(&USART_Buffer, (uint8_t *)"> ", 2)) >= 0) {
				if (found == 0) {
					/* Make a dummy read */
					BUFFER_Read(&USART_Buffer, dummy, 2);
				}
			} else if ((found = BUFFER_Find(&TMP_Buffer, (uint8_t *)"> ", 0)) >= 0) {
				if (found == 0) {
					/* Make 2 dummy reads */
					BUFFER_Read(&TMP_Buffer, dummy, 2);
				}
			}
			if (found >= 0) {
				/* Send data */
				ProcessSendData(ESP8266);
			}
		}
	}
	
	/* If AT+UART command was used, only check if "OK" exists in buffer */
	if (
		ESP8266->ActiveCommand == ESP8266_COMMAND_UART && /*!< Active command is UART change */
		!ESP8266->IPD.InIPD                               /*!< We are not in IPD mode */
	) {
		/* Check for "OK\r" */
		if (BUFFER_Find(&USART_Buffer, (uint8_t *)"OK\r", 3) >= 0) {
			/* Clear buffer */
			BUFFER_Reset(&USART_Buffer);
			
			/* We are OK here */
			ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			
			/* Last command is OK */
			ESP8266->Flags.F.LastOperationStatus = 1;
			
			/* Return from function */
			//return ESP_OK;
		}
	}
	
	/* Get string from USART buffer if we are not in IPD mode */
	while (
		!ESP8266->IPD.InIPD &&                                                             /*!< Not in IPD mode */
		//!ESP8266->Flags.F.WaitForWrapper &&
		(stringlength = BUFFER_ReadString(&USART_Buffer, Received, sizeof(Received))) > 0 /*!< Something in USART buffer */
	) {		
		/* Parse received string */
		ParseReceived(ESP8266, Received, 1, stringlength);
	}
	
	/* Get string from TMP buffer when no command active */
	while (
		!ESP8266->IPD.InIPD &&                                                             /*!< Not in IPD mode */
		//!ESP8266->Flags.F.WaitForWrapper &&
		ESP8266->ActiveCommand == ESP8266_COMMAND_IDLE &&                                  /*!< We are in IDLE mode */
		(stringlength = BUFFER_ReadString(&TMP_Buffer, Received, sizeof(Received))) > 0 /*!< Something in TMP buffer */
	) {
		/* Parse received string */
		ParseReceived(ESP8266, Received, 0, stringlength);
	}
	
	/* If we are in IPD mode */
	if (ESP8266->IPD.InIPD) {
		BUFFER_t* buff;
		/* Check for USART buffer */
		if (ESP8266->IPD.USART_Buffer) {
			buff = &USART_Buffer;
		} else {
			buff = &TMP_Buffer;
		}
		
		/* If anything received */
		while (
			ESP8266->IPD.InPtr < ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived && /*!< Still not everything received */
			BUFFER_GetFull(buff) > 0                                                    /*!< Data are available in buffer */
		) {
			/* Read from buffer */
			BUFFER_Read(buff, (uint8_t *)&ch, 1);
			
			/* Add from USART buffer */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].Data[ESP8266->IPD.InPtr] = ch;
			
			/* Increase pointers */
			ESP8266->IPD.InPtr++;
			ESP8266->IPD.PtrTotal++;
			
#if ESP8266_CONNECTION_BUFFER_SIZE < ESP8255_MAX_BUFF_SIZE
			/* Check for pointer */
			if (ESP8266->IPD.InPtr >= ESP8266_CONNECTION_BUFFER_SIZE && ESP8266->IPD.InPtr != ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived) {
				/* Set connection buffer size */
				ESP8266->Connection[ESP8266->IPD.ConnNumber].DataSize = ESP8266->IPD.InPtr;
				ESP8266->Connection[ESP8266->IPD.ConnNumber].LastPart = 0;
				
				/* Buffer is full, call user function */
				if (ESP8266->Connection[ESP8266->IPD.ConnNumber].Client) {
					ESP8266_Callback_ClientConnectionDataReceived(ESP8266, &ESP8266->Connection[ESP8266->IPD.ConnNumber], ESP8266->Connection[ESP8266->IPD.ConnNumber].Data);
				} else {
					ESP8266_Callback_ServerConnectionDataReceived(ESP8266, &ESP8266->Connection[ESP8266->IPD.ConnNumber], ESP8266->Connection[ESP8266->IPD.ConnNumber].Data);
				}
				
				/* Reset input pointer */
				ESP8266->IPD.InPtr = 0;
			}
#endif
		}
		
		/* Check if everything received */
		if (ESP8266->IPD.PtrTotal >= ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived) {
			char* ptr;
			
			/* Not in IPD anymore */
			ESP8266->IPD.InIPD = 0;
			
			/* Set package data size */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].DataSize = ESP8266->IPD.InPtr;
			ESP8266->Connection[ESP8266->IPD.ConnNumber].LastPart = 1;
			
			/* We have data, lets see if Content-Length exists and save it */
			if (
				ESP8266->Connection[ESP8266->IPD.ConnNumber].FirstPacket &&
				(ptr = strstr(ESP8266->Connection[ESP8266->IPD.ConnNumber].Data, "Content-Length: ")) != NULL
			) {
				/* Increase pointer and parse number */
				ptr += 16;
				
				/* Parse content length */
				ESP8266->Connection[ESP8266->IPD.ConnNumber].ContentLength = ParseNumber(ptr, NULL);
			}
			
			/* Set flag to trigger callback for data received */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].CallDataReceived = 1;
		}
	}
	
	/* Call user functions on connections if needed */
	CallConnectionCallbacks(ESP8266);
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

void ESP8266_TimeUpdate(ESP8266_t* ESP8266, uint32_t time_increase) {
	/* Increase time */
	ESP8266->Time += time_increase;
}

ESP8266_Result_t ESP8266_WaitReady(ESP8266_t* ESP8266) {
	/* Do job */
	do {
		/* Check for wrapper */
		if (ESP8266->Flags.F.WaitForWrapper) {
			/* We have found it, stop execution here */
			if (BUFFER_Find(&USART_Buffer, (uint8_t *)"> ", 2) >= 0) {
				ESP8266->Flags.F.WaitForWrapper = 0;
				break;
			}
		}
		/* Update device */
		ESP8266_Update(ESP8266);
	} while (ESP8266->ActiveCommand != ESP8266_COMMAND_IDLE);
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_IsReady(ESP8266_t* ESP8266) {
	/* Check IDLE */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_SetMode(ESP8266_t* ESP8266, ESP8266_Mode_t Mode) {
	char command[30];
	
	/* Format command */
	sprintf(command, "AT+CWMODE_CUR=%d\r\n", (uint8_t)Mode);
	
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CWMODE, command, "AT+CWMODE") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Save mode we sent */
	ESP8266->SentMode = Mode;

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);
	
	/* Check status */
	if (ESP8266->Mode != Mode) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Now get settings for current AP mode */
	return ESP8266_GetAP(ESP8266);
}

ESP8266_Result_t ESP8266_RequestSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	char command[30];
	
	/* Check idle state */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Format command */
	sprintf(command, "AT+CIPSENDEX=%d,2048\r\n", Connection->Number);
	
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_SEND, command, "AT+CIPSENDEX") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* We are waiting for "> " response */
	Connection->WaitForWrapper = 1;
	ESP8266->Flags.F.WaitForWrapper = 1;
	
	/* Save connection pointer */
	ESP8266->SendDataConnection = Connection;
	
	/* Return from function */
	return ESP8266->Result;
}

ESP8266_Result_t ESP8266_CloseConnection(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	char tmp[30];
	
	/* Format connection */
	sprintf(tmp, "AT+CIPCLOSE=%d\r\n", Connection->Number);
	
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CLOSE, tmp, "AT+CIPCLOSE");
}

ESP8266_Result_t ESP8266_CloseAllConnections(ESP8266_t* ESP8266) {
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CLOSE, "AT+CIPCLOSE=5\r\n", "AT+CIPCLOSE");
}

ESP8266_Result_t ESP8266_AllConectionsClosed(ESP8266_t* ESP8266) {
	uint8_t i;
	
	/* Go through all connections */
	for (i = 0; i < ESP8266_MAX_CONNECTIONS; i++) {
		/* Check if active */
		if (ESP8266->Connection[i].Active) {
			ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
		}
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_SetMux(ESP8266_t* ESP8266, uint8_t mux) {
	char tmp[30];
	
	/* Format command */
	sprintf(tmp, "AT+CIPMUX=%d\r\n", mux);
	
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CIPMUX, tmp, "AT+CIPMUX") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);
	
	/* Check last status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_Setdinfo(ESP8266_t* ESP8266, uint8_t info) {
	char tmp[30];
	
	/* Format string */
	sprintf(tmp, "AT+CIPDINFO=%d\r\n", info);

	/* Send command and wait */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CIPDINFO, tmp, "AT+CIPDINFO") != ESP_OK) {
		return ESP8266->Result;
	}

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);
	
	/* Check last status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_ServerEnable(ESP8266_t* ESP8266, uint16_t port) {
	char tmp[30];
	
	/* Format string */
	sprintf(tmp, "AT+CIPSERVER=1,%d\r\n", port);

	/* Send command and wait */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CIPSERVER, tmp, "AT+CIPSERVER") != ESP_OK) {
		return ESP8266->Result;
	}

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);

	/* Check last status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_ServerDisable(ESP8266_t* ESP8266) {
	/* Send command and wait */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CIPSERVER, "AT+CIPSERVER=0\r\n", "AT+CIPSERVER") != ESP_OK) {
		return ESP8266->Result;
	}

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);

	/* Check last status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_SetServerTimeout(ESP8266_t* ESP8266, uint16_t timeout) {
	char tmp[20];
	
	/* Format string */
	sprintf(tmp, "AT+CIPSTO=%d\r\n", timeout);

	/* Send command and wait */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CIPSTO, tmp, NULL) != ESP_OK) {
		return ESP8266->Result;
	}

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);

	/* Check last status */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		/* Return error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_WifiDisconnect(ESP8266_t* ESP8266) {
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CWQAP, "AT+CWQAP\r\n", "AT+CWQAP");
}

ESP8266_Result_t ESP8266_WifiConnect(ESP8266_t* ESP8266, char* ssid, char* pass) {
	char tmp[100];
	char s[50], p[50];
	
	/* Escape special characters for ESP8266 */
	strcpy(s, EscapeString(ssid));
	strcpy(p, EscapeString(pass));
	
	/* Format command */
	sprintf(tmp, "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n", s, p);
	
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CWJAP, tmp, "+CWJAP:");
}

ESP8266_Result_t ESP8266_WifiConnectDefault(ESP8266_t* ESP8266, char* ssid, char* pass) {
	char tmp[100];
	char s[50], p[50];
	
	/* Escape special characters for ESP8266 */
	strcpy(s, EscapeString(ssid));
	strcpy(p, EscapeString(pass));
	
	/* Format command */
	sprintf(tmp, "AT+CWJAP_DEF=\"%s\",\"%s\"\r\n", s, p);
	
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CWJAP, tmp, "+CWJAP:");
}

ESP8266_Result_t ESP8266_WifiGetConnected(ESP8266_t* ESP8266) {	
	/* Send command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CWJAP_GET, "AT+CWJAP_CUR?\r\n", "+CWJAP_CUR:");
}

ESP8266_Result_t ESP8266_GetSTAIPBlocking(ESP8266_t* ESP8266) {
	/* Send command */
	if (ESP8266_GetSTAIP(ESP8266) != ESP_OK) {
	
	}

	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

ESP8266_Result_t ESP8266_GetAPIPBlocking(ESP8266_t* ESP8266) {
	/* Send command */
	if (ESP8266_GetAPIP(ESP8266) != ESP_OK) {
	
	}

	/* Wait till command end */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_GetSTAIP(ESP8266_t* ESP8266) {	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CIPSTA, "AT+CIPSTA_CUR?\r\n", "+CIPSTA_CUR");
	
	/* Check status */
	if (ESP8266->Result == ESP_OK) {
		/* Reset flags */
		ESP8266->Flags.F.STAIPIsSet = 0;
		ESP8266->Flags.F.STANetmaskIsSet = 0;
		ESP8266->Flags.F.STAGatewayIsSet = 0;
	}
	
	/* Return status */
	return ESP8266->Result;
}

ESP8266_Result_t ESP8266_GetAPIP(ESP8266_t* ESP8266) {	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CIPAP, "AT+CIPAP_CUR?\r\n", "+CIPAP_CUR");
	
	/* Check status */
	if (ESP8266->Result == ESP_OK) {
		/* Reset flags */
		ESP8266->Flags.F.APIPIsSet = 0;
		ESP8266->Flags.F.APNetmaskIsSet = 0;
		ESP8266->Flags.F.APGatewayIsSet = 0;
	}
	
	/* Return status */
	return ESP8266->Result;
}

/******************************************/
/*            MAC MANUPULATION            */
/******************************************/
ESP8266_Result_t ESP8266_GetSTAMAC(ESP8266_t* ESP8266) {	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CIPSTAMAC, "AT+CIPSTAMAC?\r\n", "+CIPSTAMAC");
	
	/* Check status */
	if (ESP8266->Result == ESP_OK) {
		/* Reset flags */
		ESP8266->Flags.F.STAMACIsSet = 0;
	}
	
	/* Return stats */
	return ESP8266->Result;
}

ESP8266_Result_t ESP8266_SetSTAMAC(ESP8266_t* ESP8266, uint8_t* addr) {
	/* Send current MAC command */
	return SendMACCommand(ESP8266, addr, "AT+CIPSTAMAC_CUR", ESP8266_COMMAND_CIPSTAMAC);
}

ESP8266_Result_t ESP8266_SetSTAMACDefault(ESP8266_t* ESP8266, uint8_t* addr) {
	/* Send current MAC command */
	return SendMACCommand(ESP8266, addr, "AT+CIPSTAMAC_DEF", ESP8266_COMMAND_CIPSTAMAC);
}

ESP8266_Result_t ESP8266_GetAPMAC(ESP8266_t* ESP8266) {	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CIPAPMAC, "AT+CIPAPMAC?\r\n", "+CIPAPMAC");
	
	/* Check status */
	if (ESP8266->Result == ESP_OK) {
		/* Reset flags */
		ESP8266->Flags.F.APMACIsSet = 0;
	}
	
	/* Return stats */
	return ESP8266->Result;
}

ESP8266_Result_t ESP8266_SetAPMAC(ESP8266_t* ESP8266, uint8_t* addr) {
	/* Send current MAC command */
	return SendMACCommand(ESP8266, addr, "AT+CIPAPMAC_CUR", ESP8266_COMMAND_CIPAPMAC);
}

ESP8266_Result_t ESP8266_SetAPMACDefault(ESP8266_t* ESP8266, uint8_t* addr) {
	/* Send current MAC command */
	return SendMACCommand(ESP8266, addr, "AT+CIPAPMAC_DEF", ESP8266_COMMAND_CIPAPMAC);
}

/******************************************/
/*                AP + STA                */
/******************************************/
#if ESP8266_USE_APSEARCH
ESP8266_Result_t ESP8266_ListWifiStations(ESP8266_t* ESP8266) {
	/* Reset pointer */
	ESP8266_APs.Count = 0;
	
	/* Send list command */
	return SendCommand(ESP8266, ESP8266_COMMAND_CWLAP, "AT+CWLAP\r\n", "+CWLAP");	
}
#endif

ESP8266_Result_t ESP8266_GetAP(ESP8266_t* ESP8266) {
	/* Send command to read current AP settings */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CWSAP, "AT+CWSAP?\r\n", "+CWSAP") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Wait till command end */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_SetAP(ESP8266_t* ESP8266, ESP8266_APConfig_t* ESP8266_Config) {
	char tmp[120];
	char pass[65], ssid[65];
	
	/* Check input values */
	if (
		strlen(ESP8266_Config->SSID) > 64 ||
		strlen(ESP8266_Config->Pass) > 64 ||
		(ESP8266_Config->Ecn != ESP8266_Ecn_OPEN && strlen(ESP8266_Config->Pass) < 8) ||
		ESP8266_Config->Ecn == ESP8266_Ecn_WEP ||
		ESP8266_Config->MaxConnections < 1 || ESP8266_Config->MaxConnections > 4
	) {
		/* Error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Escape characters */
	strcpy(ssid, EscapeString(ESP8266_Config->SSID));
	strcpy(pass, EscapeString(ESP8266_Config->Pass));
	
	/* Format command */
	sprintf(tmp, "AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d\r\n",
		ssid, pass,
		ESP8266_Config->Channel,
		(uint8_t)ESP8266_Config->Ecn,
		ESP8266_Config->MaxConnections,
		ESP8266_Config->Hidden
	);
	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CWSAP, tmp, "AT+CWSAP");
	
	/* Return status */
	return ESP8266_Update(ESP8266);
}

ESP8266_Result_t ESP8266_SetAPDefault(ESP8266_t* ESP8266, ESP8266_APConfig_t* ESP8266_Config) {
	char tmp[120];
	char pass[65], ssid[65];
	
	/* Check input values */
	if (
		strlen(ESP8266_Config->SSID) > 64 ||
		strlen(ESP8266_Config->Pass) > 64 ||
		(ESP8266_Config->Ecn != ESP8266_Ecn_OPEN && strlen(ESP8266_Config->Pass) < 8) ||
		ESP8266_Config->Ecn == ESP8266_Ecn_WEP ||
		ESP8266_Config->MaxConnections < 1 || ESP8266_Config->MaxConnections > 4
	) {
		/* Error */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Escape characters */
	strcpy(ssid, EscapeString(ESP8266_Config->SSID));
	strcpy(pass, EscapeString(ESP8266_Config->Pass));
	
	/* Format command */
	sprintf(tmp, "AT+CWSAP_DEF=\"%s\",\"%s\",%d,%d,%d,%d\r\n",
		ssid, pass,
		ESP8266_Config->Channel,
		(uint8_t)ESP8266_Config->Ecn,
		ESP8266_Config->MaxConnections,
		ESP8266_Config->Hidden
	);
	
	/* Send command */
	SendCommand(ESP8266, ESP8266_COMMAND_CWSAP, tmp, "AT+CWSAP");
	
	/* Wait till command end */
	return ESP8266_WaitReady(ESP8266);
}

ESP8266_Result_t ESP8266_GetConnectedStations(ESP8266_t* ESP8266) {
	char resp[4];
	
	/* Format response, use first byte of IP as first string */
	sprintf(resp, "%d", ESP8266->APIP[0]);
	
	/* Try to send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_CWLIF, "AT+CWLIF\r\n", resp) != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Reset counters in structure */
	ESP8266->ConnectedStations.Count = 0;
	
	/* Return result */
	return ESP8266->Result;
}

/******************************************/
/*               TCP CLIENT               */
/******************************************/
ESP8266_Result_t ESP8266_StartClientConnection(ESP8266_t* ESP8266, char* name, char* location, uint16_t port, void* user_parameters) {
	int8_t conn = -1;
	uint8_t i = 0;
	
	/* Check if IDLE */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Check if connected to network */
	ESP8266_CHECK_WIFICONNECTED(ESP8266);
	
	/* Find available connection */
	for (i = 0; i < ESP8266_MAX_CONNECTIONS; i++) {
		if (!ESP8266->Connection[i].Active) {
			/* Save */
			conn = i;
			
			break;
		}
	}
	
	/* Try it */
	if (conn != -1) {
		char tmp[100];
		/* Format command */
		sprintf(tmp, "AT+CIPSTART=%d,\"TCP\",\"%s\",%d\r\n", conn, location, port);
		
		/* Send command */
		if (SendCommand(ESP8266, ESP8266_COMMAND_CIPSTART, tmp, NULL) != ESP_OK) {
			return ESP8266->Result;
		}
		
		/* We are active now as client */
		ESP8266->Connection[i].Active = 1;
		ESP8266->Connection[i].Client = 1;
		ESP8266->Connection[i].TotalBytesReceived = 0;
		ESP8266->Connection[i].Number = conn;
#if ESP8266_USE_SINGLE_CONNECTION_BUFFER == 1
		ESP8266->Connection[i].Data = ConnectionData;
#endif
		ESP8266->StartConnectionSent = i;
		
		/* Copy values */
		strncpy(ESP8266->Connection[i].Name, name, sizeof(ESP8266->Connection[i].Name));
		ESP8266->Connection[i].UserParameters = user_parameters;
		
		/* Return OK */
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
	}
	
	/* Return error */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
}

/******************************************/
/*              PING SUPPORT              */
/******************************************/
#if ESP8266_USE_PING
ESP8266_Result_t ESP8266_Ping(ESP8266_t* ESP8266, char* addr) {
	char tmp[100];
	
	/* Check idle */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Device must be connected to wifi */
	ESP8266_CHECK_WIFICONNECTED(ESP8266);
	
	/* Save ping address information */
	strcpy(ESP8266->PING.Address, addr);
	
	/* Format command for pinging */
	sprintf(tmp, "AT+PING=\"%s\"\r\n", addr);
	
	/* Reset flag */
	ESP8266->PING.Success = 0;
	
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_PING, tmp, "+") == ESP_OK) {
		/* Call user function */
		ESP8266_Callback_PingStarted(ESP8266, addr);
	}
	
	/* Return status */
	return ESP8266->Result;
}
#endif

uint16_t ESP8266_DataReceived(uint8_t* ch, uint16_t count) {
	/* Writes data to USART buffer */
	return BUFFER_Write(&USART_Buffer, ch, count);
}

/******************************************/
/*                CALLBACKS               */
/******************************************/
/* Called when ready string detected */
__weak void ESP8266_Callback_DeviceReady(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_DeviceReady could be implemented in the user file
	*/
}

/* Called when watchdog reset on ESP8266 is detected */
__weak void ESP8266_Callback_WatchdogReset(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WatchdogReset could be implemented in the user file
	*/
}

/* Called when module disconnects from wifi network */
__weak void ESP8266_Callback_WifiDisconnected(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiDisconnected could be implemented in the user file
	*/
}

/* Called when module connects to wifi network */
__weak void ESP8266_Callback_WifiConnected(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiConnected could be implemented in the user file
	*/
}

/* Called when connection to wifi network fails */
__weak void ESP8266_Callback_WifiConnectFailed(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiConnectFailed could be implemented in the user file
	*/
}

/* Called when module gets IP address */
__weak void ESP8266_Callback_WifiGotIP(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiGotIP could be implemented in the user file
	*/
}

/* Called when IP is set */
__weak void ESP8266_Callback_WifiIPSet(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiIPSet could be implemented in the user file
	*/
}

/* Called when wifi spot is detected */
__weak void ESP8266_Callback_WifiDetected(ESP8266_t* ESP8266, ESP8266_APs_t* ESP8266_AP) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_WifiDetected could be implemented in the user file
	*/
}

/* Called on DHCP timeout */
__weak void ESP8266_Callback_DHCPTimeout(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_DHCPTimeout could be implemented in the user file
	*/
}

/* Called when "x,CONNECT" is detected */
__weak void ESP8266_Callback_ServerConnectionActive(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionActive could be implemented in the user file
	*/
}

/* Called when "x,CLOSED" is detected */
__weak void ESP8266_Callback_ServerConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionClosed could be implemented in the user file
	*/
}

/* Called when "+IPD..." is detected */
__weak void ESP8266_Callback_ServerConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionDataReceived could be implemented in the user file
	*/
}

/* Called when user should fill data buffer to be sent with connection */
__weak uint16_t ESP8266_Callback_ServerConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionSendData could be implemented in the user file
	*/
	
	/* Return number of bytes in array */
	return 0;
}

/* Called when data are send successfully */
__weak void ESP8266_Callback_ServerConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionDataSent could be implemented in the user file
	*/
}

/* Called when error returned trying to sent data */
__weak void ESP8266_Callback_ServerConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ServerConnectionDataSentError could be implemented in the user file
	*/
}

/* Called when user is connected to server as client */
__weak void ESP8266_Callback_ClientConnectionConnected(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionConnected could be implemented in the user file
	*/
}

/* Called when user should fill data buffer to be sent with connection */
__weak uint16_t ESP8266_Callback_ClientConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionSendData could be implemented in the user file
	*/
	
	/* Return number of bytes in array */
	return 0;
}

/* Called when data are send successfully */
__weak void ESP8266_Callback_ClientConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionDataSent could be implemented in the user file
	*/
}

/* Called when error returned trying to sent data */
__weak void ESP8266_Callback_ClientConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionDataSentError could be implemented in the user file
	*/
}

/* Called when server returns data back to client */
__weak void ESP8266_Callback_ClientConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionDataReceived could be implemented in the user file
	*/
}

/* Called when ERROR is returned on AT+CIPSTART command */
__weak void ESP8266_Callback_ClientConnectionError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionError could be implemented in the user file
	*/
}

/* Called when timeout is reached on AT+CIPSTART command */
__weak void ESP8266_Callback_ClientConnectionTimeout(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionTimeout could be implemented in the user file
	*/
}

/* Called when "x,CLOSED" is detected when connection was made as client */
__weak void ESP8266_Callback_ClientConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ClientConnectionClosed could be implemented in the user file
	*/
}

#if ESP8266_USE_PING
/* Called when pinging started */
__weak void ESP8266_Callback_PingStarted(ESP8266_t* ESP8266, char* address) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_PingStarted could be implemented in the user file
	*/
}

/* Called when PING command ends */
__weak void ESP8266_Callback_PingFinished(ESP8266_t* ESP8266, ESP8266_Ping_t* PING) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_PingFinished could be implemented in the user file
	*/
}
#endif

#if ESP8266_USE_FIRMWAREUPDATE
/* Called on status messages for network firmware update */
__weak void ESP8266_Callback_FirmwareUpdateStatus(ESP8266_t* ESP8266, ESP8266_FirmwareUpdate_t status) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_FirmwareUpdateStatus could be implemented in the user file
	*/
}

/* Called when firmware network update was successful */
__weak void ESP8266_Callback_FirmwareUpdateSuccess(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_FirmwareUpdateSuccess could be implemented in the user file
	*/
}

/* Called when firmware network error occurred */
__weak void ESP8266_Callback_FirmwareUpdateError(ESP8266_t* ESP8266) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_FirmwareUpdateError could be implemented in the user file
	*/
}
#endif

/* Called when AT+CWLIF returns OK */
__weak void ESP8266_Callback_ConnectedStationsDetected(ESP8266_t* ESP8266, ESP8266_ConnectedStations_t* Stations) {
	/* NOTE: This function Should not be modified, when the callback is needed,
           the ESP8266_Callback_ConnectedStationsDetected could be implemented in the user file
	*/
}

/******************************************/
/*           PRIVATE FUNCTIONS            */
/******************************************/
static void ParseCWSAP(ESP8266_t* ESP8266, char* Buffer) {
	char* ptr;
	uint8_t i, cnt;
	
	/* Find : in string */
	ptr = Buffer;
	while (*ptr) {
		if (*ptr == ':') {
			break;
		}
		ptr++;
	}
	
	/* Check if exists */
	if (ptr == 0) {
		return;
	}
	
	/* Go to '"' character */
	ptr++;
	
	/**** NEEDS IMPROVEMENT ****/
	/* If '"' character is inside SSID or password part, parser will fail */
	
	/***** SSID ****/
	ESP8266->AP.SSID[0] = 0;
	if (*ptr == '"') {
		ptr++;
	}
	
	/* Copy till "," which indicates end of SSID string and start of password part */
	i = 0;
	while (*ptr && (*ptr != '"' || *(ptr + 1) != ',' || *(ptr + 2) != '"')) {
		ESP8266->AP.SSID[i++] = *ptr++;
	}
	ESP8266->AP.SSID[i++] = 0;
	
	/* Increase pointer by 3, ignore "," part */
	ptr += 3;
	
	/* Copy till ", which indicates end of password string and start of number */
	i = 0;
	while (*ptr && (*ptr != '"' || *(ptr + 1) != ',')) {
		ESP8266->AP.Pass[i++] = *ptr++;
	}
	ESP8266->AP.Pass[i++] = 0;
	
	/* Increase pointer by 2 */
	ptr += 2;
	
	/* Get channel number */
	ESP8266->AP.Channel = ParseNumber(ptr, &cnt);
	
	/* Increase pointer and comma */
	ptr += cnt + 1;
	
	/* Get ecn value */
	ESP8266->AP.Ecn = (ESP8266_Ecn_t)ParseNumber(ptr, &cnt);
	
	/* Increase pointer and comma */
	ptr += cnt + 1;
	
	/* Get max connections value */
	ESP8266->AP.MaxConnections = ParseNumber(ptr, &cnt);
	
	/* Increase pointer and comma */
	ptr += cnt + 1;
	
	/* Get hidden value */
	ESP8266->AP.Hidden = ParseNumber(ptr, &cnt);
}

#if ESP8266_USE_APSEARCH
static void ParseCWLAP(ESP8266_t* ESP8266, char* Buffer) {
	uint8_t pos = 7, num = 0;
	char* ptr;
	
	/* Check if we have memory available first */
	if (ESP8266_APs.Count >= ESP8266_MAX_CONNECTIONS) {
		return;
	}
	
	/* Get start pointer */
	if (Buffer[pos] == '(') {
		pos++;
	}
	
	/* Get token */
	ptr = strtok(&Buffer[pos], ",");
	
	/* Do it until token != NULL */
	while (ptr != NULL) {
		/* Send debug */
		
		/* Get positions */
		switch (num++) {
			case 0: 
				ESP8266_APs.AP[ESP8266_APs.Count].Ecn = ParseNumber(ptr, NULL);
				break;
			case 1:
				/* Ignore first and last " */
				ptr++;
				ptr[strlen(ptr) - 1] = 0;
				strcpy(ESP8266_APs.AP[ESP8266_APs.Count].SSID, ptr);
				break;
			case 2: 
				ESP8266_APs.AP[ESP8266_APs.Count].RSSI = ParseNumber(ptr, NULL);
				break;
			case 3:
				/* Ignore first and last " */
				ptr++;
				ptr[strlen(ptr) - 1] = 0;
			
				/* Parse MAC address */
				ParseMAC(ptr, ESP8266_APs.AP[ESP8266_APs.Count].MAC, NULL);
			
				break;
			case 4: 
				ESP8266_APs.AP[ESP8266_APs.Count].Channel = ParseNumber(ptr, NULL);
				break;
			case 5: 
				ESP8266_APs.AP[ESP8266_APs.Count].Offset = ParseNumber(ptr, NULL);
				break;
			case 6: 
				ESP8266_APs.AP[ESP8266_APs.Count].Calibration = ParseNumber(ptr, NULL);
				break;
			default: break;
		}
		
		/* Get new token */
		ptr = strtok(NULL, ",");
	}
	
	/* Increase count */
	ESP8266_APs.Count++;
}
#endif

static void ParseCIPSTA(ESP8266_t* ESP8266, char* Buffer) {
	uint8_t pos, s;
	uint8_t command = 0;
	
	/* Get positions */
	if (strncmp("+CIPSTA_CUR:ip", Buffer, 14) == 0) {
		pos = 14;
		s = 1;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPSTA_CUR:netmask", Buffer, 19) == 0) {
		pos = 19;
		s = 2;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPSTA_CUR:gateway", Buffer, 19) == 0) {
		pos = 19;
		s = 3;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPSTA:ip", Buffer, 10) == 0) {
		pos = 10;
		s = 1;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPSTA:netmask", Buffer, 15) == 0) {
		pos = 15;
		s = 2;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPSTA:gateway", Buffer, 15) == 0) {
		pos = 15;
		s = 3;
		command = ESP8266_COMMAND_CIPSTA;
	} else if (strncmp("+CIPAP_CUR:ip", Buffer, 13) == 0) {
		pos = 13;
		s = 1;
	} else if (strncmp("+CIPAP_CUR:netmask", Buffer, 18) == 0) {
		pos = 18;
		s = 2;
	} else if (strncmp("+CIPAP_CUR:gateway", Buffer, 18) == 0) {
		pos = 18;
		s = 3;
	} else if (strncmp("+CIPAP:ip", Buffer, 9) == 0) {
		pos = 9;
		s = 1;
	} else if (strncmp("+CIPAP:netmask", Buffer, 14) == 0) {
		pos = 14;
		s = 2;
	} else if (strncmp("+CIPAP:gateway", Buffer, 14) == 0) {
		pos = 14;
		s = 3;
	}
	
	/* Copy content */
	if (command == ESP8266_COMMAND_CIPSTA) {
		switch (s) {
			case 1:
				/* Parse IP string */
				ParseIP(&Buffer[pos + 2], ESP8266->STAIP, NULL);
				ESP8266->Flags.F.STAIPIsSet = 1;
				break;
			case 2:
				ParseIP(&Buffer[pos + 2], ESP8266->STANetmask, NULL);
				ESP8266->Flags.F.STANetmaskIsSet = 1;
				break;
			case 3:
				ParseIP(&Buffer[pos + 2], ESP8266->STAGateway, NULL);
				ESP8266->Flags.F.STAGatewayIsSet = 1;
				break;
			default: break;
		}
	} else {
		switch (s) {
			case 1:
				/* Parse IP string */
				ParseIP(&Buffer[pos + 2], ESP8266->APIP, NULL);
				ESP8266->Flags.F.APIPIsSet = 1;
				break;
			case 2:
				ParseIP(&Buffer[pos + 2], ESP8266->APNetmask, NULL);
				ESP8266->Flags.F.APNetmaskIsSet = 1;
				break;
			case 3:
				ParseIP(&Buffer[pos + 2], ESP8266->APGateway, NULL);
				ESP8266->Flags.F.APGatewayIsSet = 1;
				break;
			default: break;
		}		
	}
}

static void ParseCWLIF(ESP8266_t* ESP8266, char* Buffer) {
	uint8_t cnt;
	
	/* Check if memory available */
	if (ESP8266->ConnectedStations.Count >= ESP8266_MAX_CONNECTEDSTATIONS) {
		return;
	}
	
	/* Parse IP */
	ParseIP(Buffer, ESP8266->ConnectedStations.Stations[ESP8266->ConnectedStations.Count].IP, &cnt);
	
	/* Parse MAC */
	ParseMAC(&Buffer[cnt + 1], ESP8266->ConnectedStations.Stations[ESP8266->ConnectedStations.Count].MAC, NULL);
	
	/* Increase counter */
	ESP8266->ConnectedStations.Count++;
}

static void ParseCWJAP(ESP8266_t* ESP8266, char* Buffer) {
	char* ptr = Buffer;
	uint8_t i, cnt;
	
	/* Check for existance */
	if (!strstr(Buffer, "+CWJAP_")) {
		return;
	}
	
	/* Find first " character */
	while (*ptr && *ptr != '"') {
		ptr++;
	}
	
	/* Remove first " for SSID */
	ptr++;
	
	/* Parse SSID part */
	i = 0;
	while (*ptr && (*ptr != '"' || *(ptr + 1) != ',' || *(ptr + 2) != '"')) {
		ESP8266->ConnectedWifi.SSID[i++] = *ptr++;
	}
	ESP8266->ConnectedWifi.SSID[i++] = 0;
	
	/* Increase pointer by 3, ignore "," part */
	ptr += 3;
	
	/* Get MAC */
	ParseMAC(ptr, ESP8266->ConnectedWifi.MAC, &cnt);
	
	/* Increase counter by elements in MAC address and ", part */
	ptr += cnt + 2;
	
	/* Get channel */
	ESP8266->ConnectedWifi.Channel = ParseNumber(ptr, &cnt);
	
	/* Increase position */
	ptr += cnt + 1;
	
	/* Get RSSI */
	ESP8266->ConnectedWifi.RSSI = ParseNumber(ptr, &cnt);
}

static uint8_t Hex2Num(char a) {
	if (a >= '0' && a <= '9') {
		return a - '0';
	} else if (a >= 'a' && a <= 'f') {
		return (a - 'a') + 10;
	} else if (a >= 'A' && a <= 'F') {
		return (a - 'A') + 10;
	}
	
	return 0;
}

static uint8_t CHARISHEXNUM(char a) {
	if (a >= '0' && a <= '9') {
		return 1;
	} else if (a >= 'a' && a <= 'f') {
		return 1;
	} else if (a >= 'A' && a <= 'F') {
		return 1;
	}
	
	return 0;
}

static int32_t ParseNumber(char* ptr, uint8_t* cnt) {
	uint8_t minus = 0;
	int32_t sum = 0;
	uint8_t i = 0;
	
	/* Check for minus character */
	if (*ptr == '-') {
		minus = 1;
		ptr++;
		i++;
	}
	
	/* Parse number */
	while (CHARISNUM(*ptr)) {
		sum = 10 * sum + CHAR2NUM(*ptr);
		ptr++;
		i++;
	}
	
	/* Save number of characters used for number */
	if (cnt != NULL) {
		*cnt = i;
	}
	
	/* Minus detected */
	if (minus) {
		return 0 - sum;
	}
	
	/* Return number */
	return sum;
}

static uint32_t ParseHexNumber(char* ptr, uint8_t* cnt) {
	uint32_t sum = 0;
	uint8_t i = 0;
	
	/* Parse number */
	while (CHARISHEXNUM(*ptr)) {
		sum = 16 * sum + Hex2Num(*ptr);
		ptr++;
		i++;
	}
	
	/* Save number of characters used for number */
	if (cnt != NULL) {
		*cnt = i;
	}
	
	/* Return number */
	return sum;
}

static void ParseIP(char* ip_str, uint8_t* arr, uint8_t* cnt) {
	char* token;
	uint8_t i = 0;
	uint8_t x = 0;
	uint8_t c;
	char Data[16];
	
	/* Make a string copy first */
	memcpy(Data, ip_str, sizeof(Data) - 1);
	Data[sizeof(Data) - 1] = 0;
	
	/* Parse numbers, skip :" */
	token = strtok(Data, ".");
	
	/* We expect 4 loops */
	while (token != NULL) {
		/* Parse number */
		arr[x++] = ParseNumber(token, &c);
		
		/* Save number of characters used for IP string */
		i += c;
		
		/* Max 4 loops */
		if (x >= 4) {
			break;
		}
		
		/* Increase number of characters, used for "." (DOT) */
		i++;
		
		/* Get new token */
		token = strtok(NULL, ".");
	}
	
	/* Save number of characters */
	if (cnt != NULL) {
		*cnt = i;
	}
}

static void ParseMAC(char* ptr, uint8_t* arr, uint8_t* cnt) {
	char* hexptr;
	uint8_t hexnum = 0;
	uint8_t tmpcnt = 0, sum = 0;
	
	/* Get token */
	hexptr = strtok(ptr, ":");

	/* Do it till NULL */
	while (hexptr != NULL) {
		/* Parse hex */
		arr[hexnum++] = ParseHexNumber(hexptr, &tmpcnt);
		
		/* Add to sum value */
		sum += tmpcnt;
		
		/* We have 6 characters */
		if (hexnum >= 6) {
			break;
		}
		
		/* Increase for ":" */
		sum++;
		
		/* Get new token */
		hexptr = strtok(NULL, ":");
	}
	
	/* Save value */
	if (cnt) {
		*cnt = sum;
	}
}

static void ParseReceived(ESP8266_t* ESP8266, char* Received, uint8_t from_usart_buffer, uint16_t bufflen) {
	char* ch_ptr;
	uint8_t bytes_cnt;
	uint32_t ipd_ptr = 0;
	ESP8266_Connection_t* Conn;
	
	/* Update last activity */
	ESP8266->LastReceivedTime = ESP8266->Time;
	
	/* Check for empty new line */
	if (bufflen == 2 && Received[0] == '\r' && Received[1] == '\n') {
		return;
	}
	
	/* First check, if any command is active */
	if (ESP8266->ActiveCommand != ESP8266_COMMAND_IDLE && from_usart_buffer == 1) {
		/* Check if string does not belong to this command */
		if (
			strcmp(Received, "OK\r\n") != 0 &&
			strcmp(Received, "SEND OK\r\n") != 0 &&
			strcmp(Received, "ERROR\r\n") != 0 &&
			strcmp(Received, "ready\r\n") != 0 &&
			strcmp(Received, "busy p...\r\n") != 0 &&
			strncmp(Received, "+IPD:", 4) != 0 &&
			strncmp(Received, ESP8266->ActiveCommandResponse[0], strlen(ESP8266->ActiveCommandResponse[0])) != 0
		) {
			/* Save string to temporary buffer, because we received a string which does not belong to this command */
			BUFFER_WriteString(&TMP_Buffer, Received);
			
			/* Return from function */
			return;
		}
	}
	
	/* Device is ready */
	if (strcmp(Received, "ready\r\n") == 0) {
		ESP8266_Callback_DeviceReady(ESP8266);
	}
	
	/* Device WDT reset */
	if (strcmp(Received, "wdt reset\r\n") == 0) {
		ESP8266_Callback_WatchdogReset(ESP8266);
	}
	
	/* Call user callback functions if not already */
	CallConnectionCallbacks(ESP8266);
	
	/* We are connected to Wi-Fi */
	if (strcmp(Received, "WIFI CONNECTED\r\n") == 0) {
		/* Set flag */
		ESP8266->Flags.F.WifiConnected = 1;
		
		/* Call user callback function */
		ESP8266_Callback_WifiConnected(ESP8266);
	} else if (strcmp(Received, "WIFI DISCONNECT\r\n") == 0) {
		/* Clear flags */
		ESP8266->Flags.F.WifiConnected = 0;
		ESP8266->Flags.F.WifiGotIP = 0;
		
		/* Reset connected wifi structure */
		memset((uint8_t *)&ESP8266->ConnectedWifi, 0, sizeof(ESP8266->ConnectedWifi));
		
		/* Reset all connections */
		ESP8266_RESET_CONNECTIONS(ESP8266);
		
		/* Call user callback function */
		ESP8266_Callback_WifiDisconnected(ESP8266);
	} else if (strcmp(Received, "WIFI GOT IP\r\n") == 0) {
		/* Wifi got IP address */
		ESP8266->Flags.F.WifiGotIP = 1;
		
		/* Call user callback function */
		ESP8266_Callback_WifiGotIP(ESP8266);
	}
			
	/* In case data were send */
	if (strstr(Received, "SEND OK\r\n") != NULL) {
		uint8_t cnt;
		
		/* Reset active command so user will be able to call new command in callback function */
		ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
		
		for (cnt = 0; cnt < ESP8266_MAX_CONNECTIONS; cnt++) {
			/* Check for data sent */
			if (ESP8266->Connection[cnt].WaitingSentRespond) {
				/* Reset flag */
				ESP8266->Connection[cnt].WaitingSentRespond = 0;
				
				/* Call user function according to connection type */
				if (ESP8266->Connection[cnt].Client) {
					/* Client mode */
					ESP8266_Callback_ClientConnectionDataSent(ESP8266, &ESP8266->Connection[cnt]);
				} else {
					/* Server mode */
					ESP8266_Callback_ServerConnectionDataSent(ESP8266, &ESP8266->Connection[cnt]);
				}
			}
		}
	}
	
	/* Check if +IPD was received with incoming data */
	if (strncmp(Received, "+IPD", 4) == 0) {		
		/* If we are not in IPD mode already */
		/* Go to IPD mode */
		ESP8266->IPD.InIPD = 1;
		ESP8266->IPD.USART_Buffer = from_usart_buffer;
		
		/* Reset pointer */
		ipd_ptr = 5;
		
		/* Get connection number from IPD statement */
		ESP8266->IPD.ConnNumber = CHAR2NUM(Received[ipd_ptr]);
		
		/* Set working buffer for this connection */
#if ESP8266_USE_SINGLE_CONNECTION_BUFFER == 1
		ESP8266->Connection[ESP8266->IPD.ConnNumber].Data = ConnectionData;
#endif
		
		/* Save connection number */
		ESP8266->Connection[ESP8266->IPD.ConnNumber].Number = ESP8266->IPD.ConnNumber;
		
		/* Increase pointer by 2 */
		ipd_ptr += 2;
		
		/* Save number of received bytes */
		ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived = ParseNumber(&Received[ipd_ptr], &bytes_cnt);
		
		/* First time */
		if (ESP8266->Connection[ESP8266->IPD.ConnNumber].TotalBytesReceived == 0) {
			/* Reset flag */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].HeadersDone = 0;
			
			/* This is first packet of data */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].FirstPacket = 1;
		} else {
			/* This is not first packet */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].FirstPacket = 0;
		}
		
		/* Save total number of bytes */
		ESP8266->Connection[ESP8266->IPD.ConnNumber].TotalBytesReceived += ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived;
		
		/* Increase global number of bytes received from ESP8266 module to stack */
		ESP8266->TotalBytesReceived += ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived;
		
		/* Increase pointer for number of characters for number and for comma */
		ipd_ptr += bytes_cnt + 1;
		
		/* Save IP */
		ParseIP(&Received[ipd_ptr], ESP8266->Connection[ESP8266->IPD.ConnNumber].RemoteIP, &bytes_cnt);
		
		/* Increase pointer for number of characters for IP string and for comma */
		ipd_ptr += bytes_cnt + 1;
		
		/* Save PORT */
		ESP8266->Connection[ESP8266->IPD.ConnNumber].RemotePort = ParseNumber(&Received[ipd_ptr], &bytes_cnt);
		
		/* Increase pointer */
		ipd_ptr += bytes_cnt + 1;
		
		/* Find : element where real data starts */
		ipd_ptr = 0;
		while (Received[ipd_ptr] != ':') {
			ipd_ptr++;
		}
		ipd_ptr++;
		
		/* Copy content to beginning of buffer */
		memcpy((uint8_t *)ESP8266->Connection[ESP8266->IPD.ConnNumber].Data, (uint8_t *)&Received[ipd_ptr], bufflen - ipd_ptr);
		
		/* Check for length */
		if ((bufflen - ipd_ptr) > ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived) {
			/* Add zero at the end of string */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].Data[ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived] = 0;
		}
		
		/* Calculate remaining bytes */
		ESP8266->IPD.InPtr = ESP8266->IPD.PtrTotal = bufflen - ipd_ptr;
		
		/* Check remaining data */
		if (ipd_ptr >= ESP8266->Connection[ESP8266->IPD.ConnNumber].BytesReceived) {
			/* Not in IPD anymore */
			ESP8266->IPD.InIPD = 0;
			
			/* Set package data size */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].DataSize = ipd_ptr;
			ESP8266->Connection[ESP8266->IPD.ConnNumber].LastPart = 1;
			
			/* Enable flag to call received data callback */
			ESP8266->Connection[ESP8266->IPD.ConnNumber].CallDataReceived = 1;
		}
	}
	
	/* Check if we have a new connection */
	if ((ch_ptr = (char *)mem_mem(Received, bufflen, ",CONNECT\r\n", 10)) != NULL) {
		/* New connection has been made */
		Conn = &ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))];
		Conn->Active = 1;
		Conn->Number = CHAR2NUM(*(ch_ptr - 1));
		
		/* Call user function according to connection type (client, server) */
		if (Conn->Client) {			
			/* Reset current connection */
			if (ESP8266->ActiveCommand == ESP8266_COMMAND_CIPSTART) {
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			
			/* Connection started as client */
			ESP8266_Callback_ClientConnectionConnected(ESP8266, Conn);
		} else {
			/* Connection started as server */
			ESP8266_Callback_ServerConnectionActive(ESP8266, Conn);
		}
	}
	
	/* Check if already connected */
	if (strstr(Received, "ALREADY CONNECTED\r\n") != NULL) {
		printf("Connection %d already connected!\r\n", ESP8266->StartConnectionSent);
	}
	
	/* Check if we have a closed connection */
	if ((ch_ptr = (char *)mem_mem(Received, bufflen, ",CLOSED\r\n", 9)) != NULL && Received != ch_ptr) {
		uint8_t client, active;
		
		/* Check if CLOSED statement is on beginning, if not, write it to temporary buffer and leave here */
		/* If not on beginning of string, probably ,CLOSED was returned after +IPD statement */
		/* Make string standalone */
		if (ch_ptr == (Received + 1)) {
			/* Save values */
			client = ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))].Client;
			active = ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))].Active;
			
			/* Connection closed, reset flags now */
			ESP8266_RESETCONNECTION(ESP8266, &ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))]);
			
			/* Call user function */
			if (active) {
				if (client) {
					/* Client connection closed */
					ESP8266_Callback_ClientConnectionClosed(ESP8266, &ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))]);
				} else {
					/* Server connection closed */
					ESP8266_Callback_ServerConnectionClosed(ESP8266, &ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))]);
				}
			}
		} else {
			/* Write to temporary buffer */
			BUFFER_Write(&TMP_Buffer, (uint8_t *)(ch_ptr - 1), 10);
		}
	}
	
	/* Check if we have a new connection */
	if ((ch_ptr = strstr(Received, ",CONNECT FAIL\r\n")) != NULL) {
		/* New connection has been made */
		Conn = &ESP8266->Connection[CHAR2NUM(*(ch_ptr - 1))];
		ESP8266_RESETCONNECTION(ESP8266, Conn);
		Conn->Number = CHAR2NUM(*(ch_ptr - 1));
		
		/* Call user function according to connection type (client, server) */
		if (Conn->Client) {
			/* Reset current connection */
			if (ESP8266->ActiveCommand == ESP8266_COMMAND_CIPSTART) {
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			
			/* Connection failed */
			ESP8266_Callback_ClientConnectionError(ESP8266, Conn);
		}
	}
	
	/* Check commands we have sent */
	switch (ESP8266->ActiveCommand) {
		/* Check wifi disconnect response */
		case ESP8266_COMMAND_CWQAP:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_CWJAP:			
			/* We send command and we have error response */
			if (strncmp(Received, "+CWJAP:", 7) == 0) {
				/* We received an error, wait for "FAIL" string for next time */
				strcpy(ESP8266->ActiveCommandResponse[0], "FAIL\r\n");
				
				/* Check reason */
				ESP8266->WifiConnectError = (ESP8266_WifiConnectError_t)CHAR2NUM(Received[7]);
			}
			
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			
			if (strcmp(Received, "FAIL\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Call user function */
				ESP8266_Callback_WifiConnectFailed(ESP8266);
			}
			break;
		case ESP8266_COMMAND_CWJAP_GET:
			/* We sent command to get current connected AP */
			if (strncmp(Received, "+CWJAP_CUR:", 11) == 0) {
				/* Parse string */
				ParseCWJAP(ESP8266, Received);
			}
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
#if ESP8266_USE_APSEARCH
		case ESP8266_COMMAND_CWLAP:
			/* CWLAP received, parse it */
			if (strncmp(Received, "+CWLAP:", 7) == 0) {
				/* Parse CWLAP */
				ParseCWLAP(ESP8266, Received);
			}
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Call user function */
				ESP8266_Callback_WifiDetected(ESP8266, &ESP8266_APs);
			}
			break;
#endif
		case ESP8266_COMMAND_CWSAP:
			/* CWLAP received, parse it */
			if (strncmp(Received, "+CWSAP", 6) == 0) {
				/* Parse CWLAP */
				ParseCWSAP(ESP8266, Received);
			}
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_CIPSTA:
			/* CIPSTA detected */
			if (strncmp(Received, "+CIPSTA", 7) == 0) {
				/* Parse CIPSTA */
				ParseCIPSTA(ESP8266, Received);
			}
		
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Callback function */
				ESP8266_Callback_WifiIPSet(ESP8266);
			}
			break;
		case ESP8266_COMMAND_CIPAP:
			/* CIPAP detected */
			if (strncmp(Received, "+CIPAP", 6) == 0) {
				/* Parse CIPAP (or CIPSTA) */
				ParseCIPSTA(ESP8266, Received);
			}
		
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_CWMODE:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Save mode */
				ESP8266->Mode = ESP8266->SentMode;
			}
			break;
		case ESP8266_COMMAND_CIPSERVER:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_SEND:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_SENDDATA;
				
				/* Do not reset command, instead, wait for wrapper command! */
				ESP8266->Flags.F.WaitForWrapper = 1;
				
				/* We are now waiting for SEND OK */
				strcpy(ESP8266->ActiveCommandResponse[0], "SEND OK");
			}
			break;
		case ESP8266_COMMAND_SENDDATA:
			break;
		case ESP8266_COMMAND_CIPSTART:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			if (strcmp(Received, "ERROR\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Reset connection */
				ESP8266_RESETCONNECTION(ESP8266, &ESP8266->Connection[ESP8266->StartConnectionSent]);
				
				/* Call user function */
				ESP8266_Callback_ClientConnectionError(ESP8266, &ESP8266->Connection[ESP8266->StartConnectionSent]);
			}
			break;
		case ESP8266_COMMAND_CIPMUX:
		case ESP8266_COMMAND_CIPDINFO:
		case ESP8266_COMMAND_AT:
		case ESP8266_COMMAND_UART:
		case ESP8266_COMMAND_CLOSE:
		case ESP8266_COMMAND_SLEEP:
		case ESP8266_COMMAND_GSLP:
		case ESP8266_COMMAND_CIPSTO:
		case ESP8266_COMMAND_RESTORE:
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_RST:
			if (strcmp(Received, "ready\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Set flag */
				ESP8266->Flags.F.LastOperationStatus = 1;
			}
			break;
#if ESP8266_USE_PING
		case ESP8266_COMMAND_PING:
			/* Check if data about ping milliseconds are received */
			if (Received[0] == '+') {
				/* Parse number for pinging */
				ESP8266->PING.Time = ParseNumber(&Received[1], NULL);
			}
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Set status */
				ESP8266->PING.Success = 1;
				
				/* Error callback */
				ESP8266_Callback_PingFinished(ESP8266, &ESP8266->PING);
			} else if (strcmp(Received, "ERROR\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Set status */
				ESP8266->PING.Success = 0;
				
				/* Error callback */
				ESP8266_Callback_PingFinished(ESP8266, &ESP8266->PING);
			}
			break;
#endif
		case ESP8266_COMMAND_CIPSTAMAC:
			/* CIPSTA detected */
			if (strncmp(Received, "+CIPSTAMAC", 10) == 0) {
				/* Parse CIPSTA */
				ParseMAC(&Received[12], ESP8266->STAMAC, NULL);
			}
		
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
		case ESP8266_COMMAND_CIPAPMAC:
			/* CIPSTA detected */
			if (strncmp(Received, "+CIPAPMAC", 9) == 0) {
				/* Parse CIPSTA */
				ParseMAC(&Received[11], ESP8266->APMAC, NULL);
			}
		
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
			}
			break;
#if ESP8266_USE_FIRMWAREUPDATE
		case ESP8266_COMMAND_CIUPDATE:
			/* Check for strings for update */
			if (strncmp(Received, "+CIPUPDATE:", 11) == 0) {
				/* Get current number */
				uint8_t num = CHAR2NUM(Received[11]);
				
				/* Check step */
				if (num == 4) {
					/* We are waiting last step, increase timeout */
					ESP8266->Timeout = 10 * ESP8266_TIMEOUT;
				}
				
				/* Call user function */
				ESP8266_Callback_FirmwareUpdateStatus(ESP8266, (ESP8266_FirmwareUpdate_t)num);
			}
		
			if (strcmp(Received, "OK\r\n") == 0 || strcmp(Received, "ready\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Reset timeout */
				ESP8266->Timeout = ESP8266_TIMEOUT;
				
				/* Call user function */
				ESP8266_Callback_FirmwareUpdateSuccess(ESP8266);
			}
			
			if (strcmp(Received, "ERROR\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Reset timeout */
				ESP8266->Timeout = ESP8266_TIMEOUT;
				
				/* Call user function */
				ESP8266_Callback_FirmwareUpdateError(ESP8266);
			}
			break;
#endif
		case ESP8266_COMMAND_CWLIF:
			/* Check if first character is number */
			if (CHARISNUM(Received[0])) {
				/* Parse response */
				ParseCWLIF(ESP8266, Received);
			}
		
			if (strcmp(Received, "OK\r\n") == 0) {
				/* Reset active command */
				ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
				
				/* Call user function */
				ESP8266_Callback_ConnectedStationsDetected(ESP8266, &ESP8266->ConnectedStations);
			}
			break;
		default:
			/* No command was used to send, data received without command */
			break;
	}
	
	/* Set flag for last operation status */
	if (strcmp(Received, "OK\r\n") == 0) {
		ESP8266->Flags.F.LastOperationStatus = 1;
		
		/* Reset active command */
		/* TODO: Check if OK here */
		if (ESP8266->ActiveCommand != ESP8266_COMMAND_SEND) {
			/* We are waiting for "> " string */
			ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
		}
	}
	if (strcmp(Received, "ERROR\r\n") == 0 || strcmp(Received, "busy p...\r\n") == 0) {
		ESP8266->Flags.F.LastOperationStatus = 0;
		
		/* Reset active command */
		/* TODO: Check if ERROR here */
		ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
	}
	if (strcmp(Received, "SEND OK\r\n") == 0) {
		/* Force IDLE when we are in SEND mode and SEND OK is returned. Do not wait for "> " wrapper */
		ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
		
		/* Clear flag */
		ESP8266->Flags.F.WaitForWrapper = 0;
	}
}

static ESP8266_Result_t SendCommand(ESP8266_t* ESP8266, uint8_t Command, char* CommandStr, char* StartRespond) {
	/* Check idle mode */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Clear buffer */
	if (Command == ESP8266_COMMAND_UART) {
		/* Reset USART buffer */
		BUFFER_Reset(&USART_Buffer);
	}
	
	/* Clear buffer and send command */
	ESP8266_LL_USARTSend((uint8_t *)CommandStr, strlen(CommandStr));
	
	/* Save current active command */
	ESP8266->ActiveCommand = Command;
	ESP8266->ActiveCommandResponse[0][0] = 0;
	strcpy(ESP8266->ActiveCommandResponse[0], StartRespond);
	
	/* Set command start time */
	ESP8266->StartTime = ESP8266->Time;
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

static char* EscapeString(char* str) {
	static char buff[100];
	char* str_ptr = buff;
	
	/* Go through string */
	while (*str) {
		/* Check for special character */
		if (*str == ',' || *str == '"' || *str == '\\') {
			*str_ptr++ = '/';
		}
		/* Copy and increase pointers */
		*str_ptr++ = *str++;
	}
	
	/* Add zero to the end */
	*str_ptr = 0;
	
	/* Return buffer */
	return buff;
}

char* ReverseEscapeString(char* str) {
	static char buff[100];
	char* str_ptr = buff;
	
	/* Go through string */
	while (*str) {
		/* Check for special character */
		if (*str == '/') {
			/* Check for next string after '/' */
			if (*(str + 1) == ',' || *(str + 1) == '"' || *(str + 1) == '\\') {
				/* Ignore '/' */
				str++;
			}
		}
		
		/* Copy and increase pointers */
		*str_ptr++ = *str++;
	}
	
	/* Add zero to the end */
	*str_ptr = 0;
	
	/* Return buffer */
	return buff;
}

static ESP8266_Result_t SendUARTCommand(ESP8266_t* ESP8266, uint32_t baudrate, char* cmd) {
	char command[35];
	
	/* Check idle */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Format command */
	sprintf(command, "%s=%d,8,1,0,0\r\n", cmd, baudrate);
	
	/* Send command */
	if (SendCommand(ESP8266, ESP8266_COMMAND_UART, command, "AT+UART") != ESP_OK) {
		return ESP8266->Result;
	}
	
	/* Wait till command end */
	ESP8266_WaitReady(ESP8266);
	
	/* Check for success */
	if (!ESP8266->Flags.F.LastOperationStatus) {
		ESP8266_RETURNWITHSTATUS(ESP8266, ESP_ERROR);
	}
	
	/* Save baudrate */
	ESP8266->Baudrate = baudrate;
	
	/* Delay a little, wait for all bytes from ESP are received before we delete them from buffer */
	ESP8266_DELAYMS(5);
	
	/* Set new UART baudrate */
	ESP8266_LL_USARTInit(ESP8266->Baudrate);
	
	/* Clear buffer */
	BUFFER_Reset(&USART_Buffer);
	
	/* Delay a little */
	ESP8266_DELAYMS(5);
	
	/* Reset command */
	ESP8266->ActiveCommand = ESP8266_COMMAND_IDLE;
	
	/* Return OK */
	ESP8266_RETURNWITHSTATUS(ESP8266, ESP_OK);
}

static ESP8266_Result_t SendMACCommand(ESP8266_t* ESP8266, uint8_t* addr, char* cmd, uint8_t command) {
	char tmp[40];
	
	/* Check idle */
	ESP8266_CHECK_IDLE(ESP8266);
	
	/* Format string */
	sprintf(tmp, "%s=\"%02x:%02x:%02x:%02x:%02x:%02x\"", cmd, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
	
	/* Send command */
	SendCommand(ESP8266, command, tmp, NULL);
	
	/* Wait ready */
	ESP8266_WaitReady(ESP8266);
	
	/* Check last operation */
	if (ESP8266->Flags.F.LastOperationStatus) {
		/* MAC set was OK, copy MAC address */
		memcpy(command == ESP8266_COMMAND_CIPSTAMAC ? &ESP8266->STAMAC : &ESP8266->APMAC, addr, 6);
	} else {
		/* Check status */
		if (ESP8266->Result == ESP_OK) {
			/* Reset flags */
			if (command == ESP8266_COMMAND_CIPSTAMAC) {
				ESP8266->Flags.F.STAMACIsSet = 0;
			} else {
				ESP8266->Flags.F.APMACIsSet = 0;
			}
		}
	}
	
	/* Return stats */
	return ESP8266->Result;
}

static void CallConnectionCallbacks(ESP8266_t* ESP8266) {
	uint8_t conn_number;
	
	/* Check if there are any pending data to be sent to connection */
	for (conn_number = 0; conn_number < ESP8266_MAX_CONNECTIONS; conn_number++) {
		if (
			ESP8266->ActiveCommand == ESP8266_COMMAND_IDLE &&                                            /*!< Must be IDLE, if we are not because callbacks start command, stop execution */
			ESP8266->Connection[conn_number].Active && ESP8266->Connection[conn_number].CallDataReceived /*!< We must call function for received data */
		) {
			/* Clear flag */
			ESP8266->Connection[conn_number].CallDataReceived = 0;
			
			/* Call user function according to connection type */
			if (ESP8266->Connection[conn_number].Client) {
				/* Client mode */
				ESP8266_Callback_ClientConnectionDataReceived(ESP8266, &ESP8266->Connection[conn_number], ESP8266->Connection[conn_number].Data);
			} else {
				/* Server mode */
				ESP8266_Callback_ServerConnectionDataReceived(ESP8266, &ESP8266->Connection[conn_number], ESP8266->Connection[conn_number].Data);
			}
		}
	}
}

static void ProcessSendData(ESP8266_t* ESP8266) {
	uint16_t found;
	ESP8266_Connection_t* Connection = ESP8266->SendDataConnection;
	
	/* Wrapper was found */
	ESP8266->Flags.F.WaitForWrapper = 0;
	
	/* Go to SENDDATA command as active */
	ESP8266->ActiveCommand = ESP8266_COMMAND_SENDDATA;
	
	/* Get data from user */
	if (Connection->Client) {
		/* Get data as client */
		found = ESP8266_Callback_ClientConnectionSendData(ESP8266, Connection, Connection->Data, 2046);
	} else {
		/* Get data as server */
		found = ESP8266_Callback_ServerConnectionSendData(ESP8266, Connection, Connection->Data, 2046);
	}
	
	/* Check for input data */
	if (found > 2046) {
		found = 2046;
	}
	
	/* If data valid */
	if (found > 0) {
		/* Send data */
		ESP8266_LL_USARTSend((uint8_t *)Connection->Data, found);
		
		/* Increase number of bytes sent */
		ESP8266->TotalBytesSent += found;
	}
		
	/* Send zero at the end even if data are not valid = stop sending data to module */
	ESP8266_LL_USARTSend((uint8_t *)"\\0", 2);
}

/* Check if needle exists in haystack memory */
void* mem_mem(void* haystack, size_t haystacksize, void* needle, size_t needlesize) {
	unsigned char* hptr = (unsigned char *)haystack;
	unsigned char* nptr = (unsigned char *)needle;
	unsigned int i;

	/* Go through entire memory */
	for (i = 0; i < haystacksize; i++) {
		if (memcmp(&hptr[i], nptr, needlesize) == 0) {
			return &hptr[i];
		}
	}

	return 0;
}
