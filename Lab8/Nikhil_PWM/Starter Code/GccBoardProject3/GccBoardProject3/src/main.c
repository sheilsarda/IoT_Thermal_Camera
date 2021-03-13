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






#define MAX_RX_BUFFER_LENGTH   5
volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];

volatile char buffer[7];


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
	LogMessage(LOG_INFO_LVL , "%s", string); //Test
	setLogLevel(LOG_ERROR_LVL); //Sets the Debug Logger to only allow messages with LOG_ERROR_LVL or higher to be printed
	LogMessage(LOG_INFO_LVL, "Performing Temperature Test…\r\n"); //This should NOT print
	LogMessage(LOG_FATAL_LVL,"Error! Temperature over %d Degrees!\r\n", 55); //This should print


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

	}
}
