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

enum RCU_COMBINATION_TYPE {
	RCU_COMBINATION_ADVANCED_TUNING_CODE_SCREEN = 0,
	RCU_COMBINATION_ADVANCED_SETUP_SCREEN = 1,
	RCU_COMBINATION_USB_UPGRADE = 2,
	RCU_COMBINATION_MANUAL_FORCED_DOWNLOAD = 3,
	RCU_COMBINATION_FACTORY_RESET = 4,
	RCU_COMBINATION_MAX = 5,
};

typedef enum {
	LED_POWER_RED,		/* The macro corresponds to the "Red" description of the Standby LED in the LDRS table */
	LED_POWER_GREEN,	/* The macro corresponds to the "Green" description of the Standby LED in the LDRS table */
	LED_POWER_OFF,		/* The macro corresponds to the "Off" description of the Standby LED in the LDRS table */
	LED_REMOTE_RED,		/* The macro corresponds to the "Red" description of the Remote LED in the LDRS table */
	LED_REMOTE_OFF,		/* The macro corresponds to the "Off" description of the Remote LED in the LDRS table */
	LED_ALERT_YELLOW,	/* The macro corresponds to the "Yellow" description of the Alert LED in the LDRS table */
	LED_ALERT_OFF,		/* The macro corresponds to the "Off" description of the Alert LED in the LDRS table */
} led_display_type;

struct Zapper_boot_info {
	unsigned char loader_partition_header[LD_HEADER_LENGTH];
	unsigned char loader_partition[LD_LENGTH];
	unsigned char error_code[EC_LENGTH];
	unsigned char modify_flag;
	unsigned char reboot_flag;
	unsigned char download_mode;
	unsigned char standby_flag;

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
unsigned char Zapper_get_nand_standby_flag(void);

int Zapper_set_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info);


int Zapper_get_key_info(unsigned char *key_index);
int Zapper_get_rcu_combination_type(unsigned char *type);


int Zapper_get_jump_recovery_status(unsigned char* status);
int Zapper_set_jump_recovery_status(unsigned char status);
int Zapper_clear_jump_recovery_status(void);

/* set led display type */
void Zapper_led_set(led_display_type type);

/* Illuminate the LED through the LED type */
void Zapper_led_show(void);

#endif
