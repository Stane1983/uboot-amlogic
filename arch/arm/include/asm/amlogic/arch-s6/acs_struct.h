#ifndef __ACS_STRUCT_H__
#define __ACS_STRUCT_H__

/*
 * Note, this header file must be same for bl33/bl2/bl2e/bl2x
 */
typedef struct bl2_reg {
	unsigned int reg;
	unsigned int value;
	unsigned int mask;
	unsigned short udelay;
	unsigned char flag;
	unsigned char rsv_0;
} __attribute__((packed)) bl2_reg_t;

typedef struct board_clk_set {
	unsigned short cpu_clk;
	unsigned short dsu_clk;
	unsigned short vddee;
	unsigned short vcck;
	unsigned short pxp;
	unsigned char low_console_baud;
	unsigned char szPad[1];
} __attribute__((packed)) board_clk_set_t;

/* gpio/pinmux/pwm */
typedef struct _register_ops {
	/* register address*/
	uint32_t reg;
	/* value to set*/
	uint32_t value;
	/* bitmask of the value setting */
	uint32_t mask;
	/* for HW stable consideration */
	uint16_t udelay;
	/* setting stage and etc. */
	uint8_t flag;
	/* reserved. */
	uint8_t rsv_0;
} __attribute__ ((packed)) register_ops_t;

#define MAX_REG_OPS_ENTRIES     (8)
typedef struct pin_pwm_parameter {
	register_ops_t pin_pwm[MAX_REG_OPS_ENTRIES];
} __attribute__ ((packed)) pin_pwm_parameter_t;

typedef struct common_storage_parameter {
	/* version info of the common storage parameter */
	uint32_t version;
	/* fip sector counts */
	uint32_t device_fip_container_size;
	/* fip copies */
	uint32_t device_fip_container_copies;
	/*ddrfip size*/
	uint32_t ddr_fip_container_size;

	uint8_t reserved[16];
} __attribute__ ((packed)) common_storage_parameter_t;

typedef struct nand_parameter {
	/* version info of the common storage parameter */
	uint32_t version;
	/* the same as bbt_start_block, tell the bbt size for scanning mechanism */
	uint32_t bbt_pages;
	/* for bl2 stage, it can quickly generate the small part of the fromt bbt table.
	 * 20 for start block of bbt scanning
	 */
	uint32_t bbt_start_block;
	/* 1: bl2 and fip is stored separately in different area
	 * 0: bl2 and fip is stored in first 1024 pages. 1 for slc nand flash.
	 */
	uint32_t discrete_mode;
	/* set the setup_data the same as rom code reading from page0.
	 * see union cmdinfo in nand.h.
	 */
	union {
		uint32_t nand_setup_data;
		uint32_t spi_nand_page_size;
	} setup_data;
	union {
		uint32_t nand_reserved;
		uint32_t spi_nand_planes_per_lun;
	} reserved;
	/* Block counts of the reserved area */
	uint32_t reserved_area_blk_cnt;
	/* Page number of each block */
	uint32_t page_per_block;
	/* Page list source of the bl2 NAND driver. 0: calculated in source code;
	 * 1: get from byte32~63
	 */
	uint8_t use_param_page_list;
	/* List of page addresses, 8-bit per entry */
	uint8_t page_list[32];
	uint8_t reserved1[63];
} __attribute__ ((packed)) nand_parameter_t;

typedef struct storage_parameter {
	/* for all the storage media */
	common_storage_parameter_t common;
	/* for NAND and SPINAND */
	nand_parameter_t nand;
} __attribute__ ((packed)) storage_parameter_t;

typedef struct board_common_setting {
	unsigned int timming_magic;
	unsigned short timming_max_valid_configs;
	unsigned short timming_struct_version;
	unsigned short timming_struct_org_size;
	unsigned short timming_struct_real_size;
	/* 0 fastboot enable  1 window test margin
	 * 2 auto off    set after window test 3 auto window test enable
	 */
	unsigned char fast_boot[4];
	unsigned int ddr_func;
	unsigned char board_id;
	unsigned char DramType;
	unsigned char dram_rank_config;
	unsigned char DisabledDbyte;
	unsigned int dram_cs0_base_add;
	unsigned int dram_cs1_base_add;
	unsigned short dram_cs0_size_MB;
	unsigned short dram_cs1_size_MB;
	unsigned char dram_x4x8x16_mode;
	unsigned char Is2Ttiming;
	unsigned char log_level;
	unsigned char ddr_rdbi_wr_enable;
	unsigned int pll_ssc_mode;
	unsigned short org_tdqs2dq;
	unsigned char reserve1_test_function[2];
	unsigned int ddr_dmc_remap[5];
	unsigned char ac_pinmux[35];
	unsigned char ddr_dqs_swap;
	unsigned char ddr_dq_remap[36];
	unsigned int ddr_vddee_setting[4];	//add,default-value,default-voltage,step
} __attribute__((packed)) board_common_setting_t;

typedef struct board_SI_setting_ps {
	unsigned short DRAMFreq;
	unsigned char PllBypassEn;
	unsigned char training_SequenceCtrl;
	unsigned short ddr_odt_config;
	unsigned char clk_drv_ohm;
	unsigned char cs_drv_ohm;
	unsigned char ac_drv_ohm;
	unsigned char soc_data_drv_ohm_p;
	unsigned char soc_data_drv_ohm_n;
	unsigned char soc_data_odt_ohm_p;
	unsigned char soc_data_odt_ohm_n;
	unsigned char dram_data_drv_ohm;
	unsigned char dram_data_odt_ohm;
	unsigned char dram_data_wr_odt_ohm;
	unsigned char dram_ac_odt_ohm;
	unsigned char dram_data_drv_pull_up_calibration_ohm;
	unsigned char lpddr4_dram_vout_voltage_range_setting;
	unsigned char dfe_offset;
	unsigned short vref_ac_permil;	//phy
	unsigned short vref_soc_data_permil;	//soc
	unsigned short vref_dram_data_permil;
	unsigned short max_core_timmming_frequency;
	unsigned short training_phase_parameter[2];
	unsigned short ac_trace_delay_org[36];
} __attribute__((packed)) board_SI_setting_ps_t;

typedef struct board_phase_setting_ps {
	unsigned short ac_trace_delay[36];
	unsigned short write_dqs_delay[8];
	unsigned short write_dq_bit_delay[72];
	unsigned short read_dqs_gate_delay[8];
	unsigned char read_dqs_delay[8];
	unsigned char read_dq_bit_delay[72];
	unsigned char soc_bit_vref[44];
	unsigned char dram_bit_vref[36];
	/* 0-7 write dqs offset, 8-15 read dqs offset,
	 * MSB bit 7 use 0 mean right offset
	 */
	unsigned char reserve_training_parameter[16];
	unsigned char soc_bit_vref_dac1[44];
} __attribute__((packed)) board_phase_setting_ps_t;

typedef struct ddr_set {
	board_common_setting_t cfg_board_common_setting;
	board_SI_setting_ps_t cfg_board_SI_setting_ps[2];
	board_phase_setting_ps_t cfg_ddr_training_delay_ps[2];
} __attribute__((packed)) ddr_set_t;

typedef struct ddr_set_ps0_only {
	board_common_setting_t cfg_board_common_setting;
	board_SI_setting_ps_t cfg_board_SI_setting_ps;
	board_phase_setting_ps_t cfg_ddr_training_delay_ps;
} __attribute__((packed)) ddr_set_ps0_only_t;

typedef struct dev_param_hdr {
	unsigned int		magic;
	unsigned int		version;
	unsigned int		head_crc;

	char			bl2_regs_magic[6];
	unsigned short		bl2_regs_length;

	char			board_clk_magic[6];
	unsigned short		board_clk_length;

	char			opt_reg_magic[6];
	unsigned short		opt_reg_length;

	char			sto_set_magic[6];
	unsigned short		sto_set_length;

	char			ddr_set_magic[6];
	unsigned short		ddr_set_length;

	unsigned int		RFU[4];
} __attribute__ ((packed)) dev_param_hdr_t;
#endif
