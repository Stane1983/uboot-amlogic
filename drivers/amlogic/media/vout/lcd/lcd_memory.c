// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#include <common.h>
#include <malloc.h>
#include <dm.h>
#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
#include <amlogic/tee_aml.h>
#endif
#include <amlogic/media/vout/lcd/aml_lcd.h>
#include <amlogic/media/vout/lcd/lcd_memory.h>

#define LRMERR LCDERR
#define LRMPR LCDPR

#ifndef GFP_KERNEL
#define GFP_KERNEL (0)
#endif

/*
 * LCD_ALLOC_FRONT is seek free memory  from low to high
 * LCD_ALLOC_TAIL is seek free memory  from high to low
 * for these options:
 * for lcd probe, may use reserved memory and free while probe done
 * and for this kind of memorys we wish they alloced at high,
 * because we can think of them as unused memorys, and release them later in delay work
 */
#define LCD_ALLOC_FRONT 0 //form low to high
#define LCD_ALLOC_TAIL 1 //from high to low

#define RSVD_MEM_TAIL_SIZE (PAGE_SIZE * 1) ////4k for save uboot used information and args
#define RSVD_MEM_UBOOT_INFO_SIZE (RSVD_MEM_TAIL_SIZE / 2)// for save uboot used information
#define RSVD_MEM_BOOTARGS_SIZE   (RSVD_MEM_TAIL_SIZE - RSVD_MEM_UBOOT_INFO_SIZE)// for args

#define LCD_ATTR_SEC     BIT(1) //secure memory
#define LCD_ATTR_TAIL    BIT(31)

#define LCD_RSVD_MEM_MAGIC "amlogic_lcd_reserved"

#define LCD_MEM_MAGIC_LEN 32
#define LCD_MEM_INFO_SIZE 64

#define RSVD_FREE_UNUSED_DLY (120 * 1000)

struct lcd_mem_info_s {
	u32 offset;
	u32 size;
	u32 attr;
	u32 sec_handle;
	char name[LCD_MEM_INFO_SIZE - 16];
}; //64 byte

struct lcd_mem_list_s {
	struct list_head list;
	struct lcd_mem_info_s info;
};

struct lcd_rsvd_mem_s {
	char magic[LCD_MEM_MAGIC_LEN];  //magic words fixed 'amlogic_lcd_reserved'
	u32 size;              //total memory size
	u32 allocated_num;     // how many memorys are alloced
	u64 pstart;            //whole reserved memory physic start
	u32 args_len;
	u32 args_num;
	u32 tcon_size;
	char *bootargs;
	struct list_head mem_list;
};

struct lcd_rsvd_mem_s *lcd_reserved_memory;

static inline int list_is_first(const struct list_head *list,
				const struct list_head *head)
{
	return list->prev == head;
}

__weak void *kzalloc(unsigned int size, unsigned int flag)
{
	void *mem = NULL;

	mem = malloc(size);
	if (mem)
		memset(mem, 0, size);

	return mem;
}

__weak void kfree(void *mem)
{
	if (mem)
		free(mem);
}

static inline void lrm_spin_lock(void)
{
	;
}

static inline void lrm_spin_unlock(void)
{
	;
}

static inline struct lcd_rsvd_mem_s *lrm_get(void)
{
	return lcd_reserved_memory;
}

static struct lcd_mem_info_s *lrm_info_get_by_name(char *name)
{
	struct lcd_rsvd_mem_s *lrm = lrm_get();
	struct lcd_mem_list_s *pos;
	struct list_head *head;

	if (!lrm || list_empty(&lrm->mem_list))
		return NULL;

	head = &lrm->mem_list;
	list_for_each_entry(pos, head, list)
		if (strcmp(pos->info.name, name) == 0)
			return &pos->info;

	return NULL;
}

int lrm_get_by_name(char *name, u64 *pa, u32 *size)
{
	struct lcd_mem_info_s *mem_info;
	struct lcd_rsvd_mem_s *lrm = lrm_get();

	mem_info = lrm_info_get_by_name(name);
	if (lrm  && mem_info) {
		if (pa)
			*pa = lrm->pstart + mem_info->offset;
		if (size)
			*size = mem_info->size;

		return 0;
	}

	return -1;
}

/*==============================================================================================*/
int lrm_tee_protect(struct lcd_mem_info_s *mem_info, u32 type, s32 sec)
{
#if (IS_ENABLED(CONFIG_AMLOGIC_TEE))
	struct lcd_rsvd_mem_s *lrm = lrm_get();
	u64 paddr;
	u32 size;
	int ret = 0;

	if (!lrm || !mem_info)
		return -1;

	paddr = lrm->pstart + mem_info->offset;
	size = mem_info->size;
	if (sec && !(mem_info->attr & LCD_ATTR_SEC)) {
		/* user flush manually if needed
		 * flush_dcache_range(paddr, size);
		 */
		ret = tee_protect_mem(type, 0, paddr, size, &mem_info->sec_handle);

		if (ret) {
			LRMERR("%s: protect failed! start:0x%llx, size:0x%x, ret:%d\n",
			       mem_info->name, paddr, size, ret);
			return -1;
		}
		mem_info->attr |= LCD_ATTR_SEC;
	} else if (!sec && (mem_info->attr & LCD_ATTR_SEC)) {
		tee_unprotect_mem(mem_info->sec_handle);
		mem_info->attr &= ~LCD_ATTR_SEC;
	}
#endif
	return 0;
}

int lrm_tee_protect_by_name(char *name, u32 type, s32 sec)
{
	return lrm_tee_protect(lrm_info_get_by_name(name), type, sec);
}

int lrm_paddr_to_offset(phys_addr_t paddr)
{
	struct lcd_rsvd_mem_s *lrm = lrm_get();

	return (lrm && paddr >= lrm->pstart) ? (paddr - lrm->pstart) : -1;
}

/*==============================================================================================*/
void *lrm_phys_to_virt(phys_addr_t paddr, u32 size)
{
	return (void *)paddr;
}

phys_addr_t __lrm_phys_alloc(u32 size, u32 align, u32 dir, char *desc)
{
	struct lcd_rsvd_mem_s *lrm;
	struct lcd_mem_list_s *mem, *l = NULL, *r = NULL, *s, *e;
	struct list_head *head;
	u32 start = 0, temp, tail_rsvd_size = 0;
	u64 pstart, paddr = 0;

	tail_rsvd_size = RSVD_MEM_TAIL_SIZE;

	lrm = lrm_get();
	if (!lrm || !size)
		return 0;

	mem = kzalloc(sizeof(*mem), GFP_KERNEL);
	if (!mem)
		return 0;

	s = kzalloc(sizeof(*s) * 2, GFP_KERNEL);
	if (!s) {
		kfree(mem);
		return 0;
	}

	mem->info.size = ALIGN(size, PAGE_SIZE);
	mem->info.attr = 0;
	if (desc) {
		strncpy(mem->info.name, desc, sizeof(mem->info.name));
		mem->info.name[sizeof(mem->info.name) - 1] = '\0';
	}
	align = ALIGN(align, PAGE_SIZE);
	lrm_spin_lock();

	if (lrm->size - tail_rsvd_size < size) {
		kfree(mem);
		kfree(s);
		lrm_spin_unlock();
		return 0;
	}

	s->info.offset = 0;
	s->info.size = 0;
	s->info.attr = 0;
	e = s + 1;
	e->info.offset = lrm->size - tail_rsvd_size;
	e->info.size = 0;
	e->info.attr = LCD_ATTR_TAIL;

	head = &lrm->mem_list;
	list_add(&s->list, head);
	list_add_tail(&e->list, head);
	pstart = lrm->pstart;
	if (dir == LCD_ALLOC_FRONT) {
		list_for_each_entry_safe(l, r, head, list) {
			temp = l->info.attr & LCD_ATTR_TAIL;
			if (list_is_last(&l->list, head) || temp)
				goto lrm_alloc_exit;

			start = ALIGN(l->info.offset + l->info.size, align);
			if (start + size <= r->info.offset) {
				mem->info.offset = start;
				paddr = pstart + start;
				lrm->allocated_num++;
				list_add(&mem->list, &l->list);
				goto lrm_alloc_exit;
			}
		}
	} else {
		list_for_each_entry_safe_reverse(r, l, head, list) {
			temp = r->info.attr & LCD_ATTR_TAIL;
			if (list_is_first(&r->list, head) || !temp)
				goto lrm_alloc_exit;

			start = ALIGN_DOWN(r->info.offset - size, align);
			if (start >= l->info.offset + l->info.size) {
				mem->info.offset = start;
				mem->info.attr |= LCD_ATTR_TAIL;
				paddr = pstart + start;
				lrm->allocated_num++;
				list_add(&mem->list, &l->list);
				goto lrm_alloc_exit;
			}
		}
	}

lrm_alloc_exit:
	list_del(&s->list);
	list_del(&e->list);
	lrm_spin_unlock();
	kfree(s);
	if (!paddr) {
		LRMERR("%s %s size:0x%x, align:%u fail\n", __func__, desc, size, align);
		kfree(mem);
	}

	return (phys_addr_t)paddr;
}

//alloc paddr PAGE_ALIGNED
phys_addr_t lrm_phys_alloc(u32 size, char *desc)
{
	return __lrm_phys_alloc(size, PAGE_SIZE, LCD_ALLOC_FRONT, desc);
}

//alloc paddr PAGE_ALIGNED
phys_addr_t lrm_phys_alloc_tail(u32 size, char *desc)
{
	return __lrm_phys_alloc(size, PAGE_SIZE, LCD_ALLOC_TAIL, desc);
}

//align must be PAGE_ALIGNED
phys_addr_t lrm_phys_alloc_align(u32 size, u32 align, char *desc)
{
	return __lrm_phys_alloc(size, align, LCD_ALLOC_FRONT, desc);
}

//align must be PAGE_ALIGNED
phys_addr_t lrm_phys_alloc_tail_align(u32 size, u32 align, char *desc)
{
	return __lrm_phys_alloc(size, align, LCD_ALLOC_TAIL, desc);
}

/* remove memory node for other to use */
void lrm_phys_free(phys_addr_t paddr)
{
	struct lcd_rsvd_mem_s *lrm;
	struct lcd_mem_list_s *temp, *pos;
	struct list_head *head;

	unsigned int offset = 0xffffffff;

	lrm = lrm_get();
	if (!lrm ||  list_empty(&lrm->mem_list))
		return;

	offset = lrm_paddr_to_offset(paddr);

	head = &lrm->mem_list;
	lrm_spin_lock();
	list_for_each_entry_safe(pos, temp, head, list) {
		if (pos->info.offset == offset) {
			list_del(&pos->list);
			lrm->allocated_num--;
			if (pos->info.attr & LCD_ATTR_SEC) {
				LRMPR("unprotect before release\n");
				lrm_tee_protect(&pos->info, 0, 0);
			}
			kfree(pos);
			break;
		}
	}

	lrm_spin_unlock();
}

/*unmap vaddr is vaddr is mapped by vmap */
void lrm_virt_free(void *vaddr)
{
}

/*
 * unmap vaddr if va valid
 * free paddr if pa valid
 */
void lrm_free(void *va, phys_addr_t pa)
{
	if (va)
		lrm_virt_free(va);
	if (pa)
		lrm_phys_free(pa);
}

void *__lrm_alloc(u32 size, phys_addr_t *paddr, u32 align, u32 dir, char *desc)
{
	phys_addr_t pa;
	void *va;

	*paddr = 0;
	pa = __lrm_phys_alloc(size, align, dir, desc);
	va = lrm_phys_to_virt(pa, size);
	if (!va)
		lrm_phys_free(pa);
	*paddr = pa;

	return va;
}

void *lrm_alloc(u32 size, phys_addr_t *paddr, char *desc)
{
	return __lrm_alloc(size, paddr, PAGE_SIZE, LCD_ALLOC_FRONT, desc);
}

void *lrm_alloc_tail(u32 size, phys_addr_t *paddr, char *desc)
{
	return __lrm_alloc(size, paddr, PAGE_SIZE, LCD_ALLOC_TAIL, desc);
}

void *lrm_alloc_align(u32 size, phys_addr_t *paddr, u32 align, char *desc)
{
	return __lrm_alloc(size, paddr, align, LCD_ALLOC_FRONT, desc);
}

void *lrm_alloc_tail_align(u32 size, phys_addr_t *paddr, u32 align, char *desc)
{
	return __lrm_alloc(size, paddr, align, LCD_ALLOC_TAIL, desc);
}

/*==============================================================================================*/
void lrm_show(void)
{
	struct lcd_rsvd_mem_s *lrm = lrm_get();
	struct lcd_mem_list_s *pos;
	struct list_head *head;
	u64 pa;
	char *p;
	u32 offset, size, total = 0, i;

	if (!lrm)
		return;

	head = &lrm->mem_list;
	list_for_each_entry(pos, head, list) {
		pa = lrm->pstart + pos->info.offset;
		offset = pos->info.offset;
		size = pos->info.size;
		total += size;
		LRMPR("pa:0x%llx offset:%08x, size:%08x attr:0x%08x, %s\n",
		      pa, offset, size, pos->info.attr, pos->info.name);
	}

	if (lrm->bootargs) {
		LRMPR("bootargs: num:%u\n", lrm->args_num);
		p = (char *)lrm->bootargs;
		for (i = 0; i < lrm->args_num && p < lrm->bootargs + RSVD_MEM_BOOTARGS_SIZE; i++) {
			printf("%u: %s\n", i, p);
			p = p + strlen(p) + 1;
		}
	}

	LRMPR("%s pa:0x%llx, alloc_num:%u, memory used: 0x%x/0x%x, tcon_size:0x%x\n\n",
	      lrm->magic, lrm->pstart, lrm->allocated_num, total, lrm->size, lrm->tcon_size);
}

/*==============================================================================================*/
void lrm_handle_mem_info_to_kernel(void)
{
	struct lcd_rsvd_mem_s *lrm = lrm_get();
	struct lcd_mem_list_s *pos;
	struct list_head *head;
	void *p;
	int len = 0;

	if (!lrm)
		return;

	head = &lrm->mem_list;
	if (list_empty(head) || !lrm->pstart || !lrm->allocated_num)
		return;

	p = (char *)lrm->pstart + lrm->size - RSVD_MEM_TAIL_SIZE;
	memcpy(p, lrm->magic, sizeof(lrm->magic));
	len = sizeof(lrm->magic);
	*(u32 *)(p + len + 0) = lrm->size;
	*(u32 *)(p + len + 4) = lrm->allocated_num;
	*(u64 *)(p + len + 8) = lrm->pstart;
	*(u32 *)(p + len + 16) = lrm->args_num;
	/* The first 64 bytes hold basic information
	 * Each subsequent 64 bytes holds memory allocation information
	 */
	p = p + 64;
	len = 0;
	list_for_each_entry(pos, head, list) {
		memcpy(p + len, &pos->info, LCD_MEM_INFO_SIZE);
		len += LCD_MEM_INFO_SIZE;
	}
}

void lrm_bootargs_put_string(char *name, char *value)
{
	struct lcd_rsvd_mem_s *lrm = lrm_get();
	char fmt[128] = {0};
	int fmt_len = 0;

	if (!lrm || !lrm->bootargs || !name || !value)
		return;

	fmt_len = sprintf(fmt, "%s=%s", name, value);
	if (lrm->args_len + fmt_len + 1 > RSVD_MEM_BOOTARGS_SIZE)
		return;

	lrm->args_len += sprintf(lrm->bootargs + lrm->args_len, "%s", fmt);
	lrm->args_len += 1;
	lrm->args_num++;
}

void lrm_bootargs_put_number(char *name, u32 value)
{
	char fmt[32] = {0};

	sprintf(fmt, "0x%x", value);
	lrm_bootargs_put_string(name, fmt);
}

int lcd_reserved_memory_init(char *dt_addr)
{
	int parent_offset, cell_size;
	char *propdata;
	char name[128];
	struct lcd_rsvd_mem_s *lrm;
	u64 paddr = 0;
	u32 size = 0, tcon_size = 0;
	int ret = -1;

	lrm_spin_lock();
	if (lcd_reserved_memory || !dt_addr) {
		lrm_spin_unlock();
		return 0;
	}

	parent_offset = fdt_path_offset(dt_addr, "/lcd_resman");
	if (parent_offset < 0) {
		LRMERR("can't find node: /lcd_resman\n");
		goto lrm_err_exit;
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "status", NULL);
	if (!propdata || (strcmp(propdata, "ok") && strcmp(propdata, "okay"))) {
		LRMPR("warning: failed to get lcd-reserved memory from dts\n");
		goto lrm_err_exit;
	}

	parent_offset = fdt_path_offset(dt_addr, "/reserved-memory");
	if (parent_offset < 0) {
		LRMERR("can't find node: /reserved-memory\n");
		goto lrm_err_exit;
	}
	cell_size = fdt_address_cells(dt_addr, parent_offset);
	sprintf(name, "/reserved-memory/linux,lcd-reserved");

	parent_offset = fdt_path_offset(dt_addr, name);
	if (parent_offset < 0) {
		LRMERR("can't find node: %s\n", name);
		goto lrm_err_exit;
	}
	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "reg", NULL);
	if (!propdata) {
		LRMPR("warning: failed to get lcd-reserved memory from dts\n");
		goto lrm_err_exit;
	}
	if (cell_size == 2) {
		paddr = be32_to_cpup((((u32 *)propdata) + 1));
		size = be32_to_cpup((((u32 *)propdata) + 3));
	} else {
		paddr = be32_to_cpup(((u32 *)propdata));
		size = be32_to_cpup((((u32 *)propdata) + 1));
	}

	propdata = (char *)fdt_getprop(dt_addr, parent_offset, "tcon-size", NULL);
	if (!propdata) {
		LCDERR("failed to get tcon_size\n");
	} else {
		if (cell_size == 2)
			tcon_size = be32_to_cpup((((u32 *)propdata) + 1));
		else
			tcon_size = be32_to_cpup(((u32 *)propdata));
	}

	lrm = kzalloc(sizeof(*lrm), GFP_KERNEL);
	if (!lrm) {
		lrm_spin_unlock();
		return -1;
	}

	lcd_reserved_memory = lrm;
	lrm->size = size;
	lrm->allocated_num = 0;
	lrm->pstart = paddr;
	lrm->tcon_size = tcon_size;
	memset((char *)lrm->pstart, 0, lrm->size);
	INIT_LIST_HEAD(&lrm->mem_list);
	strncpy(lrm->magic, LCD_RSVD_MEM_MAGIC, 32);
	lrm->bootargs = (char *)lrm->pstart + lrm->size - RSVD_MEM_BOOTARGS_SIZE;
	ret = 0;
	lrm_spin_unlock();

	return 0;

lrm_err_exit:
	kfree(lcd_reserved_memory);
	lcd_reserved_memory = NULL;
	lrm_spin_unlock();
	return ret;
}

#define LMM_DEINIT_RETRY 20
void lcd_reserved_memory_deinit(void)
{
	struct lcd_rsvd_mem_s *lrm = NULL;
#if (IS_ENABLED(CONFIG_ARMV8_MULTIENTRY))
	int retry = LMM_DEINIT_RETRY;
#else
	int retry = 1;
#endif
	do {
		lrm_spin_lock();
		lrm = lrm_get();
		if (!lrm)
			goto lrm_deinit_exit;

		if (list_empty(&lrm->mem_list)) {
			memset(lrm, 0, sizeof(*lrm));
			kfree(lcd_reserved_memory);
			lcd_reserved_memory = NULL;
			goto lrm_deinit_exit;
		}
		lrm_spin_unlock();
		mdelay(1000);
	} while (retry--);

	if (retry <= 0)
		return;

lrm_deinit_exit:
	lrm_spin_unlock();
}

u32 lrm_get_tcon_rsvd_size(void)
{
	return  lrm_get() ? lrm_get()->tcon_size : 0;
}

int lrm_exist(void)
{
	return  lrm_get() ? 1 : 0;
}
