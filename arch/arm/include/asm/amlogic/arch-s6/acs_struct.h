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

#define MAX_REG_OPS_ENTRIES     (32)
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
	unsigned int	timming_magic;
	unsigned short	timming_max_valid_configs;
	unsigned short	timming_struct_version;
	unsigned short	timming_struct_org_size;
	unsigned short	timming_struct_real_size;
	unsigned char	fast_boot[4];
	// 0   fastboot enable  1 window test margin
	// 2 auto offset after window test 3 auto window test enable
	unsigned int	ddr_func;
	unsigned char	board_id;
	unsigned char	DramType; //bit 7 use for sip id
	//support DramType should confirm with amlogic
	//#define CONFIG_DDR_TYPE_DDR3				0
	//#define CONFIG_DDR_TYPE_DDR4				1
	//#define CONFIG_DDR_TYPE_LPDDR4				2
	//#define CONFIG_DDR_TYPE_LPDDR3				3
	//#define CONFIG_DDR_TYPE_LPDDR2				4
	//#define CONFIG_DDR_TYPE_LPDDR4x
	unsigned char dram_rank_config;
	//support Dram connection type should confirm with amlogic
	//dram total bus width 16bit only use cs0
	//#define CONFIG_DDR0_16BIT_CH0				0x1

	//dram total bus width 16bit  use cs0 cs1
	//#define CONFIG_DDR0_16BIT_RANK01_CH0		0x4

	//dram total bus width 32bit  use cs0
	//#define CONFIG_DDR0_32BIT_RANK0_CH0			0x2

	//only for lpddr4,dram total bus width 32bit  use channel a cs0 cs1 channel b cs0 cs1
	//#define CONFIG_DDR0_32BIT_RANK01_CH01		0x3

	//dram total bus width 32bit only use cs0,but high address use 16bit mode
	//#define CONFIG_DDR0_32BIT_16BIT_RANK0_CH0		0x5

	//dram total bus width 32bit  use cs0 cs1,but cs1 use 16bit mode,
	//current phy not support reserve
	//#define CONFIG_DDR0_32BIT_16BIT_RANK01_CH0	0x6

	//dram total bus width 32bit  use cs0 cs1
	//#define CONFIG_DDR0_32BIT_RANK01_CH0		0x7

	//only for lpddr4,dram total bus width 32bit  use channel a cs0  channel b cs0
	//#define CONFIG_DDR0_32BIT_RANK0_CH01		0x8

	/* rsv_char0. update for diagnose type define */
	unsigned char	rsv_char0;

	unsigned char	DisabledDbyte[2];            //ch0 and ch1
	//use for dram bus 16bit or 32bit,if use 16bit mode ,should disable bit 2,3
	//bit 0 ---cs0 use byte 0 ,1 disable byte 0,
	//bit 1 ---cs0 use byte 1 ,1 disable byte 1,
	//bit 2 ---cs0 use byte 2 ,1 disable byte 2,
	//bit 3 ---cs0 use byte 3 ,1 disable byte 3,
	//bit 4 ---cs1 use byte 0 ,1 disable byte 0,
	//bit 5 ---cs1 use byte 1 ,1 disable byte 1,
	//bit 6 ---cs1 use byte 2 ,1 disable byte 2,
	//bit 7 ---cs1 use byte 3 ,1 disable byte 3,
	unsigned short	dram_ch0_size_MB;
	//config cs0 dram size ,like 1G DRAM ,setting 1024
	unsigned short	dram_ch1_size_MB; //
	//config cs1 dram size,like 512M DRAM ,setting 512
	/* align8 */

	unsigned char	dram_x4x8x16_mode;
	unsigned char	Is2Ttiming;
	unsigned char	log_level;
	unsigned char	dbi_enable;
	//system reserve,do not modify
	unsigned char	ddr_rfc_type;
	//config dram rfc type,according dram type,also can use same dram type max config
	//#define DDR_RFC_TYPE_DDR3_512Mbx1				0
	//#define DDR_RFC_TYPE_DDR3_512Mbx2				1
	//#define DDR_RFC_TYPE_DDR3_512Mbx4				2
	//#define DDR_RFC_TYPE_DDR3_512Mbx8				3
	//#define DDR_RFC_TYPE_DDR3_512Mbx16				4
	//#define DDR_RFC_TYPE_DDR4_2Gbx1					5
	//#define DDR_RFC_TYPE_DDR4_2Gbx2					6
	//#define DDR_RFC_TYPE_DDR4_2Gbx4					7
	//#define DDR_RFC_TYPE_DDR4_2Gbx8					8
	//#define DDR_RFC_TYPE_LPDDR4_2Gbx1				9
	//#define DDR_RFC_TYPE_LPDDR4_3Gbx1				10
	//#define DDR_RFC_TYPE_LPDDR4_4Gbx1				11
	unsigned char	enable_lpddr4x_mode;
	//system reserve,do not modify
	/* align8 */

	unsigned int	pll_ssc_mode;
	//
	//pll ssc config:
	//
	//  pll_ssc_mode = (1<<20) | (1<<8) | ([strength] << 4) | [mode],
	//     ppm = strength * 500
	//     mode: 0=center, 1=up, 2=down
	//
	//  eg:
	//    1. config 1000ppm center ss. then mode=0, strength=2
	//       .pll_ssc_mode = (1<<20) | (1<<8) | (2 << 4) | 0,
	//    2. config 3000ppm down ss. then mode=2, strength=6
	//       .pll_ssc_mode = (1<<20) | (1<<8) | (6 << 4) | 2,
	//
	unsigned short	org_tdqs2dq;
	unsigned char	reserve1_test[2];
	unsigned int	ddr_dmc_remap[6];
	unsigned char	lpddr34_ca_remap[4];
	unsigned char	ddr_dq_remap[36];
	unsigned char	ac_pinmux[DWC_AC_PINMUX_TOTAL]; //24 35
	unsigned char	dfi_pinmux[DWC_DFI_PINMUX_TOTAL];
	unsigned char	ddr_dqs_swap;
	unsigned char	rsv_char1;
	unsigned char	rsv_char2;
	unsigned char	rsv_char3;
	unsigned int	ddr_vddee_setting[4];   // add,default-value,default-voltage,
}__attribute__ ((packed)) board_common_setting_t;
typedef struct board_SI_setting_ps {
	unsigned short	DRAMFreq;
	unsigned char	PllBypassEn;
	unsigned char	training_SequenceCtrl;
	unsigned int	dfi_odt_config;
	//normal go status od config,use for normal status
	//bit 12.  rank1 ODT default. default vulue for ODT[1] pins if theres
	//no read/write activity.
	//bit 11.  rank1 ODT write sel.  enable ODT[1] if there's write  occurred in rank1.
	//bit 10.  rank1 ODT write nsel. enable ODT[1] if theres's write  occurred in rank0.
	//bit 9.   rank1 odt read sel.   enable ODT[1] if there's read  occurred in rank1.
	//bit 8.   rank1 odt read nsel.  enable ODT[1] if there's read  occurrede in rank0.
	//bit 4.   rank0 ODT default.    default vulue for ODT[0] pins if
	//theres no read/write activity.
	//bit 3.   rank0 ODT write sel.  enable ODT[0] if there's write  occurred in rank0.
	//bit 2.   rank0 ODT write nsel. enable ODT[0] if theres's write  occurred in rank1.
	//bit 1.   rank0 odt read sel.   enable ODT[0] if there's read  occurred in rank0.
	//bit 0.   rank0 odt read nsel.  enable ODT[0] if there's read  occurrede in rank1.
	unsigned char	phy_odt_config_rank[2];
	unsigned short	clk_drv_ohm;
	//config soc clk pin signal driver strength ,select 20,30,40,60ohm
	unsigned short	cs_drv_ohm;
	//config soc cs0 cs1 pin signal driver strength ,select 20,30,40,60ohm
	unsigned short	ac_drv_ohm;
	//config soc  normal address command pin driver strength ,select 20,30,40,60ohm
	unsigned short	soc_data_drv_ohm_p;
	//config soc data pin pull up driver strength,
	//select 0,28,30,32,34,37,40,43,48,53,60,68,80,96,120ohm
	unsigned short	soc_data_drv_ohm_n;
	//config soc data pin pull down driver strength,
	//select 0,28,30,32,34,37,40,43,48,53,60,68,80,96,120ohm
	unsigned short	soc_data_odt_ohm_p;
	//config soc data pin odt pull up strength,
	//select 0,28,30,32,34,37,40,43,48,53,60,68,80,96,120ohm
	unsigned short	soc_data_odt_ohm_n;
	//config soc data pin odt pull down strength,
	//select 0,28,30,32,34,37,40,43,48,53,60,68,80,96,120ohm
	unsigned short	dram_data_drv_ohm;
	//config dram data pin pull up pull down driver strength,
	//ddr3 select 34,40ohm,ddr4 select 34,48ohm,lpddr4 select 40,48,60,80,120,240ohm
	unsigned short	dram_data_odt_ohm;
	//config dram data pin odt pull up down strength,ddr3 select 40,60,120ohm,
	//ddr4 select 34,40,48,60,120,240ohm,lpddr4 select 40,48,60,80,120,240ohm
	unsigned short	dram_data_wr_odt_ohm;
	//config dram data pin odt pull up down strength,ddr3 select 40,60,120ohm,
	//ddr4 select 34,40,48,60,120,240ohm,lpddr4 select 40,48,60,80,120,240ohm
	unsigned short	dram_ac_odt_ohm;
	//config dram ac pin odt pull up down strength,use for lpddr4, select 40,48,60,80,120,240ohm
	unsigned short	soc_clk_slew_rate;
	//system reserve,do not modify
	unsigned short	soc_cs_slew_rate;
	//system reserve,do not modify
	unsigned short	soc_ac_slew_rate;
	//system reserve,do not modify
	unsigned short	soc_data_slew_rate;
	//system reserve,do not modify
	unsigned char	dram_drv_pull_up_cal_ohm;
	//config soc data pin odt pull up strength,select 40,60,80,120ohm
	unsigned char	lpddr4_dram_vout_range;
	//use for lpddr4 read vout voltage  setting 0 --->2/5VDDQ ,1--->1/3VDDQ
	unsigned char	char_rev0;                      //dfe_offset_value;       //char_rev1;
	unsigned char	char_rev1;                      //training_offset;        //char_rev2;

	unsigned short	vref_ac_permil;
	//soc init dram ac vref ,config like 500 means 0.5VDDQ,take care ,please follow SI
	unsigned short	vref_soc_data_permil;
	//soc init SOC receiver vref ,config like 500 means 0.5VDDQ,take care ,please follow SI
	unsigned short	vref_dram_data_permil;
	//soc init DRAM receiver vref ,config like 500 means 0.5VDDQ,take care ,please follow SI
	unsigned short	max_core_timmming_frequency;
	//use for limited ddr speed core timmming parameter,
	//for some old dram maybe have no over speed register
	//unsigned short training_phase_parameter[2];
	//unsigned short ac_trace_delay_org[36];
}__attribute__ ((packed)) board_SI_setting_ps_t;
typedef struct board_phase_setting_ps {
	unsigned short	ac_trace_delay[DWC_AC_PINMUX_TOTAL];
	//unsigned char ac_trace_delay_rev[5];
	unsigned short	read_dq_delay_t[72];
	unsigned short	read_dq_delay_c[72];
	unsigned short	read_dqs_delay[8];
	unsigned short	write_dqs_delay[8];
	unsigned short	write_wck_delay[8];
	unsigned short	wdq_delay[72];
	unsigned short	read_dqs_gate_delay[8];
	unsigned char	soc_bit_vref0[36];
	unsigned char	soc_bit_vref1[36];
	unsigned char	soc_bit_vref2[36];
	unsigned char	soc_bit_vref3[36];
	unsigned char	dram_vref[32];

	unsigned short	dca_wck_tx_t[8];        //t and c
	unsigned short	dca_wck_rx_t[8];        //t and c
	unsigned short	dca_dqs_tx_t[8];        //t and c
	unsigned short	dca_wck_tx_c[8];        //t and c
	unsigned short	dca_wck_rx_c[8];        //t and c
	unsigned short	dca_dqs_tx_c[8];        //t and c
	unsigned short	dca_dq_tx[8];           //common

	unsigned char	dfi_mrl[4];
	unsigned char	dfi_hwtmrl;
	unsigned char	csr_hwtctrl;
	unsigned char	rever1;
	unsigned char	dram_vref_offset;
	//unsigned	char	ARdPtrInitVal;
	//unsigned	short	csr_vrefinglobal;
	//unsigned	short	csr_dqsrcvcntrl[4];
	unsigned short	pptdqscnttg0[4];
	unsigned short	pptdqscnttg1[4];
	unsigned short	PptWck2DqoCntTg0[4];
	unsigned short	PptWck2DqoCntTg1[4];
	unsigned short	RxReplicaPhase[4][5];
	//unsigned	short	csr_RxReplicaCtl03[4];
	//unsigned	short	csr_seq0bgpr[9];
	//unsigned	short	csr_dllgainctl;
	//unsigned	short	csr_dlllockpara;
	unsigned char	dac_offset[4];          //bit 7 offset direction 0 ++  1 --
	unsigned char	rx_offset[2];           //bit 7 offset direction 0 ++  1 --
	unsigned char	tx_offset[2];           //bit 7 offset direction 0 ++  1 --
	unsigned char	reserve_para[16];//prior high than tx rx_offset
	// 0-7 write dqs offset,8-15 read dqs offset,MSB bit 7 use 0 mean right offset
}__attribute__ ((packed)) board_phase_setting_ps_t;
typedef struct ddr_set {
	board_common_setting_t		cfg_board_common_setting;
	board_SI_setting_ps_t		cfg_board_SI_setting_ps;
	board_phase_setting_ps_t	cfg_ddr_training_delay_ps;
}__attribute__ ((packed)) ddr_set_t;

typedef struct ddr_set_ps0_only {
	board_common_setting_t		cfg_board_common_setting;
	board_SI_setting_ps_t		cfg_board_SI_setting_ps;
	board_phase_setting_ps_t	cfg_ddr_training_delay_ps;
} __attribute__ ((packed)) ddr_set_ps0_only_t;

typedef struct dev_param_hdr {
	unsigned int		magic;
	unsigned int		version;
	unsigned int		head_crc;

	char				bl2_regs_magic[6];
	unsigned short		bl2_regs_length;
	bl2_reg_t			*bl2_regs_sta;

	char				board_clk_magic[6];
	unsigned short		board_clk_length;
	board_clk_set_t		*board_clk_sta;

	char				opt_reg_magic[6];
	unsigned short		opt_reg_length;
	register_ops_t		*opt_reg_sta;

	char				sto_set_magic[6];
	unsigned short		sto_set_length;
	storage_parameter_t	*sto_set_sta;

	char				ddr_set_magic[6];
	unsigned short		ddr_set_length;
	ddr_set_ps0_only_t	*ddr_set_sta;

	char				ddr_2acs_magic[6];
	unsigned short		ddr_2acs_length;
	unsigned int		*ddr_2acs_sta;

	unsigned int		RFU[4];
} __attribute__ ((packed)) dev_param_hdr_t;

typedef struct vendor_key_s {
	uint32_t magic;
	uint32_t flags;
	uint8_t  pubkey[64];
	uint8_t  reserved[56];
} __attribute__ ((packed)) vendor_key_t;

#endif
