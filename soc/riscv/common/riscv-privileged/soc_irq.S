/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * common interrupt management code for riscv SOCs supporting the riscv
 * privileged architecture specification
 */
#include <zephyr/kernel_structs.h>
#include <offsets.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/riscv/irq.h>

/*
 * __soc_handle_irq is defined as .weak to allow re-implementation by
 * SOCs that do not truly follow the riscv privilege specification.
 */
WTEXT(__soc_handle_irq)

/*
 * SOC-specific function to handle pending IRQ number generating the interrupt.
 * Exception number is given as parameter via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	/* Clear exception number from CSR mip register */
	li t1, 1
	sll t0, t1, a0
	csrrc t1, mip, t0

	/* Return */
	ret

/*
 * __soc_is_irq is defined as .weak to allow re-implementation by
 * SOCs that do not truly follow the riscv privilege specification.
 */
WTEXT(__soc_is_irq)

/*
 * SOC-specific function to determine if the exception is the result of a
 * an interrupt or an exception
 * return 1 (interrupt) or 0 (exception)
 *
 */
SECTION_FUNC(exception.other, __soc_is_irq)
	/* Read mcause and check if interrupt bit is set */
	csrr t0, mcause
	li t1, RISCV_MCAUSE_IRQ_BIT
	and t0, t0, t1

	/* If interrupt bit is not set, return with 0 */
	addi a0, x0, 0
	beqz t0, not_interrupt
	addi a0, a0, 1

not_interrupt:
	/* return */
	ret
