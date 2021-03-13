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
#include "config/conf_example.h"
#include "SerialConsole/SerialConsole.h"
#include "adc_configure.h"
#include "adc_temp.h"



#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     2
#define CONF_PWM_OUTPUT      2
#define CONF_PWM_OUT_PIN     PIN_PA10F_TCC0_WO2
#define CONF_PWM_OUT_MUX     MUX_PA10F_TCC0_WO2
#define MAX_RX_BUFFER_LENGTH   5

volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];

volatile char buffer[7];
struct tcc_module tcc_instance;
struct tcc_config config_tcc;

/* Structure for ADC module instance */
struct adc_module adc_instance;

/* Structure for USART module instance */
struct usart_module console_instance;

/*  To Store ADC output in voltage format */
float result;

/* To store raw_result of ADC output */
uint16_t raw_result;

/* To read the STATUS register of ADC */
uint8_t status;

// used for the conversion of user input pin to int for use in processing
int pin_value[10];

static void configure_tcc(uint32_t newVal, uint32_t period)
{
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.period = period;
	config_tcc.compare.wave_generation =
	TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
	config_tcc.compare.match[CONF_PWM_CHANNEL] = newVal;
	config_tcc.pins.enable_wave_out_pin[CONF_PWM_OUTPUT] = true;
	config_tcc.pins.wave_out_pin[CONF_PWM_OUTPUT] =
	CONF_PWM_OUT_PIN;
	config_tcc.pins.wave_out_pin_mux[CONF_PWM_OUTPUT] =
	CONF_PWM_OUT_MUX;
	tcc_init(&tcc_instance, CONF_PWM_MODULE, &config_tcc);
	tcc_enable(&tcc_instance);
}



/**
* \brief ADC START and Read Result.
*
* This function starts the ADC and wait for the ADC
* conversation to be complete	and read the ADC result
* register and return the same to calling function.
*/
/* Structure for ADC module instance */
struct adc_module adc_instance;
uint16_t adc_start_read_result(void)
{
	uint16_t adc_result = 0;
	
	adc_start_conversion(&adc_instance);
	while((adc_get_status(&adc_instance) & ADC_STATUS_RESULT_READY) != 1);
	
	adc_read(&adc_instance, &adc_result);
	
	return adc_result;
}


void process_adc_get(char port, int pin)
{
	int valid_flag = 0;
	
	// need to first set pin as input PIN_PA02, PIN_PA03
	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(PIN_PA02, &config_port_pin);
	port_pin_set_config(PIN_PA03, &config_port_pin);
	
	
	struct adc_config conf_adc;
	
	adc_get_config_defaults(&conf_adc);
	
	conf_adc.clock_source = GCLK_GENERATOR_1;
	conf_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV16;
	conf_adc.reference = ADC_REFERENCE_INTVCC1;
	conf_adc.negative_input = ADC_NEGATIVE_INPUT_GND;
	conf_adc.sample_length = ADC_TEMP_SAMPLE_LENGTH;
	
// 	if (port == 'A' && pin == 2)
// 	{
		conf_adc.positive_input = ADC_POSITIVE_INPUT_PIN0; //go to implementation: this pin is designated as AIN[0], which is PA02. check section 5.1 multiplexing section of manual.
		valid_flag = 1;
// 	}
// 	else if (port == 'A' && pin == 3)
// 	{
// 		conf_adc.positive_input = ADC_POSITIVE_INPUT_PIN1; //AIN[1] is PA03.
// 		valid_flag = 1;
// 	}
	
	if (valid_flag == 1)
	{
		adc_init(&adc_instance, ADC, &conf_adc);
		ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(2) | ADC_AVGCTRL_SAMPLENUM_4;
		adc_enable(&adc_instance);
		raw_result = adc_start_read_result();
		
		char stringBuffer [256];
		snprintf(stringBuffer, sizeof(stringBuffer), "\r\nADC conversion: %d \r\n", raw_result);
		SerialConsoleWriteString(stringBuffer);

	}
	else
	{
		SerialConsoleWriteString("Port & Pin combination not available for 'adc_get'. Please choose A2 or A3.\r\n");
		valid_flag = 0;
	}

}


int main (void)
{
	//Board Initialization -- Code that initializes the HW and happens only once
	system_init();
	InitializeSerialConsole();
	configure_tcc(0xFFFFF/(2*10.922) * 0.5, 0xFFFFF/1.0921); // starting at 0.5ms width
	SerialConsoleWriteString("ESE516 - CLI and Debug Logger\r\n");	//Order to add string to TX Buffer
	delay_init();
	char string[] = "CLI starter code - ESE516\r\n";
	
	/*Simple DebugLogger Test*/
	setLogLevel(LOG_INFO_LVL); 
	LogMessage(LOG_INFO_LVL , "%s", string); //Test
	setLogLevel(LOG_ERROR_LVL); //Sets the Debug Logger to only allow messages with LOG_ERROR_LVL or higher to be printed
	LogMessage(LOG_INFO_LVL, "Performing Temperature Test?r\n"); //This should NOT print
	LogMessage(LOG_FATAL_LVL,"Error! Temperature over %d Degrees!\r\n", 55); //This should print

	int pwmState = 0;
	/* This skeleton code simply sets the LED to the state of the button. */
	while (1) {

		/* Is button pressed? */
		if (port_pin_get_input_level(BUTTON_0_PIN) == BUTTON_0_ACTIVE) {
			/* Yes, so turn LED on. */

			
			delay_ms(1000);
			if (pwmState == 0)
			{
				for (int i = 0; i < 6; i++)
				{
					long newVal = 0xFFFFF/(2*10.922) * 0.5 * (1+i);
					tcc_disable(&tcc_instance);
// 					delay_ms(150);
					configure_tcc((uint32_t) newVal,  0xFFFFF/1.0921);
 					delay_ms(100);
					process_adc_get('A','2');

				}
				pwmState = 1;
			} else {
				for (int i = 5; i >= 0; i--)
				{
					long newVal = 0xFFFFF/(2*10.922) * 0.5 * (1+i);
					tcc_disable(&tcc_instance);
//  					delay_ms(150);
					configure_tcc((uint32_t) newVal,  0xFFFFF/1.0921);
					delay_ms(500);
					process_adc_get('A','2');

				}
				pwmState = 0;
			}

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