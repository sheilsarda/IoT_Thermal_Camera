/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \main page User Application template doxygen documentation
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
#include <status_codes.h>
#define MAX_RX_BUFFER_LENGTH 1
volatile char rx_buffer[MAX_RX_BUFFER_LENGTH];

struct usart_module usart_instance;

void usart_read_callback(struct usart_module *const usart_module){
	usart_write_buffer_job(&usart_instance, (uint8_t *)rx_buffer, MAX_RX_BUFFER_LENGTH);
}

void usart_write_callback(struct usart_module *const usart_module){
	port_pin_toggle_output_level(LED_0_PIN);
}

void configure_usart(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate    = 9600;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;

	while (usart_init(&usart_instance, EDBG_CDC_MODULE, &config_usart) != STATUS_OK) {
	}	
	usart_enable(&usart_instance);
}
void configure_usart_callbacks(void)
{
	usart_register_callback(&usart_instance,
	usart_write_callback, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
	usart_read_callback, USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
}
/* 
Building on the last example from LAB0, make a program where the user types an ASCII character, 
and the system returns the ASCII numeric character code (and a next line, so we can easily read the response).
*/
int main (void)
{
	system_init();
	configure_usart();
	configure_usart_callbacks();

	/* Insert application code here, after the board has been initialized. */
	system_interrupt_enable_global();

	uint8_t string[] = "Please type a character, I'll convert it to ASCII!\r\n";

	usart_write_buffer_wait(&usart_instance, string, sizeof(string));
	
	while (1) {
		/* Is button pressed? */
		if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) {
			/* Yes, so turn LED on. */
			port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		} else {
			/* No, so turn LED off. */
			port_pin_set_output_level(LED_0_PIN, !LED_0_ACTIVE);
		}

		int i = usart_read_buffer_job(&usart_instance, (char *)rx_buffer, MAX_RX_BUFFER_LENGTH);
		if(i == STATUS_OK && rx_buffer[0] != NULL){
			uint8_t buffer[20]; 
			snprintf(buffer, 20, " - %u\r\n", rx_buffer[0]);
			usart_write_buffer_wait(&usart_instance, buffer, sizeof(buffer));

		}
		
	}
}
