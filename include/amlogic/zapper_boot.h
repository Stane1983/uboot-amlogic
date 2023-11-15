#ifndef _ZAPPER_BOOT_H_
#define _ZAPPER_BOOT_H_

#include <amlogic/zapper_boot_common.h>

enum ADC_KEY_TYPE {
	NO_ADC_KEY_PRESS = 0,
	ADC_KEY_A_PRESS,
	ADC_KEY_B_PRESS,
	ADC_KEY_C_PRESS,
	UNKNOWN_KEY_PRESS,
	ADC_DEVICE_ERROR,
};

enum JUMP_RECOVERY_TYPE {
	NO_NEED_JUMP = 0,
	USB_DETECT_JUMP,
	OTA_DETECT_JUMP,
	BOOT_CHECK_JUMP,
	UNKNOWN_JUMP,
};



struct Zapper_boot_info {
	unsigned char loader_partition_header[LD_HEADER_LENGTH];
	unsigned char loader_partition[LD_LENGTH];
	unsigned char error_code[EC_LENGTH];
	unsigned char modify_flag;
	unsigned char reboot_flag;

	unsigned char bbcb_header[BBCB_HEADER_LENGTH];
	unsigned char bbcb[BBCB_LENGTH];

	unsigned char uk_header[UK_HEADER_LENGTH];
	unsigned char uk[UK_LENGTH];

	unsigned char kernel_header[KERNEL_HEADER_LENGTH];
	unsigned char kernel[KERNEL_LENGTH];
};

int Zapper_get_usb_download_request(void);
int Zapper_read_usb_file_name(char *file_name, int num);

int Zapper_get_nand_hwconfig_partition_address(unsigned long long hwconfig_s, unsigned long long hwconfig_e);
int Zapper_get_nand_ldflag_partition_address(unsigned long long ldflag_s, unsigned long long ldflag_e);
int Zapper_get_nand_ldsec_partition_address(unsigned long long ldsec_s, unsigned long long ldsec_e);
int Zapper_get_nand_kernel_partition_address(unsigned long long kernel_s, unsigned long long kernel_e);


int Zapper_get_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_hwconfig_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_ldsec_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_kernel_partition_info(struct Zapper_boot_info *p_s_e_boot_info);

int Zapper_set_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info);


int Zapper_get_key_info(unsigned char *key_index);


int Zapper_get_jump_recovery_status(unsigned char* status);
int Zapper_set_jump_recovery_status(unsigned char status);
int Zapper_clear_jump_recovery_status(void);


#endif
