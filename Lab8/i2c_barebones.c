#include "asf.h"
#include "driver/include/m2m_wifi.h"
#include "driver/source/nmasic.h"

/* Structure for USART module instance */
struct usart_module console_instance;

uint8_t timeout=0;
volatile offset = 0;
// used for the conversion of user input pin to int for use in processing
int pin_value[10];


/// I2C

#define DATA_LENGTH 8
static uint8_t wr_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
};
static uint8_t rd_buffer[DATA_LENGTH];

#define SLAVE_ADDRESS (0x52>>1)
/// 0x52 0x53 0x29

struct i2c_master_module i2c_master_instance;

static struct i2c_master_packet wr_packet = {
	.address          = SLAVE_ADDRESS,
	.data_length      = DATA_LENGTH,
	.data             = wr_buffer,
	.ten_bit_address  = false,
	.high_speed       = false,
	.hs_master_code   = 0x00,
};

static struct i2c_master_packet rd_packet = {
	.address          = SLAVE_ADDRESS,
	.data_length      = DATA_LENGTH,
	.data             = rd_buffer,
	.ten_bit_address  = false,
	.high_speed       = false,
	.hs_master_code   = 0x00,
};

void configure_i2c(void)
{
	/* Initialize config structure and software module */
	struct i2c_master_config config_i2c_master;
	i2c_master_get_config_defaults(&config_i2c_master);
	/* Change buffer timeout to something longer */
	config_i2c_master.buffer_timeout    = 65535;
	config_i2c_master.pinmux_pad0       = PINMUX_PA08D_SERCOM2_PAD0;
	config_i2c_master.pinmux_pad1       = PINMUX_PA09D_SERCOM2_PAD1;
	config_i2c_master.generator_source  = GCLK_GENERATOR_0;
	/* Initialize and enable device with config */
	while(i2c_master_init(&i2c_master_instance, SERCOM2, &config_i2c_master) != STATUS_OK);
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

	/// stdio_serial_init does usart_init + registers getchar and putchar, used by printf -- comment out to end up in Dummy Handler
	stdio_serial_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
	usart_enable(&cdc_uart_module);
}



void configure_pin()
{
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	
	// set pin A17 & A23 (onboard LED) as output pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PIN_PA17, &config_port_pin);
	port_pin_set_config(LED_0_PIN, &config_port_pin);
	
	// set pin A16 & A19 as input pins
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PIN_PA16, &config_port_pin);
	port_pin_set_config(PIN_PA19, &config_port_pin);
}


void processCommand(void)
{
	
	int8_t found = 0;
	
	
	enum status_code i2c_status;
	wr_packet.address     = SLAVE_ADDRESS;
	wr_packet.data_length = 1;
	wr_buffer[0]          = 0xC0;
	wr_packet.data        = wr_buffer;
	bool flag=1;
	while(flag)
	{
		while((i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet))==STATUS_OK)
		{
			printf("slave address:0X%x",wr_packet.address);
			flag=0;
			offset=0;
			break;
		}
		timeout++;
		if(timeout==200)
		{
			timeout=0;
			offset++;
			wr_packet.address = SLAVE_ADDRESS+offset;
		}
	}
	found=1;
	

	/// If we don't find any commands, print an error
	if (!found) printf("ERROR: Unknown command\r\n");
	
	/// Space things out a bit
	printf("\r\n\r\n");
	
}


int main(void)
{

	/* Initialize the board. */
	system_init();
	system_interrupt_enable_global();

	

	/* Initialize the UART console. */
	configure_console();
	
	
	/* Initialize I2C */
	configure_i2c();
	
	enum status_code i2c_status;
	wr_packet.address     = SLAVE_ADDRESS;
	wr_packet.data_length = 1;
	wr_buffer[0]          = 0xC0;
	wr_packet.data        = wr_buffer;
	i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet);
	if( i2c_status == STATUS_OK ){ i2c_status = i2c_master_read_packet_wait(&i2c_master_instance, &rd_packet); }
	i2c_master_send_stop(&i2c_master_instance);
	
	wr_buffer[0]          = 0xC1;
	wr_packet.data        = wr_buffer;
	i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet);
	if( i2c_status == STATUS_OK ){ i2c_status = i2c_master_read_packet_wait(&i2c_master_instance, &rd_packet); }
	i2c_master_send_stop(&i2c_master_instance);
	
	wr_buffer[0]          = 0xC2;
	wr_packet.data        = wr_buffer;
	i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet);
	if( i2c_status == STATUS_OK ){ i2c_status = i2c_master_read_packet_wait(&i2c_master_instance, &rd_packet); }
	i2c_master_send_stop(&i2c_master_instance);
	
	wr_buffer[0]          = 0x51;
	wr_packet.data        = wr_buffer;
	i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet);
	if( i2c_status == STATUS_OK ){ i2c_status = i2c_master_read_packet_wait(&i2c_master_instance, &rd_packet); }
	i2c_master_send_stop(&i2c_master_instance);
	
	wr_buffer[0]          = 0x61;
	wr_packet.data        = wr_buffer;
	i2c_status = i2c_master_write_packet_wait_no_stop(&i2c_master_instance, &wr_packet);
	if( i2c_status == STATUS_OK ){ i2c_status = i2c_master_read_packet_wait(&i2c_master_instance, &rd_packet); }
	i2c_master_send_stop(&i2c_master_instance);
	
	
	configure_pin();
	
	while (1) {

	}
	return 0;
}

