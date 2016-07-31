/*
 * Driver definitions reside in this header file
 */

#pragma once

#include <ntifs.h>

#include "drv_common\drv_types.h"
#include "drv_common\drv_atomic.h"

typedef __u64				ext4_fsblk_t;
typedef __u32				ext4_lblk_t;
typedef __u32				ext4_grpblk_t;
typedef __u32				ext4_ino_t;
typedef __u32				ext4_group_t;

typedef __u32				ext4_uid_t;
typedef __u32				ext4_gid_t;
typedef __u32				ext4_umode_t;

#include "ext4_fs.h"

/*
 * In-core structure node ID
 */
enum ext4_nid {
	EXT4_NID_VCB		= 1603821,
	EXT4_NID_ICB,
	EXT4_NID_IRP_CTX
};

/*
* Volume control block
*/
struct ext4_vcb {
	enum ext4_nid			v_nid;		/* Identifier for this structure */
	drv_atomic_t			v_refcount;	/* Reference counter */

	struct ext4_super_block	v_sb;
};

/*
 * Inode control block
 */
struct ext4_icb {
	enum ext4_nid			i_nid;			/* Identifier for this structure */
	drv_atomic_t			i_refcount;		/* Reference counter */

	__s64				i_size;			/* File size */
	struct drv_timespec		i_atime;			/* Access time */
	struct drv_timespec		i_ctime;			/* Inode change time */
	struct drv_timespec		i_mtime;			/* Modification time */
	struct drv_timespec		i_crtime;			/* Creation time */
	struct ext4_inode *		i_buf;			/* On-disk inode buffer */

	struct ext4_vcb *		i_vcb;			/* The volume this ICB belongs to */
};

struct ext4_irp_ctx {
	enum ext4_nid			ic_nid;			/* Identifier for this structure */
	PIRP					ic_irp;			/* Pointer to the IRP this request describes */
	ULONG				ic_flags;			/* Flags */
	UCHAR				ic_major_func;		/* The major function code for the request */
	UCHAR				ic_minor_func;		/* The minor function code for the request */
	PDEVICE_OBJECT		ic_device_object;	/* The device object */
	PDEVICE_OBJECT		ic_real_device;		/* The real device object */
	PFILE_OBJECT			ic_file_object;		/* The file object */
	struct ext4_fcb *		ic_fcb;			/* Fcb object */
	__bool				ic_top_level;		/* If the request is top level */
	WORK_QUEUE_ITEM	ic_wq_item;		/* Used if the request needs to be queued for later processing */
	__bool				ic_in_exception;	/* If an exception is currently in progress */
	NTSTATUS			ic_exception;		/* The exception code when an exception is in progress */
};

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

/*
 * ext4_cachesup.c
 */
void ext4_cache_init_map(
	PFILE_OBJECT file_object,
	PCC_FILE_SIZES file_size,
	__bool pin_access,
	PCACHE_MANAGER_CALLBACKS callbacks,
	void *lazy_write_ctx);

void ext4_cache_sync_uninit_map(
	struct ext4_irp_ctx *irp_ctx,
	PFILE_OBJECT file_object);

__bool ext4_cache_pin_read(
	PFILE_OBJECT file_object,
	__s64 file_offset,
	ULONG len,
	__bool wait,
	__bool exclusive,
	void **bcb,
	void **buf);

__bool ext4_cache_pin_write(
	PFILE_OBJECT file_object,
	__s64 file_offset,
	ULONG len,
	__bool zero,
	__bool wait,
	void **bcb,
	void **buf);

void ext4_cache_set_dirty(void *bcb, __u32 lsn);

void ext4_cache_unpin_bcb(void *bcb);

void ext4_cache_repin_bcb(void *bcb);

void ext4_cache_unpin_repinned_bcb(void *bcb);

/*
 * ext4_txn.c
 */

EXTERN_C_END
