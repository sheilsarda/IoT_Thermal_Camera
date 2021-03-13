/**
 * \file
 *
 * \brief SD/MMC card example with FatFs
 *
 * Copyright (c) 2014-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

/**
 * \mainpage SD/MMC Card with FatFs Example
 *
 * \section Purpose
 *
 * This example shows how to implement the SD/MMC stack with the FatFS.
 * It will mount the file system and write a file in the card.
 *
 * The example outputs the information through the standard output (stdio).
 * To know the output used on the board, look in the conf_example.h file
 * and connect a terminal to the correct port.
 *
 * While using Xplained Pro evaluation kits, please attach I/O1 Xplained Pro
 * extension board to EXT1.
 *
 * \section Usage
 *
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application.
 * Refert to conf_example.h file.
 * -# Start the application.
 * -# In the terminal window, the following text should appear:
 *    \code
 *     -- SD/MMC Card Example on FatFs --
 *     -- Compiled: xxx xx xxxx xx:xx:xx --
 *     Please plug an SD, MMC card in slot.
 *    \endcode
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include <asf.h>
#include "conf_example.h"
#include <string.h>
#include "SerialConsole/SerialConsole.h"
#include "sd_mmc_spi.h"

//! Structure for UART module connected to EDBG (used for unit test output)
struct usart_module cdc_uart_module;
static void jumpToApplication(void);
static void configure_nvm(void);
void nvm_function(void);

#define APP_START_ADDRESS  ((uint32_t)0xB000) //Must be address of start of main application

/// Main application reset vector address
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS+(uint32_t)0x04)
FRESULT res;
enum status_code error_code;

Ctrl_status status;
FATFS fs;
FIL file_object;

/**
 * \brief Application entry point.
 *
 * \return Unused (ANSI-C compatibility).
 */
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
	
	
	SerialConsoleWriteString("ESE516 - BOOTLOADER");	//Order to add string to TX Buffer

	SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

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
		SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
		memset(&fs, 0, sizeof(FATFS));
		res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);
		if (FR_INVALID_DRIVE == res) {
			LogMessage(LOG_INFO_LVL ,"[FAIL] res %d\r\n", res);
			goto main_end_of_test;
		}
		SerialConsoleWriteString("[OK]\r\n");


		
		nvm_function();
main_end_of_test:
		SerialConsoleWriteString("Please unplug the card.\n\r");


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

static void configure_nvm(void){

	struct nvm_config config_nvm;
	nvm_get_config_defaults(&config_nvm);
	config_nvm.manual_page_write = false;
	nvm_set_config(&config_nvm);
}


void nvm_function(void){
	volatile crc32_t sd_crc32  = 0;
	volatile crc32_t nvm_crc32 = 0;

		char dummy_info[NVMCTRL_PAGE_SIZE];
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
				if(nvm_crc32 == sd_crc32){
					SerialConsoleWriteString("CRC32 Do equal. \r\n");
					if(memcmp(NVM_read_buffer, transferBuffer, sizeof(transferBuffer)) == 0){
						SerialConsoleWriteString("Arrays are equal \r\n");
					}
					delay_s(1);
				}

			}
		}
		f_close(&file_object);	

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

