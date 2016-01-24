/**
 * Keil project example for ESP8266 module
 *
 * Before you start, select your target, on the right of the "Load" button
 *
 * In this example, when you press button on discovery/nucleo board,
 * ESP goes to stm32f4-discovery and reads content of web page.
 *
 * @author    Tilen Majerle
 * @email     tilen@majerle.eu
 * @website   http://stm32f4-discovery.com
 * @ide       Keil uVision 5
 * @conf      PLL parameters are set in "Options for Target" -> "C/C++" -> "Defines"
 * @packs     STM32F4xx/STM32F7xx Keil packs are requred with HAL driver support
 * @stdperiph STM32F4xx/STM32F7xx HAL drivers required
 */
/* Include core modules */
#include "stm32fxxx_hal.h"
/* Include my libraries here */
#include "defines.h"
#include "tm_stm32_disco.h"
#include "tm_stm32_delay.h"
#include "tm_stm32_usart.h"
#include "esp8266.h"

/* ESP8266 working structure */
ESP8266_t ESP8266;

int main(void) {
	/* Init system */
	TM_RCC_InitSystem();
	
	/* Init HAL layer */
	HAL_Init();
	
	/* Init leds */
	TM_DISCO_LedInit();
	
	/* Init button */
	TM_DISCO_ButtonInit();
	
	/* Init delay */
	TM_DELAY_Init();
	
	/* Init debug USART */
	TM_USART_Init(USART2, TM_USART_PinsPack_1, 921600);
	
	/* Display message */
	printf("ESP8266 AT commands parser\r\n");
	
	/* Init ESP module */
	while (ESP8266_Init(&ESP8266, 115200) != ESP_OK) {
		printf("Problems with initializing module!\r\n");
	}
	
	/* Set mode to STA+AP */
	while (ESP8266_SetMode(&ESP8266, ESP8266_Mode_STA_AP) != ESP_OK);
	
	/* Enable server on port 80 */
	while (ESP8266_ServerEnable(&ESP8266, 80) != ESP_OK);
	
	/* Module is connected OK */
	printf("Initialization finished!\r\n");
	
	/* Disconnect from wifi if connected */
	ESP8266_WifiDisconnect(&ESP8266);
	
	/* Get a list of all stations  */
	ESP8266_ListWifiStations(&ESP8266);
	
	/* Wait till finishes */
	ESP8266_WaitReady(&ESP8266);
	
	/* Connect to wifi and save settings */
	ESP8266_WifiConnect(&ESP8266, "your SSID", "your password");
	
	/* Wait till finish */
	ESP8266_WaitReady(&ESP8266);
	
	/* Get connected devices */
	ESP8266_WifiGetConnected(&ESP8266);
	
	while (1) {
		/* Update ESP module */
		ESP8266_Update(&ESP8266);
		
		/* Check for button */
		if (TM_DISCO_ButtonOnPressed()) {
			/* Starting with connection to web */
			while (ESP8266_StartClientConnection(&ESP8266, "stm32f4_discovery", "stm32f4-discovery.com", 80, NULL));
		}
	}
}

/* 1ms handler */
void TM_DELAY_1msHandler() {	
	/* Update ESP8266 library time for 1 ms */
	ESP8266_TimeUpdate(&ESP8266, 1);
}

/* printf handler */
int fputc(int ch, FILE* fil) {
	/* Send over debug USART */
	TM_USART_Putc(USART2, ch);
	
	/* Return OK */
	return ch;
}

/* Called when ready string detected */
void ESP8266_Callback_DeviceReady(ESP8266_t* ESP8266) {
	printf("Device is ready\r\n");
}

/* Called when watchdog reset on ESP8266 is detected */
void ESP8266_Callback_WatchdogReset(ESP8266_t* ESP8266) {
	printf("Watchdog reset detected!\r\n");
}

/* Called when we are disconnected from WIFI */
void ESP8266_Callback_WifiDisconnected(ESP8266_t* ESP8266) {
	printf("Wifi is disconnected!\r\n");
}

void ESP8266_Callback_WifiConnected(ESP8266_t* ESP8266) {
	printf("Wifi is connected!\r\n");
}

void ESP8266_Callback_WifiConnectFailed(ESP8266_t* ESP8266) {
	printf("Connection to wifi network has failed. Reason %d\r\n", ESP8266->WifiConnectError);
}

void ESP8266_Callback_WifiGotIP(ESP8266_t* ESP8266) {
	printf("Wifi got an IP address\r\n");
	
	/* Read that IP from module */
	printf("Grabbing IP status: %d\r\n", ESP8266_GetSTAIP(ESP8266));
}

void ESP8266_Callback_WifiIPSet(ESP8266_t* ESP8266) {
	/* We have STA IP set (IP set by router we are connected to) */
	printf("We have valid IP address: %d.%d.%d.%d\r\n", ESP8266->STAIP[0], ESP8266->STAIP[1], ESP8266->STAIP[2], ESP8266->STAIP[3]);
}

void ESP8266_Callback_DHCPTimeout(ESP8266_t* ESP8266) {
	printf("DHCP timeout!\r\n");
}

void ESP8266_Callback_WifiDetected(ESP8266_t* ESP8266, ESP8266_APs_t* ESP8266_AP) {
	uint8_t i = 0;
	
	/* Print number of detected stations */
	printf("We have detected %d AP stations\r\n", ESP8266_AP->Count);
	
	/* Print each AP */
	for (i = 0; i < ESP8266_AP->Count; i++) {
		/* Print SSID for each AP */
		printf("%2d: %s\r\n", i, ESP8266_AP->AP[i].SSID);
	}
}

/************************************/
/*         CLIENT CALLBACKS         */
/************************************/
void ESP8266_Callback_ClientConnectionConnected(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* We are connected to external server */
	printf("Client connected to server! Connection number: %s\r\n", Connection->Name);
	
	/* We are connected to server, request to sent header data to server */
	ESP8266_RequestSendData(ESP8266, Connection);
}

/* Called when client connection fails to server */
void ESP8266_Callback_ClientConnectionError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	/* Fail with connection to server */
	printf("An error occured when trying to connect on connection: %d\r\n", Connection->Number);
}

/* Called when data are ready to be sent to server */
uint16_t ESP8266_Callback_ClientConnectionSendData(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer, uint16_t max_buffer_size) {
	/* Format data to sent to server */
	sprintf(Buffer, "GET / HTTP/1.1\r\n");
	strcat(Buffer, "Host: stm32f4-discovery.com\r\n");
	strcat(Buffer, "Connection: close\r\n");
	strcat(Buffer, "\r\n");
	
	/* Return length of buffer */
	return strlen(Buffer);
}

/* Called when data are send successfully */
void ESP8266_Callback_ClientConnectionDataSent(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Data successfully sent as client!\r\n");
}

/* Called when error returned trying to sent data */
void ESP8266_Callback_ClientConnectionDataSentError(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Error while sending data on connection %d!\r\n", Connection->Number);
}

uint32_t time = 0;
void ESP8266_Callback_ClientConnectionDataReceived(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection, char* Buffer) {
	/* Data received from server back to client */
	printf("Data received from server on connection: %s; Number of bytes received: %d; %d / %d;\r\n",
		Connection->Name,
		Connection->BytesReceived,
		Connection->TotalBytesReceived,
		Connection->ContentLength
	);
	
	/* Print message when first packet */
	if (Connection->FirstPacket) {
		/* Start counting time */
		time = TM_DELAY_Time();
		
		/* Print first message */
		printf("This is first packet received. Content length on this connection is: %d\r\n", Connection->ContentLength);
	}
}

/* Called when connection is closed */
void ESP8266_Callback_ClientConnectionClosed(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Client connection closed, connection: %d; Total bytes received: %d; Content-Length header: %d\r\n",
		Connection->Number, Connection->TotalBytesReceived, Connection->ContentLength
	);
	
	/* Calculate time */
	time = TM_DELAY_Time() - time;
	
	/* Print time we need to get data back from server */
	printf("Time for data: %u ms; speed: %d kb/s\r\n", time, Connection->TotalBytesReceived / time);
}

/* Called when timeout is reached on connection to server */
void ESP8266_Callback_ClientConnectionTimeout(ESP8266_t* ESP8266, ESP8266_Connection_t* Connection) {
	printf("Timeout reached on connection: %d\r\n", Connection->Number);
}
