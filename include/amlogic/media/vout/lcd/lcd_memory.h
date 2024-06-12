/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Amlogic, Inc. All rights reserved.
 */

#ifndef __AML_LCD_MEMORY_H__
#define __AML_LCD_MEMORY_H__

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

int lrm_get_by_name(char *name, u64 *pa, u32 *size);
int lrm_tee_protect_by_name(char *name, u32 type, s32 sec);
void *lrm_phys_to_virt(phys_addr_t paddr, u32 size);
phys_addr_t lrm_phys_alloc(u32 size, char *desc);
phys_addr_t lrm_phys_alloc_tail(u32 size, char *desc);
phys_addr_t lrm_phys_alloc_align(u32 size, u32 align, char *desc);
phys_addr_t lrm_phys_alloc_tail_align(u32 size, u32 align, char *desc);
void lrm_phys_free(phys_addr_t paddr);
void lrm_virt_free(void *vaddr);
void lrm_free(void *va, phys_addr_t pa);
void *lrm_alloc(u32 size, phys_addr_t *paddr, char *desc);
void *lrm_alloc_tail(u32 size, phys_addr_t *paddr, char *desc);
void *lrm_alloc_align(u32 size, phys_addr_t *paddr, u32 align, char *desc);
void *lrm_alloc_tail_align(u32 size, phys_addr_t *paddr, u32 align, char *desc);
void lrm_show(void);
void lrm_handle_mem_info_to_kernel(void);
void lrm_bootargs_put_string(char *name, char *value);
void lrm_bootargs_put_number(char *name, u32 value);
int lcd_reserved_memory_init(char *dt_addr);
u32 lrm_get_tcon_rsvd_size(void);
void lcd_reserved_memory_deinit(void);
int lrm_exist(void);

#endif

