/*
 * Driver definitions reside in this header file
 */

#pragma once

#include <ntifs.h>

#include "ext4_types.h"
#include "ext4_atomic.h"

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
	atomic_t				v_refcount;	/* Reference counter */

	struct ext4_super_block	v_sb;
};

/*
 * Inode control block
 */
struct ext4_icb {
	enum ext4_nid			i_nid;			/* Identifier for this structure */
	atomic_t				i_refcount;	/* Reference counter */

	__s64				i_size;		/* File size */
	struct ext4_timespec	i_atime;		/* Access time */
	struct ext4_timespec	i_ctime;		/* Inode change time */
	struct ext4_timespec	i_mtime;		/* Modification time */
	struct ext4_timespec	i_crtime;		/* Creation time */
	struct ext4_inode *		i_buf;		/* On-disk inode buffer */

	struct ext4_vcb *		i_vcb;		/* The volume this ICB belongs to */
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
	__bool					ic_top_level;		/* If the request is top level */
	WORK_QUEUE_ITEM	ic_wq_item;		/* Used if the request needs to be queued for later processing */
	__bool					ic_in_exception;	/* If an exception is currently in progress */
	NTSTATUS			ic_exception;		/* The exception code when an exception is in progress */
};

EXTERN_C_START

DRIVER_INITIALIZE DriverEntry;

EXTERN_C_END
