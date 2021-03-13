#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"
#include "main.h"

/* Structure for USART module instance */
struct usart_module console_instance;

uint8_t timeout=0;
volatile offset = 0;
// used for the conversion of user input pin to int for use in processing
int pin_value[10];


#define DATA_LENGTH 256
static uint16_t buffer[DATA_LENGTH];
//
// static uint8_t buffer_reversed[DATA_LENGTH];

#define SLAVE_ADDRESS 0x69

struct i2c_master_module i2c_master_instance;
struct i2c_master_packet packet;

void configure_i2c(void)
{
	/* Initialize config structure and software module */
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	config_i2c_master.pinmux_pad0       = PINMUX_PA08C_SERCOM0_PAD0;
	config_i2c_master.pinmux_pad1       = PINMUX_PA09C_SERCOM0_PAD1;

	/* Change buffer timeout to something longer */
	config_i2c_master.buffer_timeout = 65535;
	/* Initialize and enable device with config */
	while(i2c_master_init(&i2c_master_instance, SERCOM0, &config_i2c_master) != STATUS_OK);
	
	i2c_master_enable(&i2c_master_instance);
}


/** UART module for debug. */
static struct usart_module cdc_uart_module;


/**
* \brief Configure UART console.
*/
static void configure_console(void)
{
	struct usart_config usart_conf;
	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 115200;

	stdio_serial_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
	usart_enable(&cdc_uart_module);
}

void i2c_write_complete_callback(struct i2c_master_module *const module)
{
	
}


void configure_i2c_callbacks(void)
{
	/* Register callback function. */
	i2c_master_register_callback(&i2c_master_instance, i2c_write_complete_callback,
	I2C_MASTER_CALLBACK_WRITE_COMPLETE);
	i2c_master_enable_callback(&i2c_master_instance,
	I2C_MASTER_CALLBACK_WRITE_COMPLETE);
}

void printImage(){
	printf("----------START----------");
	for(int loop = 64; loop < 64+64; loop++){
		if (loop%8 == 0)
		{
			printf("\r\n");
		}
		
 		printf("%d ", buffer[loop]);		
	}
	
	printf("\r\n");
	printf("-------------END--------");
	printf("\r\n");
	
}


int main(void)
{
	/* Initialize the board. */
	system_init();
	system_interrupt_enable_global();
	delay_init();
	
	packet.address = SLAVE_ADDRESS;
	packet.data_length = DATA_LENGTH;
	packet.data = buffer;
	
	/* Initialize the UART console. */
	configure_console();

	/* Configure device and enable. */
	configure_i2c();
	/* Configure callbacks and enable. */
	configure_i2c_callbacks();
	
	while (1) {
		i2c_master_read_packet_job(&i2c_master_instance, &packet);
		printImage();
		delay_ms(100);
	}
	return 0;
}

