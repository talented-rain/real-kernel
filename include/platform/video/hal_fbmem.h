/*
 * Framebuffer Driver Top Interface
 *
 * File Name:   hal_fbmem.h
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2023.07.01
 *
 * Copyright (c) 2023   Yang Yujun <yujiantianhu@163.com>
 *
 */

#ifndef __HAL_FBMEM_H_
#define __HAL_FBMEM_H_

/*!< The includes */
#include <platform/hal_basic.h>
#include <platform/hal_platform.h>

/*!< The defines */
/*!< Maximum supported resolution */
#define HAL_FB_MAX_DPI									(1920 * 1080)
/*!< Each pixel supports up to 32 bits */
#define HAL_FB_MAX_BPP									(32)

/*!< 
 * The RGB bit field of the fb cache, which describes how each pixel display buffer is organized, 
 * in RGB565 mode, R takes 5 bits = bit[11:15] G takes 6 bits = bit[10:5] B takes 5 bits = bit[4:0] 
 */
struct hal_fb_bitfield
{
	kuint32_t offset;        								/*!< Bitfield offset: red = 11, green = 5, blue = 0 */
	kuint32_t length;       								/*!< Bitfield lenth: red = 5,  green = 6, blue = 5 */
	kuint32_t msb_right; 									/*!< (msb_right != 0) ===> MSB on the right. Generally, this value is set to 0 */
};

/*!< LCD fix parameters */
struct hal_fb_fix_screen_info
{
	kstring_t id[16];										/*!< LCD identifier in the form of a string */
	kuaddr_t smem_start;									/*!< The starting physical address of the framebuffer memory */
	kuint32_t smem_len;										/*!< The length of the framebuffer memory */
	kuint32_t type;											/*!< The type of frame buffer storage, i.e. the way in which image data is stored in the frame buffer */
	kuint32_t type_aux;										/*!< Subtype of the framebuffer store */
	kuint32_t visual;										/*!< Screen color mode, such as black and white mode, true color mode, etc */

	kuint32_t line_lenth;									/*!< The number of bytes in a row */
};

/*!< LCD variable parameters */
struct hal_fb_var_screen_info
{
	kuint32_t xres;											/*!< Visual screen resolution: x-direction width */
	kuint32_t yres;											/*!< Visual screen resolution: y-direction width */
	kuint32_t xres_virtual;									/*!< Virtual screen resolution: x-direction width */
	kuint32_t yres_virtual;									/*!< Virtual screen resolution: y-direction width */
	kuint32_t xoffset;										/*!< X-Direction: The offset between the virtual screen and the visual screen */
	kuint32_t yoffset;										/*!< Y-Direction: The offset between the virtual screen and the visual screen */

	kuint32_t bits_per_pixel;								/*!< bpp, The number of bits required per pixel, such as 16, 24, 32, ... */

	struct hal_fb_bitfield red;								/*!< RGB red bit field */
	struct hal_fb_bitfield green;							/*!< RGB green bit field */
	struct hal_fb_bitfield blue;							/*!< RGB blue bit field */
	struct hal_fb_bitfield transp;							/*!< RGB deaphaneity bit field, which is only present when (bpp == 32) */

	kuint32_t height;										/*!< Screen height (unit: ms) */
	kuint32_t width;										/*!< Screen width (unit: ms) */

	kuint32_t pixclock;										/*!< Pixel clock, unit: ps(picoseconds) */

	/*!<
	 * The time (unit: pixclock) from the end of the line synchronization signal hsync to the start of drawing a new line image is actually HBP, 
	 * the horizontal synchronization trailing edge
	 */
	kuint32_t left_margin;
	/*!<
	 * The time (unit: pixclock) from the end of one line of image drawing to the start of the line synchronization signal of the next line is actually HFP, 
	 * the horizontal synchronization front
	 */
	kuint32_t right_margin;
	/*!<
	 * The time from the end of the frame synchronization signal vsync to the start of drawing a new image (unit: pixclock) is actually VBP, 
	 * vertical synchronization trailing edge
	 */
	kuint32_t upper_margin;
	/*!<
	 * The time (unit: pixclock) from the end of one frame to the start of the frame synchronization signal of the next frame is actually VFP, 
	 * vertical Sync Frontier
	 */
	kuint32_t lower_margin;

	kuint32_t hsync_len;									/*!< The pulse width (unit: pixclock) of the horizontal synchronization signal hsync, i.e. hpw */
	kuint32_t vsync_len;									/*!< The pulse width (unit: PixClock) of the V-sync signal vsync, i.e. VPW */
};

enum __ERT_HAL_FB_IOCTL_CMD
{
	Ert_HAL_FB_IOGetVScreenInfo	= HAL_IOWR('F', 0, struct hal_fb_var_screen_info),
	Ert_HAL_FB_IOPutVScreenInfo = HAL_IOWR('F', 1, struct hal_fb_var_screen_info),
	Ert_HAL_FB_IOGetFScreenInfo = HAL_IOWR('F', 2, struct hal_fb_fix_screen_info),
};

struct hal_fb_info
{
	ksint32_t node;											/*!< minor */
	ksint32_t flags;

	struct hal_fb_fix_screen_info sgrt_fix;					/*!< fix parameters */
	struct hal_fb_var_screen_info sgrt_var;					/*!< variadics */

	const struct hal_fb_oprts *sprt_fbops;					/*!< operation API */
	struct hal_device *sprt_device;							/*!< device */

	kuint8_t *ptr_screen_base;								/*!< base address of the virtual memory */
	kuint32_t screen_size;									/*!< virtual memory size */

	void *ptr_par;											/*!< private data of the framebuffer device */
};

struct hal_fb_oprts
{
	ksint32_t (*fb_open) (struct hal_fb_info *sprt_info, ksint32_t user);
	ksint32_t (*fb_release) (struct hal_fb_info *sprt_info, ksint32_t user);

	kssize_t (*fb_read) (struct hal_fb_info *sprt_info, ksbuffer_t *ptr_buf, kusize_t count, kuint8_t *ptr_offset);
	kssize_t (*fb_write) (struct hal_fb_info *sprt_info, const ksbuffer_t *ptr_buf, kusize_t count, kuint8_t *ptr_offset);
	ksint32_t (*fb_ioctl) (struct hal_fb_info *sprt_info, kuint32_t cmd, kuint64_t arg);
	ksint32_t (*fb_mmap) (struct hal_fb_info *sprt_info, struct hal_vm_area *vma);
};

/*!< The functions */
TARGET_EXT struct hal_fb_info *hal_framebuffer_alloc(kusize_t size, struct hal_device *sprt_dev);
TARGET_EXT void hal_framebuffer_release(struct hal_fb_info *sprt_fb_info);
TARGET_EXT ksint32_t hal_register_framebuffer(struct hal_fb_info *sprt_fb_info);
TARGET_EXT void hal_unregister_framebuffer(struct hal_fb_info *sprt_fb_info);
TARGET_EXT struct hal_fb_info *hal_get_fb_info(kuint32_t idx);
TARGET_EXT struct hal_fb_info *hal_file_fb_info(struct hal_file *sprt_file);

static inline void *hal_fb_get_drvdata(struct hal_fb_info *sprt_fb_info)
{
	return sprt_fb_info ? sprt_fb_info->ptr_par : mrt_nullptr;
}

#endif /*!< __HAL_FBMEM_H_ */
