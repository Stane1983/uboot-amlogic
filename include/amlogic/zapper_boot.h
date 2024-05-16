#ifndef _ZAPPER_BOOT_H_
#define _ZAPPER_BOOT_H_

#include <amlogic/zapper_boot_common.h>

/* define the reboot flag */
#define REBOOT_FLAG_WARM    (0x55)  /* The process of automatically rebooting after a successful download update is called a warm reboot. */
#define REBOOT_FLAG_COLD    (0xAA)  /* Other than the warm reboot, all other reboot scenarios are called cold reboot. */
#define REBOOT_FLAG_FIRST   (0xFF)
#define DOWNLOAD_MASK (0b00010000)
#define OTA_DOWNLOAD_INDICATOR (0b00010010)
#define USB_DOWNLOAD_INDICATOR (0b00010011)

//---------------From irdeto-downloader START-----------//

/* Typical size for module_info_header */
#define AML_TYPICAL_MODULE_INFO_HEADER_SIZE (26)
/* Length of signed CCP CAM signature */
#define AML_SIGNATURE_LENGTH                (256)
/* Short size for module_info_header, applicable for BBCB, Loader Core, and Loader Partition. */
#define AML_SHORT_MODULE_INFO_HEADER_SIZE   (12)

/* header size of boot,system,ccaconfig,casecure */
#define AML_NORMAL_HEAD_SIZE    (AML_TYPICAL_MODULE_INFO_HEADER_SIZE + 2 * AML_SIGNATURE_LENGTH)
/* boot header */
#define BOOT_HEAD_OFFSET        AML_SHORT_MODULE_INFO_HEADER_SIZE
/* system header */
#define SYSTEM_HEAD_OFFSET      (AML_SHORT_MODULE_INFO_HEADER_SIZE + AML_NORMAL_HEAD_SIZE)
/* CCA Config header */
#define CCA_HEAD_OFFSET         (AML_SHORT_MODULE_INFO_HEADER_SIZE + 3 * AML_NORMAL_HEAD_SIZE)
/* CASECURE header */
#define CASECURE_HEAD_OFFSET    (AML_SHORT_MODULE_INFO_HEADER_SIZE + 4 * AML_NORMAL_HEAD_SIZE)

//---------------From irdeto-downloader END-----------//

//---------------From FLASHMAP, CL:425841 START-----------//
#define BOOT_PARTITION_NAME         "boot"
#define SYSTEM_PARTITION_NAME       "system"
#define CASECURE_PARTITION_NAME     "casecure"
#define CCACONFIG_PARTITION_NAME    "ccaconfig"
#define CAVERIFY_PARTITION_NAME     "caverify"
#define RESCUELIST_PARTITION_NAME   "rescuelist"

#define FACTORY_RESET_CADATA_PARTITION_NAME     "cadata"

#define BOOT_PARTITION_SIZE         (12 * 1024 * 1024)
#define SYSTEM_PARTITION_SIZE       (48 * 1024 * 1024)
#define CASECURE_PARTITION_SIZE     (4 * 1024 * 1024)
#define CCACONFIG_PARTITION_SIZE    (128 * 1024)
#define CAVERIFY_PARTITION_SIZE     (128 * 1024)
#define RESCUELIST_PARTITION_SIZE   (128 * 1024)
//---------------From FLASHMAP, CL:425841 END-----------//

#define FACTORY_RESET_PS_INDEX_0    "cloaked_ca_0.dat"
#define FACTORY_RESET_PS_INDEX_15   "cloaked_ca_15.dat"

typedef enum {
    VERIFY_MODULE_BOOT,         /* module-id, 0x20 */
    VERIFY_MODULE_SYSTEM,       /* module-id, 0x21 */
    VERIFY_MODULE_CCACONFIG,    /* module-id, 0x23 */
    VERIFY_MODULE_CASECURE,     /* module-id, 0x24 */
} VERIFY_MODULE_TYPE;

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

/**
 * Download MODE, Consistent with the definition in the downloader.
 */
enum DOWNLOAD_MODE_TYPE{
    DOWNLOAD_MODE_NORMAL = 0,
    DOWNLOAD_MODE_RESCUE,
    DOWNLOAD_MODE_MANUAL_FORCE,
    DOWNLOAD_MODE_USB,
    DOWNLOAD_MODE_TUNING_CODE,
    DOWNLOAD_MODE_ADVANCE_SETUP,
    DOWNLOAD_MODE_MAX
};

typedef enum {
    LED_POWER_RED,      /* The macro corresponds to the "Red" description of the Standby LED in the LDRS table */
    LED_POWER_GREEN,    /* The macro corresponds to the "Green" description of the Standby LED in the LDRS table */
    LED_POWER_OFF,      /* The macro corresponds to the "Off" description of the Standby LED in the LDRS table */
    LED_REMOTE_RED,     /* The macro corresponds to the "Red" description of the Remote LED in the LDRS table */
    LED_REMOTE_OFF,     /* The macro corresponds to the "Off" description of the Remote LED in the LDRS table */
    LED_ALERT_YELLOW,   /* The macro corresponds to the "Yellow" description of the Alert LED in the LDRS table */
    LED_ALERT_OFF,      /* The macro corresponds to the "Off" description of the Alert LED in the LDRS table */
} led_display_type;

struct Zapper_boot_info {
    unsigned char loader_partition_header[LD_HEADER_LENGTH];
    unsigned char loader_partition[LD_LENGTH];
    unsigned char error_code[EC_LENGTH];
    unsigned char modify_flag;
    unsigned char reboot_flag;
    unsigned char download_mode;
    unsigned char standby_flag;
    unsigned char backupODU;
    unsigned char share_crc[LD_SHARE_DATA_CRC_LENGTH];

    unsigned char bbcb_header[BBCB_HEADER_LENGTH];
    unsigned char bbcb[BBCB_LENGTH];

    unsigned char uk_header[UK_HEADER_LENGTH];
    unsigned char uk[UK_LENGTH];

    unsigned char kernel_header[KERNEL_HEADER_LENGTH];
    unsigned char kernel[KERNEL_LENGTH];
};

/* these structure is from irdeto-download spi */
typedef enum
{
    DMD_DOWNLINK_INJECTION,
    DMD_DOWNLINK_CUSTOM,
    DMD_DOWNLINK_Ku_LOW,
    DMD_DOWNLINK_Ku_HIGH,
    DMD_DOWNLINK_C,
    DMD_DOWNLINK_C_LOW,
    DMD_DOWNLINK_C_HIGH
} dmd_downlink_t;

typedef enum
{
    DMD_LNB_TONE_DEFAULT,
    DMD_LNB_TONE_OFF,
    DMD_LNB_TONE_22KHZ
} dmd_lnb_tone_state_t;

typedef struct
{
    unsigned int band_start;
    unsigned int band_end;
    unsigned int lo;
    dmd_downlink_t downlink;
} dmd_satellite_band_t;

typedef enum
{
    DMD_DISEQC_DEFAULT,
    DMD_DISEQC_PORTA,
    DMD_DISEQC_PORTB,
    DMD_DISEQC_PORTC,
    DMD_DISEQC_PORTD,
    DMD_DISEQC_ALL
} dmd_diseqc_port_t;

typedef enum
{
    DMD_LNB_VOLTAGE_OFF = 0,
    DMD_LNB_VOLTAGE_14V = 1,
    DMD_LNB_VOLTAGE_18V = 2
} dmd_lnb_voltage_t;

typedef enum
{
    DMD_PLR_HORIZONTAL = 0x01,
    DMD_PLR_VERTICAL = 0x02,
    DMD_PLR_NONE = 0x03,
    DMD_PLR_CIRCULAR_LEFT = 0x04,
    DMD_PLR_CIRCULAR_RIGHT = 0x05
} dmd_polarization_t;

typedef enum
{
    DMD_ROLLOFF_035 = 0x00,
    DMD_ROLLOFF_025 = 0x01,
    DMD_ROLLOFF_020 = 0x02,
    DMD_ROLLOFF_015 = 0x03,
    DMD_ROLLOFF_010 = 0x04,
    DMD_ROLLOFF_005 = 0x05
} dmd_rolloff_t;

typedef enum
{
    DMD_MODSYS_DVBS = 0x00,
    DMD_MODSYS_DVBS2 = 0x01
} dmd_modulation_system_t;

typedef enum
{
    DMD_FEC_NONE = 0x0000,
    DMD_FEC_1_2 = 0x0001,
    DMD_FEC_2_3 = 0x0002,
    DMD_FEC_3_4 = 0x0004,
    DMD_FEC_4_5 = 0x0008,
    DMD_FEC_5_6 = 0x0010,
    DMD_FEC_6_7 = 0x0020,
    DMD_FEC_7_8 = 0x0040,
    DMD_FEC_8_9 = 0x0080,
    DMD_FEC_3_5 = 0x0100,
    DMD_FEC_9_10 = 0x0200,
    DMD_FEC_ALL = 0xFFFF
} dmd_fec_rate_t;

typedef enum
{
    DMD_FEC_OUTER_NONE = 0x00,
    DMD_FEC_OUTER_NOT_DEFINED = 0x01,
    DMD_FEC_OUTER_RS = 0x02
} dmd_fec_rate_outer_t;

typedef enum
{
    DMD_MOD_NONE = 0x0000,
    DMD_MOD_QPSK = 0x0001,
    DMD_MOD_8PSK = 0x0002,
    DMD_MOD_QAM = 0x0004,
    DMD_MOD_4QAM = 0x0008,
    DMD_MOD_16QAM = 0x0010,
    DMD_MOD_32QAM = 0x0020,
    DMD_MOD_64QAM = 0x0040,
    DMD_MOD_128QAM = 0x0080,
    DMD_MOD_256QAM = 0x0100,
    DMD_MOD_BPSK = 0x0200,
    DMD_MOD_ALL = 0xFFFF
} dmd_modulation_t;

typedef enum
{
    DMD_CONSTELLATION_NONE = 0x0000,
    DMD_CONSTELLATION_QPSK = 0x0001,
    DMD_CONSTELLATION_16QAM = 0x0002,
    DMD_CONSTELLATION_64QAM = 0x0004,
    DMD_CONSTELLATION_32QAM = 0x0008,
    DMD_CONSTELLATION_128QAM = 0x0010,
    DMD_CONSTELLATION_256QAM = 0x0020,
    DMD_CONSTELLATION_1024QAM = 0x0040,
    DMD_CONSTELLATION_ALL = 0xFFFF
} dmd_constellation_t;

typedef enum
{
    DMD_GUARD_INTERVAL_1_32 = 0x01,
    DMD_GUARD_INTERVAL_1_16 = 0x02,
    DMD_GUARD_INTERVAL_1_8 = 0x03,
    DMD_GUARD_INTERVAL_1_4 = 0x04,
    DMD_GUARD_INTERVAL_1_128 = 0x05,
    DMD_GUARD_INTERVAL_19_128 = 0x06,
    DMD_GUARD_INTERVAL_19_256 = 0x07
} dmd_guard_interval_t;

typedef enum
{
    DMD_HIERARCHY_NONE = 0x01,
    DMD_HIERARCHY_1 = 0x02,
    DMD_HIERARCHY_2 = 0x03,
    DMD_HIERARCHY_4 = 0x04
} dmd_hierarchy_t;

typedef enum
{
    DMD_HIERARCHY_HP = 0x00,
    DMD_HIERARCHY_LP = 0x01
} dmd_hierarchy_hplp_t;

typedef enum
{
    DMD_PLP_COMMON,
    DMD_PLP_DATA1,
    DMD_PLP_DATA2
} dmd_plp_type_t;

typedef enum
{
    TUNER_STATE_LOCKED,
    TUNER_STATE_TIMEOUT,
    TUNER_STATE_UNKNOWN
} dmd_tuner_event_t;

typedef struct {
    unsigned char version; //not unicable: 0, 1.x: 1(EN50494), 2.x: 2(EN50607)
    unsigned short frequency_mhz;
    unsigned char userband;
    unsigned char bank;
    unsigned char uncommitted;
    unsigned char committed;
} dmd_unicable_param_t;

typedef struct
{
    unsigned int frequency;
    unsigned int symbol_rate;
    dmd_modulation_system_t modulation_system;
    dmd_polarization_t polarization;
    dmd_modulation_t modulation;
    dmd_fec_rate_t fec_rate;
    dmd_rolloff_t roll_off;
    dmd_lnb_tone_state_t    lnb_tone_state;
    dmd_diseqc_port_t       diseqc_port;
    dmd_satellite_band_t    band;
} dmd_satellite_desc_t;

typedef struct
{
    unsigned int frequency;                     /* frequency in KHz */
    dmd_fec_rate_t fec_rate;                    /* select FEC rate(s) */
    dmd_fec_rate_outer_t fec_rate_outer;        /* select FEC outer */
    dmd_modulation_t modulation;                /* select modulation scheme */
    unsigned int symbol_rate;                   /* symbol rate in KBAUD */
} dmd_cable_desc_t;

typedef enum
{
    DMD_TRANSMISSION_2K = 0x01,
    DMD_TRANSMISSION_8K = 0x02,
    DMD_TRANSMISSION_4K = 0x03,
    DMD_TRANSMISSION_1K = 0x04,
    DMD_TRANSMISSION_16K = 0x05,
    DMD_TRANSMISSION_32K = 0x06
} dmd_transmission_mode_t;

typedef enum
{
    DMD_BANDWIDTH_8M = 0x01,
    DMD_BANDWIDTH_7M = 0x02,
    DMD_BANDWIDTH_6M = 0x03,
    DMD_BANDWIDTH_5M = 0x04,
    DMD_BANDWIDTH_10M = 0x05,
    DMD_BANDWIDTH_17M = 0x06
} dmd_bandwidth_t;

typedef enum
{
    DMD_DVBTYPE_DVBT,
    DMD_DVBTYPE_DVBT2,
    DMD_DVBTYPE_DTMB
} dmd_terrestrial_dvbtype_t;

typedef struct
{
    unsigned int frequency;
    dmd_constellation_t constellation;
    dmd_hierarchy_t hierarchy;
    dmd_hierarchy_hplp_t hp_lp;
    dmd_fec_rate_t HP_code_rate;
    dmd_fec_rate_t LP_code_rate;
    dmd_guard_interval_t guard_interval;
    dmd_transmission_mode_t transmission_mode;
    dmd_bandwidth_t bandwidth;
} dmd_terrestrial_dvbt_desc_t;

typedef struct
{
    unsigned int frequency;
    unsigned int plp_id;
    dmd_guard_interval_t guard_interval;
    dmd_transmission_mode_t transmission_mode;
    unsigned int T2_system_id;
    dmd_bandwidth_t bandwidth;
} dmd_terrestrial_dvbt2_desc_t;

typedef struct
{
    unsigned int frequency;
    dmd_bandwidth_t bandwidth;
} dmd_terrestrial_dtmb_desc_t;

typedef struct
{
    dmd_terrestrial_dvbtype_t dvb_type;
    union {
      dmd_terrestrial_dvbt_desc_t dvbt;
      dmd_terrestrial_dvbt2_desc_t dvbt2;
      dmd_terrestrial_dtmb_desc_t dtmb;
    } desc;
} dmd_terrestrial_desc_t;

typedef enum
{
    DMD_SATELLITE   = 0x0100,
    DMD_CABLE      = 0x0200,
    DMD_TERRESTRIAL = 0x0300,
} dmd_device_type_t;

typedef struct
{
    dmd_device_type_t   device_type;
    union
    {
        dmd_satellite_desc_t    satellite;
        dmd_cable_desc_t        cable;
        dmd_terrestrial_desc_t terrestrial;
    } delivery;
} dmd_delivery_t;

/* SPI END */

int Zapper_get_usb_download_request(void);
int Zapper_read_usb_file_name(char *file_name, int num);

int Zapper_get_nand_hwconfig_partition_address(unsigned long long hwconfig_s, unsigned long long hwconfig_e);
int Zapper_get_nand_ldflag_partition_address(unsigned long long ldflag_s, unsigned long long ldflag_e);
int Zapper_get_nand_ldsec_partition_address(unsigned long long ldsec_s, unsigned long long ldsec_e);
int Zapper_get_nand_kernel_partition_address(unsigned long long kernel_s, unsigned long long kernel_e);

int Zapper_read_verify_module_partition(void);
void Zapper_free_verify_module_partition(void);
int Zapper_get_verify_module_header(VERIFY_MODULE_TYPE module, unsigned char **buf, unsigned int *len);
int Zapper_get_verify_module_payload(VERIFY_MODULE_TYPE module, unsigned char **buf, unsigned int *len);
int Zapper_get_rescuelist_module_payload(unsigned char *buf, unsigned int len);

int Zapper_get_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_hwconfig_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_ldsec_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_get_nand_kernel_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
unsigned char Zapper_get_nand_standby_flag(void);

int Zapper_set_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info);
int Zapper_nand_factory_reset(void);

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
