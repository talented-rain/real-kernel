/*
 * Memory Allocate Management
 *
 * File Name:   mem_kalloc.c
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2023.10.02
 *
 * Copyright (c) 2023   Yang Yujun <yujiantianhu@163.com>
 *
 */

/*!< The includes */
#include <boot/boot_text.h>
#include <platform/fwk_mempool.h>

/*!< The globals */
struct mem_info sgrt_kernel_mem_info =
{
	.base	= 0,
	.lenth	= 0,

	.sprt_mem = mrt_nullptr,
};

/*!< API function */
/*!
 * @brief   fwk_mempool_initial
 * @param   none
 * @retval  none
 * @note    kernel memory block initial
 */
kbool_t fwk_mempool_initial(void)
{
	struct mem_info *sprt_info = &sgrt_kernel_mem_info;

	if (isValid(sprt_info->sprt_mem))
		return false;

	memory_simple_block_create(sprt_info, 
							   MEMORY_POOL_BASE, 
							   MEMORY_POOL_SIZE);

	return true;
}

/*!
 * @brief   memory_block_self_defines
 * @param   none
 * @retval  none
 * @note    memory block initial
 */
kbool_t memory_block_self_defines(kuaddr_t base, kusize_t size)
{
	struct mem_info *sprt_info = &sgrt_kernel_mem_info;

	if (isValid(sprt_info->sprt_mem))
		return false;

	memory_simple_block_create(sprt_info, base, size);

	return true;
}

/*!
 * @brief   memory_block_self_destroy
 * @param   none
 * @retval  none
 * @note    memory block destroy
 */
void memory_block_self_destroy(void)
{
	memory_simple_block_destroy(&sgrt_kernel_mem_info);
}

/*!
 * @brief   kmalloc
 * @param   __size
 * @retval  none
 * @note    kernel memory pool allocate
 */
__weak void *kmalloc(size_t __size, ert_fwk_mempool_t flags)
{
	void *p = mrt_nullptr;
	
	if (flags & GFP_KERNEL)
		p = alloc_spare_simple_memory(sgrt_kernel_mem_info.sprt_mem, __size);

	if (!isValid(p))
		return mrt_nullptr;

	if (flags & GFP_ZERO)
		memory_reset(p, __size);

	return p;
}

/*!
 * @brief   kzalloc
 * @param   __size
 * @retval  none
 * @note    kernel memory pool allocate, and reset automatically
 */
__weak void *kzalloc(size_t __size, ert_fwk_mempool_t flags)
{
	return kmalloc(__size, flags | GFP_ZERO);
}

/*!
 * @brief   kfree
 * @param   __ptr
 * @retval  none
 * @note    kernel memory pool free
 */
__weak void kfree(void *__ptr)
{
	struct mem_info *sprt_info = &sgrt_kernel_mem_info;

	if (__ptr < (void *)sprt_info->base)
		return;
	if (__ptr >= (void *)(sprt_info->base + sprt_info->lenth))
		return;

	free_employ_simple_memory(sprt_info->sprt_mem, __ptr);
}

/* end of file */
