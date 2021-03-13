/**
 * \mainpage SD/MMC Card with FatFs Example
 *
 * 
 *
 */

#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "SerialConsole/SerialConsole.h"
#include "sd_mmc_spi.h"
#include "bootloader.h"

//! Structure for UART module connected to EDBG (used for unit test output)
struct usart_module cdc_uart_module;
static void jumpToApplication(void);
static void configure_nvm(void);
bool fw_update(Status * currentStatus);
bool updateFirmware(FIL file_object, Status * currentStatus);
bool readMetadata(Status * currentStatus);
void updateFlags(Status * currentStatus);
Status * readFlags();
char * meta_ver;
char * meta_crc;

#define APP_START_ADDRESS  ((uint32_t)0xB000) //Must be address of start of main application
#define STRUCT_START_ADDRESS ((uint32_t)(0xAD00))
/// Main application reset vector address
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04)

char metadata_file_name[] = "0:metadata.txt";

FRESULT res;
enum status_code error_code;
Ctrl_status status;
FATFS fs;
FIL file_object;


int main(void)
{

	//INITIALIZE SYSTEM PERIPHERALS
	system_init();
	delay_init();
	InitializeSerialConsole();
	system_interrupt_enable_global();
	/* Initialize SD MMC stack */
	sd_mmc_init();
	configure_nvm();
	
	irq_initialize_vectors();
	cpu_irq_enable();
	Status *  currentStatus;
	
	
	SerialConsoleWriteString("\n\rRadiance_T BOOTLOADER\n\r");	//Order to add string to TX Buffer
	
	currentStatus = readFlags();
	
	//Check SD Card is mounted
			while (1) {
				
				SerialConsoleWriteString("Please plug an SD/MMC card in slot.\n\r");
		
				/* Wait card present and ready */
				do {
					status = sd_mmc_test_unit_ready(0);
					if (CTRL_FAIL == status) {
						SerialConsoleWriteString("Card install FAIL\n\r");
						SerialConsoleWriteString("Please unplug and re-plug the card.\n\r");
						while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
						}
					}
				} while (CTRL_GOOD != status);
		
		
				//Attempt to mount a FAT file system on the SD Card using FATFS
				SerialConsoleWriteString("Mount disk (f_mount)\r\n");
				memset(&fs, 0, sizeof(FATFS));
				res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
				if (FR_INVALID_DRIVE == res) {
					LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
					goto main_end_of_test;
				}
		        
				
				if(port_pin_get_input_level(SW0_PIN) == SW0_ACTIVE){
					// Button PRESSED	
					SerialConsoleWriteString("DETECTED PRESS.\r\n");
					currentStatus = readFlags(); // reading struct Flags stored on NVM
					readMetadata(currentStatus);

					
				} else {
					// Button NOT pressed and no new version found
					currentStatus = readFlags(); // reading struct Flags stored on NVM
					bool updateRequired = readMetadata(currentStatus);
					
					if(!updateRequired){
						goto main_end_of_test;
					}
					
				}
				
				bool success = false;
				do{
					success = fw_update(currentStatus);
				} while(!success);
				
				updateFlags(currentStatus);
						
		
main_end_of_test:
		SerialConsoleWriteString("\r\nProceeding to Application Code\r\n");


		delay_s(1); //Delay to allow text to print
		cpu_irq_disable();

		//Deinitialize HW
		DeinitializeSerialConsole();
		sd_mmc_deinit();
		
		//Jump to application
		jumpToApplication();

		while (CTRL_NO_PRESENT != sd_mmc_check(0)) {
		}
	}
}



//To read firmware flags from NVM
Status * readFlags(){
	uint16_t * bytesRead = (uint16_t *)malloc(sizeof(uint16_t));
	Status * currentStatus = malloc(sizeof(Status));
	do{
		error_code = nvm_read_buffer(STRUCT_START_ADDRESS, currentStatus, (* bytesRead));
	} while (error_code == STATUS_BUSY);
	return currentStatus;
}

//Read metadata.txt (Version name and Precomputed CRC from SD card)
bool readMetadata(Status * currentStatus){
	
	FIL fileobject;
	
	SerialConsoleWriteString("Open metadata file (f_open)\r\n");
	metadata_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)metadata_file_name,
	FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	if (res != FR_OK) {
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
	}
	
	char file_contents[32];
	volatile char *ver_capture = malloc(sizeof(file_contents));
	volatile char *cr_capture = malloc(sizeof(file_contents));
	memset(cr_capture, 0x00, sizeof(cr_capture));
	
	uint8_t i=0x00;
	memset(file_contents,  0x00, sizeof(file_contents));
	while (f_gets(file_contents, 32, &file_object))
	{
		if (i==0x00){
			memcpy(ver_capture,file_contents, sizeof(file_contents));
			i++;
		}
		else{
			memcpy(cr_capture,file_contents, sizeof(file_contents));
		}
	}
	f_close(&file_object);

// 	SerialConsoleWriteString(ver_capture);
// 	SerialConsoleWriteString(cr_capture);
// 	SerialConsoleWriteString("\r\n");
	SerialConsoleWriteString("Read in Metadata [OK]\r\n");
 	
	bool updateRequired = (strcmp(ver_capture,currentStatus->fw_version) != 0)
	|| (strcmp(cr_capture,currentStatus->crc32) != 0);
	
	meta_ver = ver_capture;
	meta_crc = cr_capture;

	return updateRequired;
}

void updateFlags(Status * currentStatus){
	
	FIL fileobject;
	
	SerialConsoleWriteString("Open metadata file (f_open)\r\n");
	metadata_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)metadata_file_name,
	FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	if (res != FR_OK) {
		LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
	}
	
	char file_contents[32];
	volatile char *ver_capture = malloc(sizeof(file_contents));
	volatile char *cr_capture = malloc(sizeof(file_contents));
	memset(cr_capture, 0x00, sizeof(cr_capture));
	
	uint8_t i=0x00;
	memset(file_contents,  0x00, sizeof(file_contents));
	while (f_gets(file_contents, 32, &file_object))
	{
		if (i==0x00){
			memcpy(ver_capture,file_contents, sizeof(file_contents));
			i++;
		}
		else{
			memcpy(cr_capture,file_contents, sizeof(file_contents));
		}
	}
	f_close(&file_object);
		
	memcpy(currentStatus->crc32, cr_capture, sizeof(char) * 32);
	memcpy(currentStatus->fw_version, ver_capture, sizeof(char) * 32);
	int structSize = sizeof(Status);
	
	//Erase the old struct
	do{
		error_code = nvm_erase_row(STRUCT_START_ADDRESS);
	} while (error_code == STATUS_BUSY);

	do{
		error_code = nvm_write_buffer(STRUCT_START_ADDRESS, currentStatus,  sizeof(Status));
	} while (error_code == STATUS_BUSY);

}

static void configure_nvm(void){

	struct nvm_config config_nvm;
	nvm_get_config_defaults(&config_nvm);
	config_nvm.manual_page_write = false;
	nvm_set_config(&config_nvm);
}


// Firmware update
bool fw_update(Status * currentStatus){
	volatile crc32_t sd_crc32  = 0;
	volatile crc32_t nvm_crc32 = 0;

	char app_file_name[] = "0:ApplicationCode.bin";

	app_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
	res = f_open(&file_object, (char const *)app_file_name,
	FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
	
	int fileSize = f_size(&file_object);
	
	char out3[32];
	sprintf(out3, "The size of the firmware file is %d bytes.\r\n", fileSize);
	SerialConsoleWriteString(out3);
	int max_erased_row = 0;
	int max_read_page = 0;
	max_erased_row = (fileSize / NVMCTRL_ROW_SIZE) + 1;
	max_read_page = (fileSize / NVMCTRL_PAGE_SIZE) + 1;
	
	//Erase the old firmware
	for (int eraseCounter = 0; eraseCounter < max_erased_row; eraseCounter++){
		do{
			error_code = nvm_erase_row(APP_START_ADDRESS + ( (NVMCTRL_ROW_PAGES * NVMCTRL_PAGE_SIZE * eraseCounter)));
		} while (error_code == STATUS_BUSY);
	}
	
	
	// write and read function
	uint16_t * bytesRead = (uint16_t *)malloc(sizeof(uint16_t));
	
	char NVM_read_buffer[NVMCTRL_PAGE_SIZE];
	char transferBuffer[NVMCTRL_PAGE_SIZE];
	
	for (int read_counter = 0; read_counter < max_read_page; read_counter++){

		res = f_read(&file_object, transferBuffer, sizeof(transferBuffer), bytesRead);

		if (res == 0){
			if (sd_crc32 == 0)
			{
				crc32_calculate(transferBuffer, (* bytesRead), &sd_crc32);
				}  else {
				crc32_recalculate(transferBuffer, (* bytesRead), &sd_crc32);
			}

			do{
				error_code = nvm_write_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter, transferBuffer, (* bytesRead));
			} while (error_code == STATUS_BUSY);
			

			do{
				error_code = nvm_read_buffer(APP_START_ADDRESS + NVMCTRL_PAGE_SIZE*read_counter, NVM_read_buffer, (* bytesRead));
			} while (error_code == STATUS_BUSY);
			

			if (nvm_crc32 == 0)
			{
				crc32_calculate(transferBuffer,  (* bytesRead), &nvm_crc32);
				} else {
				crc32_recalculate(transferBuffer,  (* bytesRead), &nvm_crc32);
			}
			if(nvm_crc32 != sd_crc32){
				SerialConsoleWriteString("CRC32 are not equal. \r\n");
				delay_s(1);
			}

		}
	}
	f_close(&file_object);

	char * nvm_crc32_text = malloc(sizeof(char) * 32);
	memset(nvm_crc32_text,  0x00, sizeof(nvm_crc32_text));
	sprintf(nvm_crc32_text, "%x", nvm_crc32);

	bool success = true;

	success &= (strcmp(nvm_crc32_text, meta_crc) == 0);
	if(!success){SerialConsoleWriteString("Meta data & NVM Read CRC DO NOT MATCH \r\n");		delay_s(1);}
	
	success &= (nvm_crc32 == sd_crc32);
	if(nvm_crc32 != sd_crc32){SerialConsoleWriteString("SD Card & NVM CRC32 DO NOT MATCH\r\n");		delay_s(1);}
	
	if (success)
	{
		SerialConsoleWriteString("3-way CRC check passed.(Meta Data = SD Card = NVM CRC) \r\n");
		delay_s(1);
		return true;
	}
	return false;
}


/**************************************************************************//**
* function      jumpToApplication()
* @brief        Jumps to main application
* @details      Detailed Description
* @note         Add a note
* @param[in]    arg1 Input parameter description
* @param[out]   arg2 Output parameter description
* @return       Description of return value
******************************************************************************/
static void jumpToApplication(void){
	/// Function pointer to application section
	void (*applicationCodeEntry)(void);

	/// Rebase stack pointer
	__set_MSP(*(uint32_t *) APP_START_ADDRESS);

	/// Rebase vector table
	SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

	/// Set pointer to application section
	applicationCodeEntry =
	(void (*)(void))(unsigned *)(*(unsigned *)(APP_START_RESET_VEC_ADDRESS));

	/// Jump to application
	applicationCodeEntry();
}

