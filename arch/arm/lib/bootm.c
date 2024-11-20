// SPDX-License-Identifier: GPL-2.0+
/* Copyright (C) 2011
 * Corscience GmbH & Co. KG - Simon Schwarz <schwarz@corscience.de>
 *  - Added prep subcommand support
 *  - Reorganized source - modeled after powerpc version
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 */

#include <common.h>
#include <bootstage.h>
#include <command.h>
#include <cpu_func.h>
#include <dm.h>
#include <log.h>
#include <asm/global_data.h>
#include <dm/root.h>
#include <env.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <linux/libfdt.h>
#include <mapmem.h>
#include <fdt_support.h>
#include <asm/bootm.h>
#include <asm/secure.h>
#include <linux/compiler.h>
#include <bootm.h>
#include <vxworks.h>
#include <asm/cache.h>

#ifdef CONFIG_ARMV7_NONSEC
#include <asm/armv7.h>
#endif
#include <asm/setup.h>

#if defined(CONFIG_KEY_PRESERVE)
#include <asm/amlogic/arch/cpu.h>
#include <asm/amlogic/arch/register.h>
#endif

#ifdef CONFIG_AMLOGIC_MODIFY
#include <stdlib.h>
#ifdef CONFIG_AMLOGIC_TIME_PROFILE
#include <initcall.h>
#endif
#endif
#ifdef CONFIG_ARMV8_MULTIENTRY
#include <asm/arch-meson/smp.h>
#include <cli.h>
#endif
#include <amlogic/aml_profile.h>

DECLARE_GLOBAL_DATA_PTR;

static struct tag *params;
unsigned int timeout_cout = 500;

__weak void board_quiesce_devices(void)
{
}

/**
 * announce_and_cleanup() - Print message and prepare for kernel boot
 *
 * @fake: non-zero to do everything except actually boot
 */
static void announce_and_cleanup(int fake)
{
	bootstage_mark_name(BOOTSTAGE_ID_BOOTM_HANDOFF, "start_kernel");
#ifdef CONFIG_BOOTSTAGE_FDT
	bootstage_fdt_add_report();
#endif
#ifdef CONFIG_BOOTSTAGE_REPORT
	bootstage_report();
#endif

#ifdef CONFIG_USB_DEVICE
	udc_disconnect();
#endif

	board_quiesce_devices();

#ifdef CONFIG_AMLOGIC_MODIFY
	if (IS_ENABLED(CONFIG_SILENT_CONSOLE)) {
		/* disable silent */
		gd->flags &= ~GD_FLG_SILENT;
	}

	run_command("dmc_vio_check", 0);
#endif

	printf("\nStarting kernel ...%s\n\n", fake ?
		"(fake run for tracing)" : "");
#ifdef CONFIG_AMLOGIC_MODIFY
	#ifdef CONFIG_AMLOGIC_TIME_PROFILE
		if (gd->time_print_flag)
			dump_initcall_time();
	#endif
#endif
	/*
	 * Call remove function of all devices with a removal flag set.
	 * This may be useful for last-stage operations, like cancelling
	 * of DMA operation or releasing device internal buffers.
	 */
	dm_remove_devices_flags(DM_REMOVE_ACTIVE_ALL | DM_REMOVE_NON_VITAL);

	/* Remove all active vital devices next */
	dm_remove_devices_flags(DM_REMOVE_ACTIVE_ALL);

#ifdef CONFIG_ARMV8_MULTIENTRY
	gd->flags &= ~GD_FLG_SMP;
	cli_release_lock(0);
#endif
	cleanup_before_linux();
}

static void setup_start_tag (struct bd_info *bd)
{
	params = (struct tag *)bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}

static void setup_memory_tags(struct bd_info *bd)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;

		params = tag_next (params);
	}
}

static void setup_commandline_tag(struct bd_info *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}

static void setup_initrd_tag(struct bd_info *bd, ulong initrd_start,
			     ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}

static void setup_serial_tag(struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}

static void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}

static void setup_end_tag(struct bd_info *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}

__weak void setup_board_tags(struct tag **in_params) {}

#ifndef CONFIG_AMLOGIC_MODIFY
#ifdef CONFIG_ARM64
static void do_nonsec_virt_switch(void)
{
	smp_kick_all_cpus();
	dcache_disable();	/* flush cache before swtiching to EL2 */
}
#endif
#endif

/*
 * suffix kernel bootargs to bootargs
 *
 */
 #ifdef CONFIG_AMLOGIC_MODIFY
static void add_kernel_bootargs(struct bootm_headers *images)
{
	int node, len_args_uboot, len_args_dts, len;
	char *bootargs_dts, *bootargs_uboot;
	char *bootargs_new;
	char  *p, *q, *t, *z, *s;
	static char kerenl_dts_update;
	char buf[256];

	if (kerenl_dts_update)
		return;

	kerenl_dts_update = 1;
	node = fdt_path_offset(images->ft_addr, "/chosen");
	if (node < 0) {
		printf("Can't find /chosen node from DTB\n");
		return;
	}
	bootargs_dts = (char *)fdt_getprop(images->ft_addr, node, "bootargs", &len);
	if (!bootargs_dts) {
		printf("Can't find bootargs property in chosen\n");
		return;
	}
	bootargs_uboot = env_get("bootargs");
	if (!bootargs_uboot)
		return;

	len_args_uboot = strlen(bootargs_uboot);
	len_args_dts = strlen(bootargs_dts);
	len = len_args_uboot + len_args_dts + 2;

	bootargs_new = (char *)malloc(len);
	if (!bootargs_new) {
		printf("fail to malloc new bootargs memory\n");
		return;
	}

	memcpy(bootargs_new, bootargs_uboot, len_args_uboot);
	bootargs_new[len_args_uboot] = ' ';
	memcpy(bootargs_new + len_args_uboot + 1, bootargs_dts, len_args_dts);
	bootargs_new[len - 1] = '\0';
	p = bootargs_new;
	while ((q = strchr(p, ' '))) {
		if (q == p) {
			p = q + 1;
			continue;
		}
		memset(buf, 0, sizeof(buf));
		if ((q - p) > sizeof(buf)) {
			memcpy(buf, p, sizeof(buf));
			printf("%s is oversized\n", buf);
			p = q + 1;
			continue;
		}
		memcpy(buf, p, q - p);
		len = strlen(buf);
		p = q + 1;
		z = p;
		while (z) {
			t = strstr(z, buf);
			if (t) {
				s = t;
				if (strchr(s, ' ') && (*(t + len) == ' ')) {
					memmove(t, t + len, z + strlen(z) - t - len + 1);
					z = t;
				} else {
					if (strlen(t) == len) {
						memmove(t, t + len, z + strlen(z) - t - len + 1);
						break;
					} else {
						z = t + len;
					}
				}
			} else {
				break;
			}
		}
	}

	env_set("bootargs", bootargs_new);
	free(bootargs_new);
}

/*
 * kernel 5.15 limit boot env number to 32, there are lots of unused/default
 * zero envs which may cause boot failed in kernel. So remove these envs
 */
static void fix_bootargs(void)
{
	static char const *remove_list[] = {
		"dolby_status=",
		"dolby_vision_on=",
		"hdr_policy=",
		"hdr_priority=",
		"disable_ir=",
		"lcd_debug=",
		"recovery_offset=",
		"hdmi_read_edid=",
		"ramoops.pstore_en=",
		"ramoops.record_size=",
		"ramoops.console_size="
	};
	int i, len, rlen, find;
	char *cmdline, *p, *q;
	char buf[64];
	unsigned long value;

	cmdline = env_get("bootargs");
	if (!cmdline)
		return;

	p      = cmdline;
	debug("bootargs:%s\n", cmdline);
	while (*p) {
		/* get an arg */
		q = strchr(p, ' ');
		if (!q)
			break;
		rlen = strlen(q);
		find = 0;
		/* match remove args */
		for (i = 0; i < ARRAY_SIZE(remove_list); i++) {
			len = strlen(remove_list[i]);
			if (!memcmp(p, remove_list[i], len)) {
				/* copy this env to temp buffer and check it's value */
				memset(buf, 0, sizeof(buf));
				memcpy(buf, p, q - p);
				if (buf[len] == ' ') { /* empty one */
					find = 1;
				} else {
					value = -1UL;
					str2long(buf + len, &value);
					if (!value ||
						!strncmp(p, "ramoops.pstore_en=", len) ||
						!strncmp(p, "ramoops.record_size=", len) ||
						!strncmp(p, "ramoops.console_size=", len))
						find = 1;
				}
				if (find) {
					memmove(p, q + 1, rlen);
					//pr_info("remove env:%s\n", buf);
					break;
				}
			}
		}
		if (find)
			continue;
		else
			p = q + 1;
	}
	debug("new boot args:%s\n", cmdline);
	/* update env */
	env_set("bootargs", cmdline);
}
#endif /* CONFIG_AMLOGIC_MODIFY */

__weak void board_prep_linux(struct bootm_headers *images) { }

/* Subcommand: PREP */
static void boot_prep_linux(struct bootm_headers *images)
{
	char *commandline = env_get("bootargs");

	if (CONFIG_IS_ENABLED(OF_LIBFDT) && CONFIG_IS_ENABLED(LMB) && images->ft_len) {
		debug("using: FDT\n");
		if (image_setup_linux(images)) {
			panic("FDT creation failed!");
		}
	} else if (BOOTM_ENABLE_TAGS) {
		debug("using: ATAGS\n");
		setup_start_tag(gd->bd);
		if (BOOTM_ENABLE_SERIAL_TAG)
			setup_serial_tag(&params);
		if (BOOTM_ENABLE_CMDLINE_TAG)
			setup_commandline_tag(gd->bd, commandline);
		if (BOOTM_ENABLE_REVISION_TAG)
			setup_revision_tag(&params);
		if (BOOTM_ENABLE_MEMORY_TAGS)
			setup_memory_tags(gd->bd);
		if (BOOTM_ENABLE_INITRD_TAG) {
			/*
			 * In boot_ramdisk_high(), it may relocate ramdisk to
			 * a specified location. And set images->initrd_start &
			 * images->initrd_end to relocated ramdisk's start/end
			 * addresses. So use them instead of images->rd_start &
			 * images->rd_end when possible.
			 */
			if (images->initrd_start && images->initrd_end) {
				setup_initrd_tag(gd->bd, images->initrd_start,
						 images->initrd_end);
			} else if (images->rd_start && images->rd_end) {
				setup_initrd_tag(gd->bd, images->rd_start,
						 images->rd_end);
			}
		}
		setup_board_tags(&params);
		setup_end_tag(gd->bd);
	} else {
		panic("FDT and ATAGS support not compiled in\n");
	}

	board_prep_linux(images);
}

__weak bool armv7_boot_nonsec_default(void)
{
#ifdef CONFIG_ARMV7_BOOT_SEC_DEFAULT
	return false;
#else
	return true;
#endif
}

#ifdef CONFIG_ARMV7_NONSEC
bool armv7_boot_nonsec(void)
{
	char *s = env_get("bootm_boot_mode");
	bool nonsec = armv7_boot_nonsec_default();

	if (s && !strcmp(s, "sec"))
		nonsec = false;

	if (s && !strcmp(s, "nonsec"))
		nonsec = true;

	return nonsec;
}
#endif

#ifdef CONFIG_ARM64
__weak void update_os_arch_secondary_cores(uint8_t os_arch)
{
}

#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
static void switch_to_el1(void)
{
	if ((IH_ARCH_DEFAULT == IH_ARCH_ARM64) &&
	    (images.os.arch == IH_ARCH_ARM))
		armv8_switch_to_el1(0, (u64)gd->bd->bi_arch_number,
				    (u64)images.ft_addr, 0,
				    (u64)images.ep,
				    ES_TO_AARCH32);
	else
		armv8_switch_to_el1((u64)images.ft_addr, 0, 0, 0,
				    images.ep,
				    ES_TO_AARCH64);
}
#endif
#endif

/* Subcommand: GO */
#ifdef CONFIG_AMLOGIC_MODIFY
extern void jump_to_a32_kernel(unsigned long, unsigned long, unsigned long);
#endif
static void boot_jump_linux(struct bootm_headers *images, int flag)
{
#ifdef  CONFIG_AML_UPDATE_PDVFS
	run_command("update_pdvfs", 0);
	run_command("update_cooling_state", 0);
#endif

	if (IS_ENABLED(CONFIG_CMD_SCMI_SHMEM_ADDR))
		run_command("update_scmi_shmem", 0);

#ifdef  CONFIG_KEY_PRESERVE
	(*((volatile unsigned int *)(STARTUP_KEY_PRESERVE))) |= 0x1;
#endif

#ifdef CONFIG_ARM64
	void (*kernel_entry)(void *fdt_addr, void *res0, void *res1,
			void *res2);
	int fake = (flag & BOOTM_STATE_OS_FAKE_GO);

#ifdef CONFIG_AMLOGIC_MODIFY
	unsigned long machid = 0xf81;
#endif
#ifdef CONFIG_ARMV8_MULTIENTRY
	int waitcount = 0;
#endif
	kernel_entry = (void (*)(void *fdt_addr, void *res0, void *res1,
				void *res2))images->ep;

	debug("## Transferring control to Linux (at address %lx)...\n",
		(ulong) kernel_entry);
	bootstage_mark(BOOTSTAGE_ID_RUN_OS);

#ifdef CONFIG_AML_KASLR_SEED
	int node, ret, len;
	char *prop, *bootargs;
	uint64_t seed;

	node = fdt_path_offset(images->ft_addr, "/chosen");
	if (node < 0)
		printf("Can't find /chosen node from DTB\n");

	bootargs = (char *)fdt_getprop(images->ft_addr, node, "bootargs", &len);
	if (!bootargs)
		printf("Can't find bootargs property in chosen\n");

	char *env = env_get("ramdump_enable");

	if ((bootargs && strstr(bootargs, "ramoops_io_en=1")) || (env && (env[0] == '1'))) {
		ret = fdt_appendprop_string(images->ft_addr, node, "bootargs", " nokaslr");
		if (!ret)
			printf("Not enable kaslr for debug purpose\n");
		else
			printf("Fail to set nokaslr %s\n", fdt_strerror(ret));
	} else {
		prop = (char *)fdt_getprop(images->ft_addr, node, "kaslr-seed", NULL);
		if (!prop) {
			printf("Can't find kaslr-seed property in chosen\n");
		} else {
			srand(timer_get_us());
			/*
			 * random() function use hardware RNG, not software, ignore
			 * coverity weak cryptor report.
			 */
			/* coverity[dont_call] */
			seed = (uint64_t)rand();
			//printf("--leo-- seed 0x%llx\n", seed);

			ret = fdt_setprop(images->ft_addr, node, "kaslr-seed", &seed, sizeof(seed));
			if (!ret)
				printf("Enable kaslr\n");
			else
				printf("Can't set kaslr-seed value in chosen\n");
		}
	}
#endif

	announce_and_cleanup(fake);

	if (!fake) {
#ifndef CONFIG_AMLOGIC_MODIFY
#ifdef CONFIG_ARMV8_PSCI
		armv8_setup_psci();
#endif
		do_nonsec_virt_switch();

		update_os_arch_secondary_cores(images->os.arch);

#ifdef CONFIG_ARMV8_SWITCH_TO_EL1
		armv8_switch_to_el2((u64)images->ft_addr, 0, 0, 0,
				    (u64)switch_to_el1, ES_TO_AARCH64);
#else
		if ((IH_ARCH_DEFAULT == IH_ARCH_ARM64) &&
		    (images->os.arch == IH_ARCH_ARM))
			armv8_switch_to_el2(0, (u64)gd->bd->bi_arch_number,
					    (u64)images->ft_addr, 0,
					    (u64)images->ep,
					    ES_TO_AARCH32);
		else
			armv8_switch_to_el2((u64)images->ft_addr, 0, 0, 0,
					    images->ep,
					    ES_TO_AARCH64);
#endif
#else
#ifdef CONFIG_ARMV8_MULTIENTRY
		while (cpu_online_status() & 0xfffffffe) {
			waitcount++;
			if (waitcount >= timeout_cout) {
				if (!is_secondary_core_power_on())
					break;
				else
					waitcount = 0;
			} else {
				mdelay(1);
			}
		}
#endif

		PUSH_TIME_TE(__func__, BL33_BOOT_KERNEL_s);
		extern uint32_t get_time(void);
		printf("uboot time: %u us\n", get_time());
		DUMP_TE();
		if (images->os.arch == IH_ARCH_ARM) {
			printf("boot 32bit kernel\n");
			jump_to_a32_kernel(images->ep, machid, (unsigned long)images->ft_addr);
		}
		else {
			printf("boot 64bit kernel\n");
			kernel_entry(images->ft_addr, NULL, NULL, NULL);
		}
#endif
	}

#else
	unsigned long machid = gd->bd->bi_arch_number;
	char *s;
	void (*kernel_entry)(int zero, int arch, uint params);
	unsigned long r2;
	int fake = (flag & BOOTM_STATE_OS_FAKE_GO);

	kernel_entry = (void (*)(int, int, uint))images->ep;
#ifdef CONFIG_CPU_V7M
	ulong addr = (ulong)kernel_entry | 1;
	kernel_entry = (void *)addr;
#endif
	s = env_get("machid");
	if (s) {
		if (strict_strtoul(s, 16, &machid) < 0) {
			debug("strict_strtoul failed!\n");
			return;
		}
		printf("Using machid 0x%lx from environment\n", machid);
	}

	debug("## Transferring control to Linux (at address %08lx)" \
		"...\n", (ulong) kernel_entry);
	bootstage_mark(BOOTSTAGE_ID_RUN_OS);
	announce_and_cleanup(fake);

	if (CONFIG_IS_ENABLED(OF_LIBFDT) && images->ft_len)
		r2 = (unsigned long)images->ft_addr;
	else
		r2 = gd->bd->bi_boot_params;

	if (!fake) {
#ifdef CONFIG_ARMV7_NONSEC
		if (armv7_boot_nonsec()) {
			armv7_init_nonsec();
			secure_ram_addr(_do_nonsec_entry)(kernel_entry,
							  0, machid, r2);
		} else
#endif
			kernel_entry(0, machid, r2);
	}
#endif
}

/* Main Entry point for arm bootm implementation
 *
 * Modeled after the powerpc implementation
 * DIFFERENCE: Instead of calling prep and go at the end
 * they are called if subcommand is equal 0.
 */
int do_bootm_linux(int flag, int argc, char *const argv[],
		   struct bootm_headers *images)
{
#ifdef CONFIG_AMLOGIC_MODIFY
	add_kernel_bootargs(images);
	fix_bootargs();
#endif

	/* No need for those on ARM */
	if (flag & BOOTM_STATE_OS_BD_T || flag & BOOTM_STATE_OS_CMDLINE)
		return -1;

	if (flag & BOOTM_STATE_OS_PREP) {
		boot_prep_linux(images);
		return 0;
	}

	if (flag & (BOOTM_STATE_OS_GO | BOOTM_STATE_OS_FAKE_GO)) {
		boot_jump_linux(images, flag);
		return 0;
	}
	boot_prep_linux(images);
	boot_jump_linux(images, flag);
	return 0;
}

#if defined(CONFIG_BOOTM_VXWORKS)
void boot_prep_vxworks(struct bootm_headers *images)
{
#if defined(CONFIG_OF_LIBFDT)
	int off;

	if (images->ft_addr) {
		off = fdt_path_offset(images->ft_addr, "/memory");
		if (off > 0) {
			if (arch_fixup_fdt(images->ft_addr))
				puts("## WARNING: fixup memory failed!\n");
		}
	}
#endif
	cleanup_before_linux();
}

void boot_jump_vxworks(struct bootm_headers *images)
{
#if defined(CONFIG_ARM64) && defined(CONFIG_ARMV8_PSCI)
	armv8_setup_psci();
	smp_kick_all_cpus();
#endif

	/* ARM VxWorks requires device tree physical address to be passed */
	((void (*)(void *))images->ep)(images->ft_addr);
}
#endif
