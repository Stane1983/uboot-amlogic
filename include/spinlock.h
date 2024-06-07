/* SPDX-License-Identifier: GPL-2.0
 * (C) Copyright 2013
 */
#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_
#include <asm/system.h>

typedef struct {
	volatile u32 lock;
} spin_lock_t;

#define UNLOCK 0
#define LOCK 1

/* Current Program Status Register(CPSR) */
#define CPSR_N_FLAG			(0x01 << 31)
#define CPSR_Z_FLAG			(0x01 << 30)
#define CPSR_C_FLAG			(0x01 << 29)
#define CPSR_V_FLAG			(0x01 << 28)
#define CPSR_Q_FLAG			(0x01 << 27)
#define CPSR_IT_1_0			(0x03 << 25)
#define CPSR_JAZELLE			(0x01 << 24)
/* [23:20] RAZ/SBZP		*/
#define CPSR_GE				(0x01 << 16)
#define CPSR_IT_7_2			(0x3F << 10)
#define CPSR_E_STATE		(0x01 << 9)	/* Endianness execution state. 0:Little, 1:Big */
#define CPSR_A_DISABLE		(0x01 << 8)	/* Asynchronous abort disable bit */
#define CPSR_I_DISABLE		(0x01 << 7)	/* Interrupt disable bit */
#define CPSR_F_DISABLE		(0x01 << 6)	/* Fast interrupt disable bit */
#define CPSR_T_STATE		(0x01 << 5)	/* Thumb execution state bit */
#define CPSR_MODE			(0x1F << 0)	/* The current mode of the processor */

#define CPSR_MODE_USER			0x10
#define CPSR_MODE_FIQ			0x11
#define CPSR_MODE_IRQ			0x12
#define CPSR_MODE_SVC			0x13
#define CPSR_MODE_ABT			0x17
#define CPSR_MODE_UND			0x1B
#define CPSR_MODE_SYS			0x1F

#define INIT_SPIN_LOCK		{.lock = UNLOCK}

/*
 * RTK FIXME: there is a the same name definition in io.h.
 * So in orderto prevent compiler error, we comment this function temporarily.
 */
static inline unsigned long local_irq_flags(void)
{
	unsigned long flags;

	asm volatile ("mrs    %0, daif        // arch_local_irq_save\n"
		: "=r" (flags)
		:
		: "memory", "cc"
	);
	return flags;
}

static inline int is_irq_on(void)
{
	if (local_irq_flags() & CPSR_I_DISABLE)
		return 0;
	return 1;
}

static inline unsigned long arch_local_irq_save(void)
{
	unsigned long flags;

	asm volatile("mrs	%0, daif		/// save arch local irq\n"
		"msr	daifset, #2"
		: "=r" (flags)
		:
		: "memory");
	return flags;
}

static inline void arch_local_irq_enable(void)
{
	asm volatile("msr	daifclr, #2		// enable arch local irq\n"
		:
		:
		: "memory");
}

static inline void arch_local_irq_disable(void)
{
	asm volatile("msr	daifset, #2		// disable arch local irq\n"
		:
		:
		: "memory");
}

/*
 * restore saved IRQ state
 */
static inline void arch_local_irq_restore(unsigned long flags)
{
	asm volatile("msr	daif, %0		// restore arch local irq\n"
		:
		: "r" (flags)
		: "memory");
}

static inline unsigned long save_cpsr(void)
{
	return arch_local_irq_save();
}

static inline void restore_cpsr(unsigned long flags)
{
	arch_local_irq_restore(flags);
}

static inline void irq_enable(void)
{
	arch_local_irq_enable();
}

static inline void irq_disable(void)
{
	arch_local_irq_disable();
}

#ifdef CONFIG_ARMV8_MULTIENTRY
/* Below functions should be defined in arch */
void arch_spin_lock(void *lock);
int arch_try_lock(void *lock);
void arch_spin_unlock(void *lock);

static inline void spin_lock_init(spin_lock_t *lock)
{
	lock->lock = UNLOCK;
}

static inline void spin_lock(spin_lock_t *lock)
{
	arch_spin_lock((void *)&lock->lock);
}

static inline int spin_trylock(spin_lock_t *lock)
{
	return arch_try_lock((void *)&lock->lock);
}

static inline void spin_unlock(spin_lock_t *lock)
{
	arch_spin_unlock((void *)&lock->lock);
}

#define spin_lock_irq(lock)	    { irq_disable(); spin_lock(lock); }
#define spin_unlock_irq(lock)	{ spin_unlock(lock); irq_enable(); }

#define spin_lock_save(lock, flags)	{ flags = save_cpsr(); spin_lock(lock); }
#define spin_unlock_restore(lock, flags) { spin_unlock(lock); restore_cpsr(flags); }

#else
#define spin_lock_init(lock)	{(void)lock; }
#define spin_lock(lock)		{(void)lock; }
#define spin_trylock(lock)	(1)
#define spin_unlock(lock)	{(void)lock; }

#define spin_lock_irq(lock)	do {irq_disable(); (void)lock; } while (0)
#define spin_unlock_irq(lock)	do {(void)lock; irq_enable(); } while (0)
#define spin_lock_save(lock, flags)		do {flags = save_cpsr(); (void)lock; } while (0)
#define spin_unlock_restore(lock, flags)	do {(void)lock; restore_cpsr(flags); } while (0)
#endif
#endif  //_SPINLOCK_H_

