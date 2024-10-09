// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */
#include <amlogic/page_info.h>
#include <amlogic/storage.h>
#include <nand.h>

struct boot_info *page_info;
#ifdef CONFIG_AML_SPI_NFC
extern unsigned char infopage_force_hostecc;
extern unsigned char disable_host_ecc;
#endif

extern struct mtd_info *mtd_store_get(int dev);

unsigned char page_info_get_data_lanes_mode(void)
{
	return page_info->dev_cfg0.bus_width & 0x0f;
}

unsigned char page_info_get_cmd_lanes_mode(void)
{
	return page_info->dev_cfg1.ca_lanes & 0x0f;
}

unsigned char page_info_get_addr_lanes_mode(void)
{
	return (page_info->dev_cfg1.ca_lanes >> 4) & 0x0f;
}

unsigned char page_info_get_frequency_index(void)
{
	return page_info->host_cfg.frequency_index;
}

unsigned char page_info_get_adj_index(void)
{
	return page_info->host_cfg.mode_rx_adj & 0x3f;
}

unsigned char page_info_get_work_mode(void)
{
	return (page_info->host_cfg.mode_rx_adj >> 6) & 0x3;
}

unsigned char page_info_get_line_delay1(void)
{
	return page_info->host_cfg.lines_delay[0];
}

unsigned char page_info_get_line_delay2(void)
{
	return page_info->host_cfg.lines_delay[1];
}

unsigned char page_info_get_core_div(void)
{
	return page_info->host_cfg.core_div;
}

unsigned char page_info_get_bus_cycle(void)
{
	return page_info->host_cfg.bus_cycle;
}

unsigned char page_info_get_device_ecc_disable(void)
{
	return page_info->host_cfg.device_ecc_disable & 0x01;
}

unsigned int page_info_get_n2m_command(void)
{
	return page_info->host_cfg.n2m_cmd;
}

unsigned int page_info_get_page_size(void)
{
	return page_info->dev_cfg0.page_size;
}

unsigned char page_info_get_planes(void)
{
	return  page_info->dev_cfg0.planes_per_lun & 0x0f;
}

unsigned char page_info_get_plane_shift(void)
{
	return (page_info->dev_cfg0.planes_per_lun >> 4) & 0x0f;
}

unsigned char page_info_get_cache_plane_shift(void)
{
	return (page_info->dev_cfg0.bus_width >> 4) & 0x0f;
}

unsigned char page_info_get_cs_deselect_time(void)
{
	return page_info->dev_cfg1.cs_deselect_time;
}

unsigned char page_info_get_dummy_cycles(void)
{
	return page_info->dev_cfg1.dummy_cycles;
}

unsigned int page_info_get_block_size(void)
{
	return page_info->dev_cfg1.block_size;
}

unsigned short *page_info_get_bbt(void)
{
	return &page_info->dev_cfg1.bbt[0];
}

unsigned char page_info_get_enable_bbt(void)
{
	return page_info->dev_cfg1.enable_bbt;
}

unsigned char page_info_get_high_speed_mode(void)
{
	return page_info->dev_cfg1.high_speed_mode;
}

unsigned char page_info_get_layout_method(void)
{
	return page_info->boot_layout.layout_method;
}

unsigned int page_info_get_boot_size(void)
{
	return page_info->boot_layout.boot_size;
}

unsigned int page_info_get_pages_in_block(void)
{
	unsigned int block_size, page_size;
	static unsigned int pages_in_block;

	if (pages_in_block)
		return pages_in_block;

	block_size = page_info_get_block_size();
	page_size = page_info_get_page_size();
	pages_in_block = block_size / page_size;

	return pages_in_block;
}

unsigned int page_info_get_pages_in_boot(void)
{
	unsigned int page_size, boot_size;

	page_size = page_info_get_page_size();
	boot_size = page_info_get_boot_size();

	return boot_size / page_size;
}

void page_info_initialize(unsigned int default_n2m,
			  unsigned char bus_width, unsigned char ca)
{
	memset((unsigned char *)page_info,
		0, sizeof(struct boot_info));
	page_info->dev_cfg0.page_size = sizeof(struct boot_info);
	page_info->dev_cfg0.planes_per_lun = 0;
	page_info->dev_cfg0.bus_width = bus_width;
	page_info->host_cfg.frequency_index = 0xFF;
	page_info->host_cfg.n2m_cmd = default_n2m;
	page_info->dev_cfg1.ca_lanes = ca;
	page_info->dev_cfg1.cs_deselect_time = 0xFF;
	page_info->dev_cfg1.dummy_cycles = 0xFF;
}

int page_info_version_init(unsigned char boot_layout)
{
	cpu_id_t cpu_id = get_cpu_id();

	switch (cpu_id.family_id) {
	case MESON_CPU_MAJOR_ID_A4:
	case MESON_CPU_MAJOR_ID_S1A:
		page_info->version = PAGE_INFO_V3;
		break;
	case MESON_CPU_MAJOR_ID_C3:
		page_info->version = PAGE_INFO_V2;
		break;
	case MESON_CPU_MAJOR_ID_A1:
	case MESON_CPU_MAJOR_ID_C1:
	case MESON_CPU_MAJOR_ID_C2:
	case MESON_CPU_MAJOR_ID_S4:
	case MESON_CPU_MAJOR_ID_S4D:
	case MESON_CPU_MAJOR_ID_S5:
	case MESON_CPU_MAJOR_ID_SC2:
		page_info->version = PAGE_INFO_V1;
		break;
	default:
		page_info->version = PAGE_INFO_V3;
		break;
	}
	page_info->version |= (boot_layout << 4);

	return page_info->version & 0x0F;
}

static void calc_checksum(struct boot_info *boot_info)
{
	u8 *buf = (u8 *)boot_info;
	u32 i, checksum = 0;

	boot_info->checksum = 0;
	for (i = 0; i < sizeof(struct boot_info); i++)
		checksum += buf[i];
	boot_info->checksum = checksum;

	printf("bootinfo checksum : 0x%x\n", boot_info->checksum);
}

void page_info_init_from_mtd_and_dts(struct mtd_info *mtd,
					    struct udevice *udev)
{
	unsigned char ecc_steps;
	enum PAGE_INFO_V page_info_ver;
	u32 boot_layout;

#ifdef CONFIG_MTD_SPI_NAND
	enum boot_type_e medium_type = store_get_type();
	struct nand_device *dev = mtd_to_nanddev(mtd);

	if (medium_type == BOOT_SNAND) {
		struct dm_spi_slave_plat *plat;
		#ifdef CONFIG_DDR_PARAMETER_SUPPORT
		struct spinand_device *spinand = mtd_to_spinand(mtd);
		unsigned int pages_shift, ddr_param_page;
		#endif
		plat = dev_get_parent_plat(udev);
		page_info->dev_cfg0.bus_width &= ~0x03;
		if (plat->mode & SPI_RX_QUAD)
			page_info->dev_cfg0.bus_width |= 2;
		else if (plat->mode & SPI_RX_DUAL)
			page_info->dev_cfg0.bus_width |= 1;
		NFC_Print("bus_width", page_info->dev_cfg0.bus_width);
		page_info->dev_cfg0.planes_per_lun = dev->memorg.planes_per_lun;
		if (page_info->dev_cfg0.planes_per_lun > 1) {
			page_info->dev_cfg0.planes_per_lun |= 6 << 4;
			page_info->dev_cfg0.bus_width =
				(mtd->writesize_shift + 1) << 4;
		}
	}
#endif

#ifdef BOARD_BOOT_LAYOUT_DISCRETE_BL2
	boot_layout = BOOT_DISCRETE_BL2;
#else
	boot_layout = BOOT_DISCRETE_ALL;
#endif
	page_info_ver = page_info_version_init(boot_layout);
	memcpy(page_info->magic, BOOTINFO_MAGIC, strlen(BOOTINFO_MAGIC));
	page_info->dev_cfg0.page_size = mtd->writesize;
	ecc_steps = mtd->writesize >> 9;
	page_info->host_cfg.n2m_cmd = (DEFAULT_ECC_MODE & (~0x3F)) | ecc_steps;
#ifdef CONFIG_MTD_SPI_NAND
	if (medium_type == BOOT_SNAND) {
		#ifdef CONFIG_AML_SPI_NFC
		if (disable_host_ecc)
			page_info->host_cfg.n2m_cmd = N2M_RAW | mtd->writesize;
		#endif
		page_info->host_cfg.frequency_index = 0xFF;
		page_info->dev_cfg1.ca_lanes = 0;
		page_info->dev_cfg1.cs_deselect_time = 0xFF;
		page_info->dev_cfg1.dummy_cycles = 0xFF;
	}
#endif

	page_info->dev_cfg1.block_size = mtd->erasesize;

#ifdef BOOTINFO_PROGRAMMER_SUPPORT
	page_info->dev_cfg1.is_gang_programer = 0;
	page_info->dev_cfg1.xor_bbt_start_block |= (1 << 24);
	page_info->dev_cfg1.block_num_in_chip = mtd->size / mtd->erasesize;
	if (mtd_store_get(1))
		page_info->dev_cfg1.block_num_in_chip =
			(mtd_store_get(1))->size / mtd->erasesize;
#endif
	if (meson_rsv_part_get_bl2_copy_number(mtd) > 2)
		page_info->dev_cfg1.enable_bbt = 1;
	calc_checksum(page_info);
}

#ifdef __PXP_DEBUG__
static void page_info_dump_info(void)
{
	unsigned char planes_per_lun, plane_shift, bus_width, cache_plane_shift;
	unsigned char high_speed_mode, cmd_lanes, addr_lanes;
	unsigned char enable_bbt;
	unsigned int block_size, page_size;
	unsigned char frequency_index, mode, rx_adj;
	unsigned char device_ecc_disable = 0;
	unsigned int n2m_cmd;

	planes_per_lun = page_info_get_planes();
	plane_shift = page_info_get_plane_shift();
	cache_plane_shift = page_info_get_cache_plane_shift();
	high_speed_mode = page_info_get_high_speed_mode();
	page_size = page_info_get_page_size();
	block_size = page_info_get_block_size();
	enable_bbt = page_info_get_enable_bbt();
	bus_width = page_info_get_data_lanes_mode();
	cmd_lanes = page_info_get_cmd_lanes_mode();
	addr_lanes = page_info_get_addr_lanes_mode();

	frequency_index = page_info_get_frequency_index();
	mode = page_info_get_work_mode();
	rx_adj = page_info_get_adj_index();
	device_ecc_disable = page_info_get_device_ecc_disable();
	n2m_cmd = page_info_get_n2m_command();

	pr_info("bus_width: 0x%x\n", bus_width);
	pr_info("cmd_lanes: 0x%x\n", cmd_lanes);
	pr_info("addr_lanes: 0x%x\n", addr_lanes);
	pr_info("page_size: 0x%x\n", page_size);
	pr_info("planes_per_lun: 0x%x\n", planes_per_lun);
	pr_info("plane_shift: 0x%x\n", plane_shift);
	pr_info("cache_plane_shift: 0x%x\n", cache_plane_shift);
	pr_info("block_size: 0x%x\n", block_size);
	pr_info("high_speed_mode: 0x%x\n", high_speed_mode);
	pr_info("enable_bbt: 0x%x\n", enable_bbt);

	pr_info("frequency_index: 0x%x\n", frequency_index);
	pr_info("mode: 0x%x\n", mode);
	pr_info("rx_adj: 0x%x\n", rx_adj);
	pr_info("device_ecc_disable: 0x%x\n", device_ecc_disable);
	pr_info("n2m_cmd: 0x%x\n", n2m_cmd);
	pr_info("is_gang_programer: %d\n", page_info->dev_cfg1.is_gang_programer);
	pr_info("xor_bbt_start_block: 0x%x\n", page_info->dev_cfg1.xor_bbt_start_block);
	pr_info("block_num_in_chip: %d\n", page_info->dev_cfg1.block_num_in_chip);
	pr_info("version: 0x%x\n", page_info->version);
	pr_info("layout_method: 0x%x\n", page_info->boot_layout.layout_method);
	pr_info("boot size: 0x%x\n", page_info->boot_layout.boot_size);
}
#endif

unsigned char *page_info_post_init(struct mtd_info *mtd, struct udevice *dev)
{
	page_info_init_from_mtd_and_dts(mtd, dev);
#ifdef __PXP_DEBUG__
	page_info_dump_info();
#endif
	return (unsigned char *)page_info;
}

int page_info_pre_init(void)
{
	if (!page_info) {
		page_info = kzalloc(MAX_BYTES_IN_BOOTINFO, GFP_KERNEL);
		if (!page_info)
			return -1;
	}

	return 0;
}

bool page_info_is_page(struct mtd_info *mtd, int page)
{
	enum PAGE_INFO_V page_info_ver;
	bool is_info_page = 0;
	u32 pages_per_copy =
		 meson_rsv_part_get_bl2_copy_size(mtd) / mtd->writesize;

	page_info_ver = page_info->version & 0x0F;
	if (page_info_ver == PAGE_INFO_V1)
		is_info_page = page % 128 == BL2_SIZE / 2048;
	else
		is_info_page = (!(page % pages_per_copy));

#ifdef CONFIG_AML_SPI_NFC
	if (infopage_force_hostecc) {
		if (is_info_page)
			disable_host_ecc = 0;
		else
			disable_host_ecc = 1;
	}
#endif
	return is_info_page;
}
