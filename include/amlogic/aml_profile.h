/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __PROFILE_H__
#define __PROFILE_H__

struct measurement_item {
	int start_index;
	int end_index;
	char name[24];
};

enum bl2_time {
	/*record bl2 stage time point*/
	BL2_CORE_ROM = 0,
	BL2_CORE_ENTRY,			//1
	BL2_CORE_DDR_s,			//2
	BL2_CORE_DDR_e,			//3
	BL2_CORE_LOAD_BL2E_s,	//4
	BL2_CORE_LOAD_BL2E_e,	//5
	BL2_CORE_LOAD_BL2X_s,	//6
	BL2_CORE_LOAD_BL2X_e,	//7
	BL2_CORE_VERIFY_BL2EX_s,//8
	BL2_CORE_VERIFY_BL2EX_e,//9
	BL2_CORE_BOOT_BL2E,		//10
	BL2_CORE_MAX_NUM,		//11
};

enum bl2e_time {
	/*record bl2e stage time point*/
	BL2_REE_FLAG = 0,
	BL2_REE_ENTRY,				//1
	BL2_REE_LOAD_HEADER_s,		//2
	BL2_REE_LOAD_HEADER_e,		//3
	BL2_REE_PROCESS_HEADER_s,	//4
	BL2_REE_PROCESS_HEADER_e,	//5
	BL2_REE_LOAD_DEVFIP_s,		//6
	BL2_REE_LOAD_DEVFIP_e,		//7
	BL2_REE_PROCESS_DEVFIP_s,	//8
	BL2_REE_PROCESS_DEVFIP_e,	//9
	BL2_REE_STAGE_2,			//10
	BL2_REE_MAX_NUM,			//11
};

enum bl2x_time {
	/*record bl2x stage time point*/
	BL2_TEE_FLAG = 0,
	BL2_TEE_ENTRY,				//1
	BL2_TEE_PARSE_BL3X,			//2
	BL2_TEE_DECRYPT_BL3X,		//3
	BL2_TEE_BOOT_BL31,			//4
	BL2_TEE_MAX_NUM,			//5
};

enum bl31_time {
	/*record bl31 stage time point*/
	BL31_FLAG = 0,
	BL31_ENTRY,				//1
	BL31_SOC_INIT_s,		//2
	BL31_SOC_INIT_e,		//3
	BL31_BL32_INIT_s,		//4
	BL31_BL32_INIT_e,		//5
	BL31_EXIT_MAIN,			//6
	BL31_MAX_NUM,			//7
};

enum bl33_time {
	/*record bl33 stage time point*/
	BL33_FLAG = 0,
	BL33_ENTRY,				//1
	BL33_RELOCATE_finish,	//2
	BL33_STORAGE_s,			//3
	BL33_STORAGE_e,			//4
	BL33_ENV_s,				//5
	BL33_ENV_e,				//6
	BL33_LOAD_DTB_s,		//7
	BL33_LOAD_DTB_e,		//8
	BL33_VPUVPP_INIT_s,		//9
	BL33_VPUVPP_INIT_e,		//10
	BL33_TAIL_INIT_s,		//11
	BL33_TAIL_INIT_e,		//12
	BL33_PREBOOT_s,			//13
	BL33_PREBOOT_e,			//14
	BL33_LOAD_UIMAGE_s,		//15
	BL33_LOAD_UIMAGE_e,		//16
	BL33_BOOT_KERNEL_s,		//17
	BL33_MAX_NUM,			//18
};

struct time_point_t {
	char func[12];
	unsigned int time;
};

void push_time_te(const char *func, unsigned int stage);
void dump_te(void);

#ifdef CONFIG_MEASUREMENT_BOOT_TIME
#define PUSH_TIME_TE(a, b) push_time_te(a, b)
#define DUMP_TE()	dump_te()
#else
#define PUSH_TIME_TE(a, b)
#define DUMP_TE()
#endif

#endif	//_PROFILE_H_
