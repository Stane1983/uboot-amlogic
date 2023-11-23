#include <nand.h>
#include <mtd.h>
#include <command.h>
#include <amlogic/zapper_boot.h>
#include <../legacy-mtd-utils.h>


static unsigned char flash_map_index = 0;
static unsigned long long hwconfig_start;	//hwconfig partition start address
static unsigned long long hwconfig_end;		//hwconfig partition end address

static unsigned long long ldflag_start;		//ldflag partition start address
static unsigned long long ldflag_end;		//ldflag partition end address

static unsigned long long ldsec_start;		//ldsec partition start address
static unsigned long long ldsec_end;		//ldsec partition end address

static unsigned long long kernel_start = 0x2240000;		//kernel partition start address
static unsigned long long kernel_end = 0x2e40000;		//kernel partition end address

//static unsigned long long kernel_header_start = 0x2240000;		//kernel partition start address
//static unsigned long long kernel_header_end = 0x2e40000;		//kernel partition end address



static unsigned char zapper_ldflag_partition[LDFLAG_LENGTH];
static unsigned char zapper_hwconfig_partition[HWCONFIG_LENGTH];
static unsigned char zapper_ldsec_partition[LDSEC_LENGTH];
static unsigned char zapper_kernel_partition[KERNEL_LENGTH];

static unsigned char zapper_kernel_header_partition[KERNEL_HEADER_LENGTH];

static struct Zapper_boot_info s_boot_info = {0};

int Zapper_get_nand_hwconfig_partition_address(unsigned long long hwconfig_s, unsigned long long hwconfig_e)
{
	if (hwconfig_s <= 0 || hwconfig_e <= 0 || hwconfig_e < hwconfig_s) {
		return ZAPPER_ERROR;
	}

	hwconfig_start = hwconfig_s;
	hwconfig_end = hwconfig_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}


int Zapper_get_nand_ldflag_partition_address(unsigned long long ldflag_s, unsigned long long ldflag_e)
{
	if (ldflag_s <= 0 || ldflag_e <= 0 || ldflag_e < ldflag_s) {
		return ZAPPER_ERROR;
	}

	ldflag_start = ldflag_s;
	ldflag_end = ldflag_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldsec_partition_address(unsigned long long ldsec_s, unsigned long long ldsec_e)
{
	if (ldsec_s <= 0 || ldsec_e <= 0 || ldsec_e < ldsec_s) {
		return ZAPPER_ERROR;
	}

	ldsec_start = ldsec_s;
	ldsec_end = ldsec_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_kernel_partition_address(unsigned long long kernel_s, unsigned long long kernel_e)
{
	if (kernel_s <= 0 || kernel_e <= 0 || kernel_e < kernel_s) {
                return ZAPPER_ERROR;
	}

	kernel_start = kernel_s;
	kernel_end = kernel_e;
	flash_map_index++;
	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}
	memcpy((void *)p_s_e_boot_info->loader_partition_header, (void *)s_boot_info.loader_partition_header , LD_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->loader_partition, (void *)s_boot_info.loader_partition , LD_LENGTH);
	memcpy((void *)p_s_e_boot_info->error_code, (void *)s_boot_info.error_code , EC_LENGTH);
	p_s_e_boot_info->modify_flag = s_boot_info.modify_flag;
	p_s_e_boot_info->reboot_flag = s_boot_info.reboot_flag;

	for (int i = 0;i < LD_HEADER_LENGTH ; i++) {
		printf ("p_s_e_boot_info->loader_partition_header[%d] is %x\n", i, *(p_s_e_boot_info->loader_partition_header + i));
	}

	for (int i = 0;i < LD_LENGTH ; i++) {
		printf ("p_s_e_boot_info->loader_partition[%d] is %x\n", i, *(p_s_e_boot_info->loader_partition + i));
	}

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_hwconfig_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->bbcb_header, (void *)s_boot_info.bbcb_header , BBCB_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->bbcb, (void *)s_boot_info.bbcb , BBCB_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_ldsec_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->uk_header, (void *)s_boot_info.uk_header , UK_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->uk, (void *)s_boot_info.uk , UK_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_get_nand_kernel_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}

	memcpy((void *)p_s_e_boot_info->kernel_header, (void *)s_boot_info.kernel_header , KERNEL_HEADER_LENGTH);
	memcpy((void *)p_s_e_boot_info->kernel, (void *)s_boot_info.kernel , KERNEL_LENGTH);

	return ZAPPER_SUCCESS;
}

int Zapper_set_nand_ldflag_partition_info(struct Zapper_boot_info *p_s_e_boot_info)
{
	if (p_s_e_boot_info == NULL) {
		return ZAPPER_ERROR;
	}
	memcpy((void *)zapper_ldflag_partition, (void *)p_s_e_boot_info->loader_partition_header , LD_HEADER_LENGTH);
	memcpy((void *)zapper_ldflag_partition + LD_HEADER_LENGTH, (void *)p_s_e_boot_info->loader_partition , LD_LENGTH);
	memcpy((void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH , (void *)p_s_e_boot_info->error_code , EC_LENGTH);
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH] = p_s_e_boot_info->modify_flag;
	zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1] = p_s_e_boot_info->reboot_flag;

	memcpy((void *)s_boot_info.loader_partition_header, (void *)zapper_ldflag_partition, LD_HEADER_LENGTH);
	memcpy((void *)s_boot_info.loader_partition, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH , LD_LENGTH);
	memcpy((void *)s_boot_info.error_code, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH, EC_LENGTH);
	s_boot_info.modify_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH];
	s_boot_info.reboot_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1];

	return ZAPPER_SUCCESS;
}

static int Zapper_read_all_info(struct Zapper_boot_info *p_s_boot_info)
{
	if (p_s_boot_info == NULL || flash_map_index < 2) {
		printf("[ZAPPER] flash_map_index < 2,The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);
		return ZAPPER_ERROR;
	}

	int ret = ZAPPER_ERROR;
	struct mtd_info *mtd = NULL;
	unsigned long rwsize;

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	if (!mtd) {
		puts("\n[ZAPPER]no devices available\n");
		return ZAPPER_ERROR;
	}

    printf("[ZAPPER] The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	rwsize = LDFLAG_LENGTH;
	ret = nand_read_skip_bad(mtd, ldflag_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldflag_partition);

	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_ldflag_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}
	for (int i = 0;i < LDFLAG_LENGTH ; i++) {
		printf ("zapper_ldflag_partition[%d] is %x\n", i, *(zapper_ldflag_partition + i));

	}

	rwsize = HWCONFIG_LENGTH;
	ret = nand_read_skip_bad(mtd, hwconfig_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_hwconfig_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_hwconfig_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}

	for (int i = 0;i < HWCONFIG_LENGTH ; i++) {
		printf ("zapper_hwconfig_partition[%d] is %x\n", i, *(zapper_hwconfig_partition + i));
	}

	rwsize = LDSEC_LENGTH;
	ret = nand_read_skip_bad(mtd, ldsec_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldsec_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_ldsec_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}

	for (int i = 0;i < LDSEC_LENGTH ; i++) {
		printf ("zapper_ldsec_partition[%d] is %x\n", i, *(zapper_ldsec_partition + i));
	}

	rwsize = KERNEL_LENGTH;
	ret = nand_read_skip_bad(mtd, kernel_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_kernel_partition);
	if (ret) {
		printf("[ZAPPER] nand_read_skip_bad zapper_kernel_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}


	memcpy((void *)p_s_boot_info->loader_partition_header, (void *)zapper_ldflag_partition, LD_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->loader_partition, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH , LD_LENGTH);
	memcpy((void *)p_s_boot_info->error_code, (void *)zapper_ldflag_partition + LD_HEADER_LENGTH + LD_LENGTH, EC_LENGTH);
	p_s_boot_info->modify_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH];
	p_s_boot_info->reboot_flag = zapper_ldflag_partition [LD_HEADER_LENGTH + LD_LENGTH + EC_LENGTH + 1];

	memcpy((void *)p_s_boot_info->bbcb_header, (void *)zapper_hwconfig_partition, BBCB_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->bbcb, (void *)zapper_hwconfig_partition + BBCB_HEADER_LENGTH, BBCB_LENGTH);

	memcpy((void *)p_s_boot_info->uk_header, (void *)zapper_ldsec_partition, UK_HEADER_LENGTH);
	memcpy((void *)p_s_boot_info->uk, (void *)zapper_ldsec_partition + UK_HEADER_LENGTH, UK_LENGTH);

	memcpy((void *)p_s_boot_info->kernel, (void *)zapper_kernel_partition, KERNEL_LENGTH);
	memcpy((void *)p_s_boot_info->kernel_header, (void *)zapper_kernel_header_partition, KERNEL_LENGTH);

	return ZAPPER_SUCCESS;

}

static int Zapper_write_ldflag(void)
{
	int ret = ZAPPER_ERROR;
	struct mtd_info *mtd = NULL;
	unsigned long rwsize;
	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	if (!mtd) {
		puts("\n[ZAPPER]no devices available\n");
		return ZAPPER_ERROR;
	}

    printf("[ZAPPER] The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);

	mtd = get_nand_dev_by_index(ZAPPER_FLASH_DEV);

	rwsize = LDFLAG_LENGTH;
	ret = nand_write_skip_bad(mtd, ldflag_start, &rwsize,
							 NULL, (long long)ZAPPER_FLASH_MAX_ADDRESS,
							 (u_char *)zapper_ldflag_partition,
							 0x2);

	if (ret) {
		printf("[ZAPPER] nand_write_skip_bad zapper_ldflag_partition ret = %d\n",ret);
		return ZAPPER_ERROR;
	}
	printf("[ZAPPER] after writing We will printf zapper_ldflag_partition\n");
	for (int i = 0;i < LDFLAG_LENGTH ; i++) {
		printf ("zapper_ldflag_partition[%d] is %x\n", i, *(zapper_ldflag_partition+i));

	}
	return ZAPPER_SUCCESS;
}

static int do_zapper_read_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do zapper flash read\n");
	int ret = ZAPPER_ERROR;
	ret = Zapper_read_all_info(&s_boot_info);
	return ret;
}


static int do_zapper_write_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do zapper flash read\n");
	int ret = ZAPPER_ERROR;
	run_command("nand erase.part ldflag", 0);
	ret = Zapper_write_ldflag();
	return ret;
}



U_BOOT_CMD(
	zapper_flash_read, 1, 0, do_zapper_read_flash,"zapper read" ,"zapper read for irdeto loader"
);

U_BOOT_CMD(
	zapper_flash_write, 1, 0, do_zapper_write_flash,"zapper write" ,"zapper write for irdeto loader"
);


