/*
 * General FatFs Interface
 *
 * File Name:   fs_intr.h
 * Author:      Yang Yujun
 * E-mail:      <yujiantianhu@163.com>
 * Created on:  2024.11.03
 *
 * Copyright (c) 2024   Yang Yujun <yujiantianhu@163.com>
 *
 */

#ifndef __FS_INTR_H
#define __FS_INTR_H

#ifdef __cplusplus
    extern "C" {
#endif

/*!< The includes */
#include <common/generic.h>
#include <common/api_string.h>
#include <common/io_stream.h>
#include <platform/fwk_kobj.h>
#include <platform/block/fwk_gendisk.h>

/*!< The defines */
struct fs_stream
{
    kchar_t *full_name;
    kuint32_t mode;
    struct fwk_inode *sprt_dnode;
    struct fwk_block_device_oprts *sprt_bops;

    void *private_data;
};

/*!< The functions */
extern struct fs_stream *file_open(const kchar_t *name, kuint32_t mode);
extern void file_close(struct fs_stream *sprt_fs);
extern kssize_t file_write(struct fs_stream *sprt_fs, const void *buf, kusize_t size);
extern kssize_t file_read(struct fs_stream *sprt_fs, void *buf, kusize_t size);
extern kssize_t file_size(struct fs_stream *sprt_fs);
extern kint32_t file_lseek(struct fs_stream *sprt_fs, kuint32_t offset);
extern kssize_t file_tell(struct fs_stream *sprt_fs);

#ifdef __cplusplus
    }
#endif

#endif /* __FS_INTR_H */
