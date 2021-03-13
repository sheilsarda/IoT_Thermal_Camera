#include <errno.h>
#include "asf.h"
#include "main.h"
#include "stdio_serial.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
#include "iot/http/http_client.h"
#include "MQTTClient/Wrapper/mqtt.h"
#include "SerialConsole/SerialConsole.h"

#define STRUCT_START_ADDRESS ((uint32_t)(0xAD00))

#define STRING_EOL                      "\r\n"
#define STRING_HEADER                   "-- HTTP file downloader example --"STRING_EOL \
"-- "BOARD_NAME " --"STRING_EOL	\
"-- Compiled: "__DATE__ " "__TIME__ " --"STRING_EOL


#define CONF_PWM_MODULE      TCC0
#define CONF_PWM_CHANNEL     2
#define CONF_PWM_OUTPUT      2
#define CONF_PWM_OUT_PIN     PIN_PA10F_TCC0_WO2
#define CONF_PWM_OUT_MUX     MUX_PA10F_TCC0_WO2
#define ADC_TEMP_SAMPLE_LENGTH	4

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

uint32_t currentIP = 0x0000;
volatile char mqtt_msg [64]= "{\"d\":{\"temp\":17}}\"";
volatile char loc_msg [256]= "{\"d\":{\"loc\":{\"lat\":20,\"long\":40}}}\"";
volatile char img_msg [256];
volatile char batt_msg [64]= "{\"d\":{\"batt\":17}}\"";
volatile char servo_msg [128]= "{\"d\":{\"servo\":17}}\"";
volatile char fw_msg [64]= "true";

volatile bool updateReady = false;

char * crc32_string;
uint32_t ver = 0x00;
char metadata_file_name[] = "0:metadata.txt";

volatile uint32_t dummyData = 1;
Status * currentStatus;
enum status_code error_code;




/*HTTP DOWNLOAD RELATED DEFINES AND VARIABLES*/

uint8_t do_download_flag = false; //Flag that when true initializes a download. False to connect to MQTT broker
/** File download processing state. */
static download_state down_state = NOT_READY;
/** SD/MMC mount. */
static FATFS fatfs;
/** File pointer for file download. */
static FIL file_object;
/** Http content length. */
static uint32_t http_file_size = 0;
/** Receiving content length. */
static uint32_t received_file_size = 0;
/** File name to download. */
static char save_file_name[MAIN_MAX_FILE_NAME_LENGTH + 1] = "0:";

// LED state variable
bool ledOn = false;

/** UART module for debug. */
static struct usart_module cdc_uart_module;

/** Instance of Timer module. */
struct sw_timer_module swt_module_inst;

/** Instance of HTTP client module. */
struct http_client_module http_client_module_inst;

/*MQTT RELATED DEFINES AND VARIABLES*/

/** User name of chat. */
char mqtt_user[64] = "Unit1";

/* Instance of MQTT service. */
static struct mqtt_module mqtt_inst;

/* Receive buffer of the MQTT service. */
static unsigned char mqtt_read_buffer[MAIN_MQTT_BUFFER_SIZE];
static unsigned char mqtt_send_buffer[MAIN_MQTT_BUFFER_SIZE];

#define DATA_LENGTH 256
static uint16_t i2c_buffer[DATA_LENGTH];

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
		
		printf("%d ", i2c_buffer[loop]);
	}
	
	printf("\r\n");
	printf("-------------END--------");
	printf("\r\n");
	
}



static void configure_tcc(uint32_t newVal, uint32_t period)
{
	struct tcc_config config_tcc;
	tcc_get_config_defaults(&config_tcc, CONF_PWM_MODULE);
	config_tcc.counter.period = period;
	config_tcc.compare.wave_generation = TCC_WAVE_GENERATION_SINGLE_SLOPE_PWM;
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
	conf_adc.positive_input = ADC_POSITIVE_INPUT_PIN0;

	adc_init(&adc_instance, ADC, &conf_adc);
	ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(2) | ADC_AVGCTRL_SAMPLENUM_4;
	adc_enable(&adc_instance);
	raw_result = adc_start_read_result();
	
	float scaled_angle = raw_result/22.75;
	snprintf(servo_msg, 63, "{\"d\":{\"servo\":%d}}", scaled_angle);
	mqtt_publish(&mqtt_inst, SERVO_TOPIC, servo_msg, strlen(servo_msg), 2, 0);
	printf("\r\n%s\r\n",servo_msg);	
	
}



/*HTPP RELATED STATIOC FUNCTIONS*/

/**
* \brief Initialize download state to not ready.
*/
static void init_state(void)
{
	down_state = NOT_READY;
}

/**
* \brief Clear state parameter at download processing state.
* \param[in] mask Check download_state.
*/
static void clear_state(download_state mask)
{
	down_state &= ~mask;
}

/**
* \brief Add state parameter at download processing state.
* \param[in] mask Check download_state.
*/
static void add_state(download_state mask)
{
	down_state |= mask;
}

/**
* \brief File download processing state check.
* \param[in] mask Check download_state.
* \return true if this state is set, false otherwise.
*/

static inline bool is_state_set(download_state mask)
{
	return ((down_state & mask) != 0);
}

/**
* \brief File existing check.
* \param[in] fp The file pointer to check.
* \param[in] file_path_name The file name to check.
* \return true if this file name is exist, false otherwise.
*/
static bool is_exist_file(FIL *fp, const char *file_path_name)
{
	if (fp == NULL || file_path_name == NULL) {
		return false;
	}

	FRESULT ret = f_open(&file_object, (char const *)file_path_name, FA_OPEN_EXISTING);
	f_close(&file_object);
	return (ret == FR_OK);
}

/**
* \brief Make to unique file name.
* \param[in] fp The file pointer to check.
* \param[out] file_path_name The file name change to uniquely and changed name is returned to this buffer.
* \param[in] max_len Maximum file name length.
* \return true if this file name is unique, false otherwise.
*/
static bool rename_to_unique(FIL *fp, char *file_path_name, uint8_t max_len)
{
	#define NUMBRING_MAX (3)
	#define ADDITION_SIZE (NUMBRING_MAX + 1) /* '-' character is added before the number. */
	uint16_t i = 1, name_len = 0, ext_len = 0, count = 0;
	char name[MAIN_MAX_FILE_NAME_LENGTH + 1] = {0};
	char ext[MAIN_MAX_FILE_EXT_LENGTH + 1] = {0};
	char numbering[NUMBRING_MAX + 1] = {0};
	char *p = NULL;
	bool valid_ext = false;

	if (file_path_name == NULL) {
		return false;
	}

	if (!is_exist_file(fp, file_path_name)) {
		return true;
	}
	else if (strlen(file_path_name) > MAIN_MAX_FILE_NAME_LENGTH) {
		return false;
	}

	p = strrchr(file_path_name, '.');
	if (p != NULL) {
		ext_len = strlen(p);
		if (ext_len < MAIN_MAX_FILE_EXT_LENGTH) {
			valid_ext = true;
			strcpy(ext, p);
			if (strlen(file_path_name) - ext_len > MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE) {
				name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE - ext_len;
				strncpy(name, file_path_name, name_len);
			}
			else {
				name_len = (p - file_path_name);
				strncpy(name, file_path_name, name_len);
			}
		}
		else {
			name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
			strncpy(name, file_path_name, name_len);
		}
	}
	else {
		name_len = MAIN_MAX_FILE_NAME_LENGTH - ADDITION_SIZE;
		strncpy(name, file_path_name, name_len);
	}

	name[name_len++] = '-';

	for (i = 0, count = 1; i < NUMBRING_MAX; i++) {
		count *= 10;
	}
	for (i = 1; i < count; i++) {
		sprintf(numbering, MAIN_ZERO_FMT(NUMBRING_MAX), i);
		strncpy(&name[name_len], numbering, NUMBRING_MAX);
		if (valid_ext) {
			strcpy(&name[name_len + NUMBRING_MAX], ext);
		}

		if (!is_exist_file(fp, name)) {
			memset(file_path_name, 0, max_len);
			strcpy(file_path_name, name);
			return true;
		}
	}
	return false;
}

/**
* \brief Start file download via HTTP connection.
*/
static void start_download(void)
{
	if (!is_state_set(STORAGE_READY)) {
		printf("start_download: MMC storage not ready.\r\n");
		return;
	}

	if (!is_state_set(WIFI_CONNECTED)) {
		printf("start_download: Wi-Fi is not connected.\r\n");
		return;
	}

	if (is_state_set(GET_REQUESTED)) {
		printf("start_download: request is sent already.\r\n");
		return;
	}

	if (is_state_set(DOWNLOADING)) {
		printf("start_download: running download already.\r\n");
		return;
	}
	

	/* Send the HTTP request. */
	printf("start_download: sending HTTP request...\r\n");
	http_client_send_request(&http_client_module_inst, file_url, HTTP_METHOD_GET, NULL, NULL);
}

/**
* \brief Store received packet to file.
* \param[in] data Packet data.
* \param[in] length Packet data length.
*/
static void store_file_packet(char *data, uint32_t length)
{
	FRESULT ret;
	if ((data == NULL) || (length < 1)) {
		printf("store_file_packet: empty data.\r\n");
		return;
	}

	if (!is_state_set(DOWNLOADING)) {
		char *cp = NULL;
		save_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
		save_file_name[1] = ':';
		cp = (char *)(file_url + strlen(file_url));
		while (*cp != '/') {
			cp--;
		}
		if (strlen(cp) > 1) {
			cp++;
			strcpy(&save_file_name[2], cp);
			} else {
			printf("store_file_packet: file name is invalid. Download canceled.\r\n");
			add_state(CANCELED);
			return;
		}

		rename_to_unique(&file_object, save_file_name, MAIN_MAX_FILE_NAME_LENGTH);
		printf("store_file_packet: creating file [%s]\r\n", save_file_name);
		ret = f_open(&file_object, (char const *)save_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (ret != FR_OK) {
			printf("store_file_packet: file creation error! ret:%d\r\n", ret);
			return;
		}

		received_file_size = 0;
		add_state(DOWNLOADING);
	}

	if (data != NULL) {
		UINT wsize = 0;
		ret = f_write(&file_object, (const void *)data, length, &wsize);
		if (ret != FR_OK) {
			f_close(&file_object);
			add_state(CANCELED);
			printf("store_file_packet: file write error, download canceled.\r\n");
			return;
		}

		received_file_size += wsize;
		printf("store_file_packet: received[%lu], file size[%lu]\r\n", (unsigned long)received_file_size, (unsigned long)http_file_size);
		if (received_file_size >= http_file_size) {
			f_close(&file_object);
			printf("store_file_packet: file downloaded successfully.\r\n");
			port_pin_set_output_level(LED_0_PIN, false);
			add_state(COMPLETED);
			
			return;
		}
	}
}

/**
* \brief Callback of the HTTP client.
*
* \param[in]  module_inst     Module instance of HTTP client module.
* \param[in]  type            Type of event.
* \param[in]  data            Data structure of the event. \refer http_client_data
*/
static void http_client_callback(struct http_client_module *module_inst, int type, union http_client_data *data)
{
	switch (type) {
		case HTTP_CLIENT_CALLBACK_SOCK_CONNECTED:
		printf("http_client_callback: HTTP client socket connected.\r\n");
		break;

		case HTTP_CLIENT_CALLBACK_REQUESTED:
		printf("http_client_callback: request completed.\r\n");
		add_state(GET_REQUESTED);
		break;

		case HTTP_CLIENT_CALLBACK_RECV_RESPONSE:
		printf("http_client_callback: received response %u data size %u\r\n",
		(unsigned int)data->recv_response.response_code,
		(unsigned int)data->recv_response.content_length);
		if ((unsigned int)data->recv_response.response_code == 200) {
			http_file_size = data->recv_response.content_length;
			received_file_size = 0;
		}
		else {
			add_state(CANCELED);
			return;
		}
		if (data->recv_response.content_length <= MAIN_BUFFER_MAX_SIZE) {
			store_file_packet(data->recv_response.content, data->recv_response.content_length);
			add_state(COMPLETED);
		}
		break;

		case HTTP_CLIENT_CALLBACK_RECV_CHUNKED_DATA:
		store_file_packet(data->recv_chunked_data.data, data->recv_chunked_data.length);
		if (data->recv_chunked_data.is_complete) {
			add_state(COMPLETED);
		}

		break;

		case HTTP_CLIENT_CALLBACK_DISCONNECTED:
		printf("http_client_callback: disconnection reason:%d\r\n", data->disconnected.reason);

		/* If disconnect reason is equal to -ECONNRESET(-104),
		* It means the server has closed the connection (timeout).
		* This is normal operation.
		*/
		if (data->disconnected.reason == -EAGAIN) {
			/* Server has not responded. Retry immediately. */
			if (is_state_set(DOWNLOADING)) {
				f_close(&file_object);
				clear_state(DOWNLOADING);
			}

			if (is_state_set(GET_REQUESTED)) {
				clear_state(GET_REQUESTED);
			}

			start_download();
		}

		break;
	}
}

/**
* \brief Callback to get the data from socket.
*
* \param[in] sock socket handler.
* \param[in] u8Msg socket event type. Possible values are:
*  - SOCKET_MSG_BIND
*  - SOCKET_MSG_LISTEN
*  - SOCKET_MSG_ACCEPT
*  - SOCKET_MSG_CONNECT
*  - SOCKET_MSG_RECV
*  - SOCKET_MSG_SEND
*  - SOCKET_MSG_SENDTO
*  - SOCKET_MSG_RECVFROM
* \param[in] pvMsg is a pointer to message structure. Existing types are:
*  - tstrSocketBindMsg
*  - tstrSocketListenMsg
*  - tstrSocketAcceptMsg
*  - tstrSocketConnectMsg
*  - tstrSocketRecvMsg
*/
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	http_client_socket_event_handler(sock, u8Msg, pvMsg);
}

/**
* \brief Callback for the gethostbyname function (DNS Resolution callback).
* \param[in] pu8DomainName Domain name of the host.
* \param[in] u32ServerIP Server IPv4 address encoded in NW byte order format. If it is Zero, then the DNS resolution failed.
*/
static void resolve_cb(uint8_t *pu8DomainName, uint32_t u32ServerIP)
{
	printf("resolve_cb: %s IP address is %d.%d.%d.%d\r\n\r\n", pu8DomainName,
	(int)IPV4_BYTE(u32ServerIP, 0), (int)IPV4_BYTE(u32ServerIP, 1),
	(int)IPV4_BYTE(u32ServerIP, 2), (int)IPV4_BYTE(u32ServerIP, 3));
	


	http_client_socket_resolve_handler(pu8DomainName, u32ServerIP);
}

/**
* \brief Callback to get the Wi-Fi status update.
*
* \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
*  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
*  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
*  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
*  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
*  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
*  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
*  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
*  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
*  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
*  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
*  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
* \param[in] pvMsg A pointer to a buffer containing the notification parameters
* (if any). It should be casted to the correct data type corresponding to the
* notification type. Existing types are:
*  - tstrM2mWifiStateChanged
*  - tstrM2MWPSInfo
*  - tstrM2MP2pResp
*  - tstrM2MAPResp
*  - tstrM2mScanDone
*  - tstrM2mWifiscanResult
*/
static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType) {
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
		{
			tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
			if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
				printf("wifi_cb: M2M_WIFI_CONNECTED\r\n");
				m2m_wifi_request_dhcp_client();
				} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
				printf("wifi_cb: M2M_WIFI_DISCONNECTED\r\n");
				clear_state(WIFI_CONNECTED);
				if (is_state_set(DOWNLOADING)) {
					f_close(&file_object);
					clear_state(DOWNLOADING);
				}

				if (is_state_set(GET_REQUESTED)) {
					clear_state(GET_REQUESTED);
				}


				/* Disconnect from MQTT broker. */
				/* Force close the MQTT connection, because cannot send a disconnect message to the broker when network is broken. */
				mqtt_disconnect(&mqtt_inst, 1);

				m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID),
				MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
			}

			break;
		}

		case M2M_WIFI_REQ_DHCP_CONF:
		{
			uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
			printf("wifi_cb: IP address is %u.%u.%u.%u\r\n",
			pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
			add_state(WIFI_CONNECTED);
			currentIP = pu8IPAddress;
			printf("Demo %d.%d.%d.%d\r\n\r\n",
			(int)IPV4_BYTE(currentIP, 0), (int)IPV4_BYTE(currentIP, 1),
			(int)IPV4_BYTE(currentIP, 2), (int)IPV4_BYTE(currentIP, 3));
			/* Try to connect to MQTT broker when Wi-Fi was connected. */
			if (mqtt_connect(&mqtt_inst, main_mqtt_broker))
			{
				printf("Error connecting to MQTT Broker!\r\n");
			}
		}
		break;
		

		default:
		break;
	}
}

/**
* \brief Initialize SD/MMC storage.
*/
static void init_storage(void)
{
	FRESULT res;
	Ctrl_status status;

	/* Initialize SD/MMC stack. */
	sd_mmc_init();
	while (true) {
		printf("init_storage: please plug an SD/MMC card in slot...\r\n");

		/* Wait card present and ready. */
		do {
			status = sd_mmc_test_unit_ready(0);
			if (CTRL_FAIL == status) {
				printf("init_storage: SD Card install failed.\r\n");
				printf("init_storage: try unplug and re-plug the card.\r\n");
				while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
				}
			}
		} while (CTRL_GOOD != status);

		printf("init_storage: mounting SD card...\r\n");
		memset(&fatfs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fatfs);
		if (FR_INVALID_DRIVE == res) {
			printf("init_storage: SD card mount failed! (res %d)\r\n", res);
			return;
		}

		printf("init_storage: SD card mount OK.\r\n");
		add_state(STORAGE_READY);
		return;
	}
}

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

static void deconfigure_console(void)
{

	usart_disable(&cdc_uart_module);
}


/**
* \brief Configure Timer module.
*/
static void configure_timer(void)
{
	struct sw_timer_config swt_conf;
	sw_timer_get_config_defaults(&swt_conf);

	sw_timer_init(&swt_module_inst, &swt_conf);
	sw_timer_enable(&swt_module_inst);
}

/**
* \brief Configure HTTP client module.
*/
static void configure_http_client(void)
{
	struct http_client_config httpc_conf;
	int ret;


	http_client_get_config_defaults(&httpc_conf);

	httpc_conf.recv_buffer_size = MAIN_BUFFER_MAX_SIZE;
	httpc_conf.timer_inst = &swt_module_inst;
	httpc_conf.port = 443;
	httpc_conf.tls = 1;
	
	ret = http_client_init(&http_client_module_inst, &httpc_conf);
	if (ret < 0) {
		printf("configure_http_client: HTTP client initialization failed! (res %d)\r\n", ret);
		while (1) {
			} /* Loop forever. */
		}

		http_client_register_callback(&http_client_module_inst, http_client_callback);
		
	}

	/*MQTT RELATED STATIC FUNCTIONS*/

	/** Prototype for MQTT subscribe Callback */
	void SubscribeHandler(MessageData *msgData);

	void downloadFW();


	/**
	* \brief Callback to get the Socket event.
	*
	* \param[in] Socket descriptor.
	* \param[in] msg_type type of Socket notification. Possible types are:
	*  - [SOCKET_MSG_CONNECT](@ref SOCKET_MSG_CONNECT)
	*  - [SOCKET_MSG_BIND](@ref SOCKET_MSG_BIND)
	*  - [SOCKET_MSG_LISTEN](@ref SOCKET_MSG_LISTEN)
	*  - [SOCKET_MSG_ACCEPT](@ref SOCKET_MSG_ACCEPT)
	*  - [SOCKET_MSG_RECV](@ref SOCKET_MSG_RECV)
	*  - [SOCKET_MSG_SEND](@ref SOCKET_MSG_SEND)
	*  - [SOCKET_MSG_SENDTO](@ref SOCKET_MSG_SENDTO)
	*  - [SOCKET_MSG_RECVFROM](@ref SOCKET_MSG_RECVFROM)
	* \param[in] msg_data A structure contains notification informations.
	*/
	static void socket_event_handler(SOCKET sock, uint8_t msg_type, void *msg_data)
	{
		mqtt_socket_event_handler(sock, msg_type, msg_data);
	}


	/**
	* \brief Callback of gethostbyname function.
	*
	* \param[in] doamin_name Domain name.
	* \param[in] server_ip IP of server.
	*/
	static void socket_resolve_handler(uint8_t *doamin_name, uint32_t server_ip)
	{
		mqtt_socket_resolve_handler(doamin_name, server_ip);
	}


	/**
	* \brief Callback to receive the subscribed Message.
	*
	* \param[in] msgData Data to be received.
	*/

	void SubscribeHandler(MessageData *msgData)
	{
		/* You received publish message which you had subscribed. */
		/* Print Topic and message */
		printf("\r\n %.*s",msgData->topicName->lenstring.len,msgData->topicName->lenstring.data);
		printf(" >> ");
		printf("%.*s\r\n",msgData->message->payloadlen,(char *)msgData->message->payload);
		
		//Handle LedData message
		
		if(strncmp((char *) msgData->topicName->lenstring.data, CRC_TOPIC, msgData->message->payloadlen) == 0)
		{
			crc32_string = malloc(msgData->message->payloadlen);
			strncpy(crc32_string,(char *)msgData->message->payload, msgData->message->payloadlen);
			
		}
		if(strncmp((char *) msgData->topicName->lenstring.data, VER_TOPIC, msgData->message->payloadlen) == 0)
		{
			
			char * ver_string = malloc(msgData->message->payloadlen);
			strncpy(ver_string, msgData->message->payload, msgData->message->payloadlen);
			ver = atoi(ver_string);
			
		}
		
		//Handle LedData message
		if(strncmp((char *) msgData->topicName->lenstring.data, LED_TOPIC, msgData->message->payloadlen) == 0)
		{
			if(strncmp((char *)msgData->message->payload, LED_TOPIC_LED_OFF, msgData->message->payloadlen) == 0)
			{
				port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
				port_pin_set_output_level(PIN_PA10, LED_0_INACTIVE);
				ledOn = false;
			}
			else if (strncmp((char *)msgData->message->payload, LED_TOPIC_LED_ON, msgData->message->payloadlen) == 0)
			{
				port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
				port_pin_set_output_level(PIN_PA10, LED_0_ACTIVE);
				ledOn = true;
			}
		}
		
		//Handle LedData message
		if(strncmp((char *) msgData->topicName->lenstring.data, STOP_TOPIC, msgData->message->payloadlen) == 0)
		{
			if(strncmp((char *)msgData->message->payload, LED_TOPIC_LED_OFF, msgData->message->payloadlen) == 0)
			{
				for (int i = 5; i >= 0; i--)
				{
					long newVal = 0xFFFFF/(2*10.922) * 0.5 * (1+i);
					tcc_disable(&tcc_instance);
					configure_tcc((uint32_t) newVal,  0xFFFFF/1.0921);
					delay_ms(500);
					process_adc_get('A','2');

				}
			}
			else if (strncmp((char *)msgData->message->payload, LED_TOPIC_LED_ON, msgData->message->payloadlen) == 0)
			{
				for (int i = 0; i <= 5; i++)
				{
					long newVal = 0xFFFFF/(2*10.922) * 0.5 * (1+i);
					tcc_disable(&tcc_instance);
					configure_tcc((uint32_t) newVal,  0xFFFFF/1.0921);
					delay_ms(100);
					process_adc_get('A','2');

				}
			}
			i2c_master_read_packet_job(&i2c_master_instance, &packet);
			printImage();
		}
		
		
		//Handle FWData message
		if(strncmp((char *) msgData->topicName->lenstring.data, FW_TOPIC, msgData->message->payloadlen) == 0)
		{
			if (strncmp((char *)msgData->message->payload, LED_TOPIC_LED_ON, msgData->message->payloadlen) == 0)
			{
				updateReady = true;
				
				f_unlink("metadata.txt");

				FIL fileobject;
				FRESULT res;

				SerialConsoleWriteString("Open metadata file (f_open)\r\n");
				metadata_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
				res = f_open(&file_object, (char const *)metadata_file_name,
				FA_CREATE_ALWAYS | FA_READ | FA_WRITE);
				if (res != FR_OK) {
					LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
				}


				char * toWrite = malloc(sizeof(char) * 32);
				memset(toWrite,  0x00, sizeof(toWrite));
				sprintf(toWrite, "%u\r\n43e32c64", ver, crc32_string);
				
				printf("%s\r\n", toWrite);
				
				SerialConsoleWriteString("Write to test file (f_puts)...\r\n");
				if (0 == f_puts(toWrite, &file_object)) {
					f_close(&file_object);
					LogMessage(LOG_INFO_LVL ,"[FAIL]\r\n");
					
				}
				
				SerialConsoleWriteString("[OK]\r\n");
				f_close(&file_object); //Close file

			}
		}
		

	}
	
	


	/**
	* \brief Callback to get the MQTT status update.
	*
	* \param[in] conn_id instance id of connection which is being used.
	* \param[in] type type of MQTT notification. Possible types are:
	*  - [MQTT_CALLBACK_SOCK_CONNECTED](@ref MQTT_CALLBACK_SOCK_CONNECTED)
	*  - [MQTT_CALLBACK_CONNECTED](@ref MQTT_CALLBACK_CONNECTED)
	*  - [MQTT_CALLBACK_PUBLISHED](@ref MQTT_CALLBACK_PUBLISHED)
	*  - [MQTT_CALLBACK_SUBSCRIBED](@ref MQTT_CALLBACK_SUBSCRIBED)
	*  - [MQTT_CALLBACK_UNSUBSCRIBED](@ref MQTT_CALLBACK_UNSUBSCRIBED)
	*  - [MQTT_CALLBACK_DISCONNECTED](@ref MQTT_CALLBACK_DISCONNECTED)
	*  - [MQTT_CALLBACK_RECV_PUBLISH](@ref MQTT_CALLBACK_RECV_PUBLISH)
	* \param[in] data A structure contains notification informations. @ref mqtt_data
	*/
	static void mqtt_callback(struct mqtt_module *module_inst, int type, union mqtt_data *data)
	{
		switch (type) {
			case MQTT_CALLBACK_SOCK_CONNECTED:
			{
				/*
				* If connecting to broker server is complete successfully, Start sending CONNECT message of MQTT.
				* Or else retry to connect to broker server.
				*/
				if (data->sock_connected.result >= 0) {
					printf("\r\nConnecting to Broker...");
					if(0 != mqtt_connect_broker(module_inst, 1, CLOUDMQTT_USER_ID, CLOUDMQTT_USER_PASSWORD, CLOUDMQTT_USER_ID, NULL, NULL, 0, 0, 0))
					{
						printf("MQTT  Error - NOT Connected to broker\r\n");
					}
					else
					{
						printf("MQTT Connected to broker\r\n");
					}
					} else {
					printf("Connect fail to server(%s)! retry it automatically.\r\n", main_mqtt_broker);
					mqtt_connect(module_inst, main_mqtt_broker); /* Retry that. */
				}
			}
			break;

			case MQTT_CALLBACK_CONNECTED:
			if (data->connected.result == MQTT_CONN_RESULT_ACCEPT) {
				/* Subscribe chat topic. */
				mqtt_subscribe(module_inst, ANGLE_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, STOP_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, VER_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, CRC_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, FW_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, TEMPERATURE_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, LED_TOPIC, 2, SubscribeHandler);

				mqtt_subscribe(module_inst, IMAGE_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, SERVO_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, LOCATION_TOPIC, 2, SubscribeHandler);
				mqtt_subscribe(module_inst, BATTERY_TOPIC, 2, SubscribeHandler);

				/* Enable USART receiving callback. */
				
				printf("MQTT Connected\r\n");
				} else {
				/* Cannot connect for some reason. */
				printf("MQTT broker decline your access! error code %d\r\n", data->connected.result);
			}

			break;

			case MQTT_CALLBACK_DISCONNECTED:
			/* Stop timer and USART callback. */
			printf("MQTT disconnected\r\n");
			usart_disable_callback(&cdc_uart_module, USART_CALLBACK_BUFFER_RECEIVED);
			break;
		}
	}



	/**
	* \brief Configure MQTT service.
	*/
	static void configure_mqtt(void)
	{
		struct mqtt_config mqtt_conf;
		int result;

		mqtt_get_config_defaults(&mqtt_conf);
		/* To use the MQTT service, it is necessary to always set the buffer and the timer. */
		mqtt_conf.read_buffer = mqtt_read_buffer;
		mqtt_conf.read_buffer_size = MAIN_MQTT_BUFFER_SIZE;
		mqtt_conf.send_buffer = mqtt_send_buffer;
		mqtt_conf.send_buffer_size = MAIN_MQTT_BUFFER_SIZE;
		mqtt_conf.port = CLOUDMQTT_PORT;
		mqtt_conf.keep_alive = 6000;
		
		result = mqtt_init(&mqtt_inst, &mqtt_conf);
		if (result < 0) {
			printf("MQTT initialization failed. Error code is (%d)\r\n", result);
			while (1) {
			}
		}

		result = mqtt_register_callback(&mqtt_inst, mqtt_callback);
		if (result < 0) {
			printf("MQTT register callback failed. Error code is (%d)\r\n", result);
			while (1) {
			}
		}
	}

	//SETUP FOR EXTERNAL BUTTON INTERRUPT -- Used to send an MQTT Message

	void configure_extint_channel(void)
	{
		struct extint_chan_conf config_extint_chan;
		extint_chan_get_config_defaults(&config_extint_chan);
		config_extint_chan.gpio_pin           = BUTTON_0_EIC_PIN;
		config_extint_chan.gpio_pin_mux       = BUTTON_0_EIC_MUX;
		config_extint_chan.gpio_pin_pull      = EXTINT_PULL_UP;
		config_extint_chan.detection_criteria = EXTINT_DETECT_FALLING;
		extint_chan_set_config(BUTTON_0_EIC_LINE, &config_extint_chan);
	}

	void extint_detection_callback(void);
	void configure_extint_callbacks(void)
	{
		extint_register_callback(extint_detection_callback,
		BUTTON_0_EIC_LINE,
		EXTINT_CALLBACK_TYPE_DETECT);
		extint_chan_enable_callback(BUTTON_0_EIC_LINE,
		EXTINT_CALLBACK_TYPE_DETECT);
	}


	volatile bool isPressed = false;

	void extint_detection_callback(void)
	{
		//Publish some data after a button press and release. Note: just an example! This is not the most elegant way of doing this!
		dummyData++;
		if (dummyData > 40) dummyData = 1;
		snprintf(mqtt_msg, 63, "{\"d\":{\"temp\":%d}}", dummyData);
		//snprintf(loc_msg, 63, "{\"d\":{\"loc\":{\"lat\":%d,\"long\":%d}}}", dummyData*3, dummyData*2);
	
		isPressed = true;
		
		
	}

	void matchCommand(){
		char buffer[200];
		int len = 0;
		
		bool enterFound = false;
		bool deviceNameFound = false;
		bool imgFound = false;
		bool locationFound = false;
		bool tempFound = false;
		bool battFound = false;
		bool fwFound = false;
		bool servoFound = false;
		
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
				
				if(strcmp(trimmedString, "LocationData") == 0){
					len = 0;
					locationFound = !locationFound;
					continue;
				}
				
				if(strcmp(trimmedString, "TempData") == 0){
					len = 0;
					tempFound = !tempFound;
					continue;
				}
				
				if(strcmp(trimmedString, "BatteryData") == 0){
					len = 0;
					battFound = !battFound;
					continue;
				}
				
				if(strcmp(trimmedString, "StopData") == 0){
					mqtt_publish(&mqtt_inst, STOP_TOPIC, fw_msg, strlen(fw_msg), 2, 0);
					return;
				}
				
				if(strcmp(trimmedString, "FWData") == 0){
					mqtt_publish(&mqtt_inst, FW_TOPIC, fw_msg, strlen(fw_msg), 2, 0);
					return;
					
				}
				if(strcmp(trimmedString, "LedData") == 0){
					mqtt_publish(&mqtt_inst, LED_TOPIC, fw_msg, strlen(fw_msg), 2, 0);
					return;
				}
				if(strcmp(trimmedString, "ImageData") == 0){
					len = 0;
					imgFound = !imgFound;
					continue;
				}
				if(strcmp(trimmedString, "ServoData") == 0){
					len = 0;
					servoFound = !servoFound;
					continue;
				}
				
				
			}
			
			buffer[len++] = c;
			
		}
		char trimmedString[len+1];
		
		getTrimmedString(buffer, trimmedString, len);

		if(deviceNameFound){
			setDeviceName(trimmedString);
			return;
			} else if(locationFound){

			// want to transmit location data
			const char comma = ',';
			
			char * first = strchr(trimmedString, comma);

			uint8_t lat_len = (uint8) first - (uint8) trimmedString;
			
			uint8_t long_len = strlen(trimmedString) - lat_len;
			
			char latitude[lat_len];
			char longitude[long_len];

			strncpy(latitude, trimmedString, lat_len);
			
			strncpy(longitude, first + 1, long_len);
			
			snprintf(loc_msg, 255, "{\"d\":{\"loc\":{\"lat\":%ld,\"long\":%ld}}}", atol(latitude), atol(longitude));
			mqtt_publish(&mqtt_inst, LOCATION_TOPIC, loc_msg, strlen(loc_msg), 2, 0);
			printf("\r\n%s\r\n",loc_msg);

			} else if(imgFound){
			snprintf(img_msg, 255, "{\"d\":{\"img\":{\"one\":%ld,\"two\":%ld,\"three\":%ld,\"four\":%ld}}}", atoi(trimmedString),atoi(trimmedString),atoi(trimmedString),atoi(trimmedString));
			mqtt_publish(&mqtt_inst, IMAGE_TOPIC, img_msg, strlen(img_msg), 2, 0);
			printf("\r\n%s\r\n",img_msg);
			
			}else if(tempFound){
			snprintf(mqtt_msg, 63, "{\"d\":{\"temp\":%d}}", atoi(trimmedString));
			
			mqtt_publish(&mqtt_inst, TEMPERATURE_TOPIC, mqtt_msg, strlen(mqtt_msg), 2, 0);
		}
		else if(battFound){
			snprintf(batt_msg, 63, "{\"d\":{\"batt\":%d}}", atoi(trimmedString));

			mqtt_publish(&mqtt_inst, BATTERY_TOPIC, batt_msg, strlen(batt_msg), 2, 0);
		}
		else if(servoFound){
			snprintf(servo_msg, 63, "{\"d\":{\"servo\":%d}}", atoi(trimmedString));
			mqtt_publish(&mqtt_inst, SERVO_TOPIC, servo_msg, strlen(servo_msg), 2, 0);
		}

		else if(strcmp(trimmedString, "help") == 0){
			helpFunction();
			} else if(strcmp(trimmedString, "ver_bl") == 0){
			
			SerialConsoleWriteString(ver_bl);
			
			} else if(strcmp(trimmedString, "ver_app") == 0){
			SerialConsoleWriteString("\r\n");
			SerialConsoleWriteString(currentStatus->fw_version);
			
			} else if(strcmp(trimmedString, "mac") == 0){
			
			SerialConsoleWriteString("\r\nF8:F0:05:F3:F8:F2\r\n");
			
			} else if(strcmp(trimmedString, "ip") == 0){
			char ip_addr [64];

			snprintf(ip_addr, 64, "\r\n%d.%d.%d.%d\r\n",
			(int)IPV4_BYTE(currentIP, 0), (int)IPV4_BYTE(currentIP, 1),
			(int)IPV4_BYTE(currentIP, 2), (int)IPV4_BYTE(currentIP, 3));
			SerialConsoleWriteString(ip_addr);
			
			} else if(strcmp(trimmedString, "devName") == 0){
			
			SerialConsoleWriteString("\r\nRadiance_T \r\n");
			
			} else if(strcmp(trimmedString, "getDeviceName") == 0){
			SerialConsoleWriteString("\r\n");
			getDeviceName();
			
		}
		
		else {
			SerialConsoleWriteString("\r\nError\r\n");
		}
		delay_ms(100);
	}
	extern int enterSeen = 0;

	static void configure_nvm(void){

		struct nvm_config config_nvm;
		nvm_get_config_defaults(&config_nvm);
		config_nvm.manual_page_write = false;
		nvm_set_config(&config_nvm);
	}

	//To read firmware flags from NVM
	Status * readFWVersion(){
		SerialConsoleWriteString("reading version\r\n");
		Status * currentStatus = malloc(sizeof(Status));
		memset(currentStatus, 0x03, sizeof(Status));
		do{
			error_code = nvm_read_buffer(STRUCT_START_ADDRESS, currentStatus, sizeof(Status));
		} while (error_code == STATUS_BUSY);


		return currentStatus;
	}


	
	/**
	* \brief Main application function.
	*
	* Application entry point.
	*
	* \return program return value.
	*/
	int main(void)
	{
		
		
		tstrWifiInitParam param;
		int8_t ret;
		init_state();
		
		/* Initialize the board. */
		system_init();
		configure_nvm();
		
		configure_tcc(0xFFFFF/(2*10.922) * 0.5, 0xFFFFF/1.0921); // starting at 0.5ms width

		delay_init();
		
		/* Initialize the UART console. */
		configure_console();
		printf(STRING_HEADER);
		printf("\r\nThis example requires the AP to have Internet access.\r\n\r\n");

		/* Initialize the Timer. */
// TEMPORARILY COMMENTING THIS OUT> ADD BACK ASAP
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// 		configure_timer();

		/* Initialize the HTTP client service. */
		configure_http_client();

		/* Initialize the MQTT service. */
		configure_mqtt();

		/* Initialize the BSP. */
		nm_bsp_init();

		/* Initialize SD/MMC storage. */
// TEMPORARILY COMMENTING THIS OUT> ADD BACK ASAP
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//		init_storage();

		/*Initialize BUTTON 0 as an external interrupt*/
		configure_extint_channel();
		configure_extint_callbacks();
		system_interrupt_enable_global();

		/* Initialize Wi-Fi parameters structure. */
		memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

		/* Initialize Wi-Fi driver with data and status callbacks. */
		param.pfAppWifiCb = wifi_cb;
		ret = m2m_wifi_init(&param);
		if (M2M_SUCCESS != ret) {
			printf("main: m2m_wifi_init call error! (res %d)\r\n", ret);
			while (1) {
			}
		}

// TEMPORARILY COMMENTING THIS OUT> ADD BACK ASAP
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		if (SysTick_Config(system_cpu_clock_get_hz() / 1000))
		{
			puts("ERR>> Systick configuration error\r\n");
			while (1);
		}
		
		socketInit();
		registerSocketCallback(socket_event_handler, socket_resolve_handler);
		
		/* Connect to router. */
		printf("main: connecting to WiFi AP %s...\r\n", (char *)MAIN_WLAN_SSID);
		m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		
		printf("main: please unplug the SD/MMC card.\r\n");
		printf("main: done.\r\n");

		delay_s(1);
		
		deconfigure_console();
		InitializeSerialConsole();
		
		
		packet.address = SLAVE_ADDRESS;
		packet.data_length = DATA_LENGTH;
		packet.data = i2c_buffer;
		
		/* Configure device and enable. */
		configure_i2c();
		/* Configure callbacks and enable. */
		configure_i2c_callbacks();
		
// TEMPORARILY COMMENTING THIS OUT> ADD BACK ASAP
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
//		currentStatus = readFWVersion();

		volatile int count = 0;
				
		delay_ms(300);
		
		while (1) {		
			
			/* Handle pending events from network controller. */
			m2m_wifi_handle_events(NULL);
			
			if(isPressed)
			{
				//Publish updated temperature data
				mqtt_publish(&mqtt_inst, TEMPERATURE_TOPIC, mqtt_msg, strlen(mqtt_msg), 2, 0);
				isPressed = false;
				delay_ms(200);
			}

			if (enterSeen == 1)
			{
				enterSeen = 0;
				matchCommand();
			}

			//Handle MQTT messages
			if(mqtt_inst.isConnected) mqtt_yield(&mqtt_inst, 100);
			
			
			if(updateReady){
				
				printf("Detected update \r\n");
				
				f_unlink("ApplicationCode.bin");
				
				/* Force close the MQTT connection, because cannot
				send a disconnect message to the broker when network is broken. */
				mqtt_disconnect(&mqtt_inst, 1);

				delay_s(1);
				
				do_download_flag = true;
				
				/* Initialize socket module. */
				registerSocketCallback(socket_cb, resolve_cb);
				
				start_download();

				while (!(is_state_set(COMPLETED) || is_state_set(CANCELED))) {
					/* Handle pending events from network controller. */
					m2m_wifi_handle_events(NULL);
					/* Checks the timer timeout. */
					sw_timer_task(&swt_module_inst);
				}
				
				if(!is_state_set(DOWNLOADING) && is_state_set(GET_REQUESTED) && is_state_set(WIFI_CONNECTED)){
					
				}
				
				do_download_flag = false;

				registerSocketCallback(socket_event_handler, socket_resolve_handler);
				
				/* Connect to router. */
				if (mqtt_connect(&mqtt_inst, main_mqtt_broker)){
					printf("Error connecting to MQTT Broker!\r\n");
				}
				printf("Finished update  \r\n");
				updateReady = !updateReady;
				delay_s(1);
				printf("Finished update \r\n");
				
				NVIC_SystemReset();
			}
			

		}

		return 0;
	}
