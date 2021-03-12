/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * This is a bare minimum user application template.
 *
 * For documentation of the board, go \ref group_common_boards "here" for a link
 * to the board-specific documentation.
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# Basic usage of on-board LED and button
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include "SerialConsole/SerialConsole.h"

#define ver_bl "1.1.2 \r\n"
#define ver_app "1.1.2 \r\n"
#define MAX_RX_BUFFER_LENGTH   5
volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];

//volatile char buffer[50];


void matchCommand(){
	char buffer[200];
	int len = 0;
	
	bool enterFound = false;
	bool deviceNameFound = false;
	
	while (!rxbufferIsEmpty() && !enterFound)
	{
		char c = getNextRxChar();
		if(c == 0x0D) {// enter
			enterFound = !enterFound;
			break;
		} else if (c == 0x08){// backspace
			len--;
			continue;
		} else if (c == 0x20){// space
			char trimmedString[len+1];
			getTrimmedString(buffer, trimmedString, len);
			if(strcmp(trimmedString, "setDeviceName") == 0){
				len = 0;
				deviceNameFound = !deviceNameFound;
				continue;
			}
		}
		
		buffer[len++] = c;
		
	}
	char trimmedString[len+1];
	
	getTrimmedString(buffer, trimmedString, len);
	
	// New Line for output of CLI
	//SerialConsoleWriteString("\n");

	if(deviceNameFound){
		//SerialConsoleWriteString("\n");
		setDeviceName(trimmedString);
		return;
	}		
	if(strcmp(trimmedString, "help") == 0){
		helpFunction();
	} else if(strcmp(trimmedString, "ver_bl") == 0){
		
		SerialConsoleWriteString(ver_bl);
	
	} else if(strcmp(trimmedString, "ver_app") == 0){
		
		SerialConsoleWriteString(ver_app);			
		
	} else if(strcmp(trimmedString, "mac") == 0){
		
		SerialConsoleWriteString("00:00:00:00:00:00 \r\n");
		
	} else if(strcmp(trimmedString, "ip") == 0){
		
		SerialConsoleWriteString("255.255.255.255 \r\n");
		
	} else if(strcmp(trimmedString, "devName") == 0){
		
		SerialConsoleWriteString("sheils \r\n");
		
	} else if(strcmp(trimmedString, "getDeviceName") == 0){
		
		getDeviceName();
		
	} else {
		SerialConsoleWriteString("\r\nError\r\n");
	}

	
}


int main (void)
{
	//Board Initialization -- Code that initializes the HW and happens only once
	system_init();
	InitializeSerialConsole();

	/* Insert application code here, after the board has been initialized. */


	system_interrupt_enable_global();
	
	SerialConsoleWriteString("ESE516 - CLI and Debug Logger\r\n");	//Order to add string to TX Buffer
	
	char string[] = "CLI starter code - ESE516\r\n";
	
	/*Simple DebugLogger Test*/
	setLogLevel(LOG_INFO_LVL); 
//	LogMessage(LOG_INFO_LVL , "%s", string); //Test
	setLogLevel(LOG_ERROR_LVL); //Sets the Debug Logger to only allow messages with LOG_ERROR_LVL or higher to be printed
//	LogMessage(LOG_INFO_LVL, "Performing Temperature Test…\r\n"); //This should NOT print
//	LogMessage(LOG_FATAL_LVL,"Error! Temperature over %d Degrees!\r\n", 55); //This should print


	/* This skeleton code simply sets the LED to the state of the button. */
	while (1) {

		/* Is button pressed? */
		if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) {
			/* Yes, so turn LED on. */
			port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		} else {
			/* No, so turn LED off. */
			port_pin_set_output_level(LED_0_PIN, !LED_0_ACTIVE);
		}
		
		//At the very end of the system, we tell the MCU to handle the text that is currently on the RX buffer
		//Put a call to your state machine code that will handle the CLI by reading the data
		//present on the RX buffer.
		
		if (enterSeen == 1)
		{
			enterSeen = 0;
			matchCommand();
		}

	}
}


