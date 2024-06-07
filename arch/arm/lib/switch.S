/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2013
 */

#define ENTRY(name) .globl name; .align 4; name:
#define END(name)   .size name, .-name

#define UNLOCK 0
#define LOCK 1

	.global __switch_context
__switch_context:
	cbz	x0, _next
	mov	x9, sp
	mov	x8, x0
	stp	x19, x20, [x8], #16
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9,  [x8], #16
	str	x30, [x8]

_next:
	mov	x8, x1
	ldp	x19, x20, [x8], #16
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9,  [x8], #16
	ldr	x30, [x8]
	mov	sp, x9
	ret

ENTRY(arch_spin_lock)
	mov	w2, #(LOCK)
	sevl
spin:
	wfe
1:
	ldaxr	w1, [x0]
	cbnz	w1, spin
	stxr	w1, w2, [x0]
	cbnz	w1, 1b
	dmb	ish
	ret
END(arch_spin_lock)

ENTRY(arch_spin_unlock)
	mov	w1, #(UNLOCK)
	dmb	ish
	stlr	w1, [x0]
	dsb	nsh
	ret
END(arch_spin_unlock)
