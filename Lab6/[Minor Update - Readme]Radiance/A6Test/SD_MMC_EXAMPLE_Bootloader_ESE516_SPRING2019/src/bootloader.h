/*
 * bootloader.h
 *
 * Created: 4/5/2019 5:33:11 PM
 *  Author: sheil
 */ 


#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

typedef struct Status{
	char fw_version[32];
	char crc32[32];
	uint8_t flag;
} Status;



#endif /* BOOTLOADER_H_ */