#pragma once

/*
 * Define integer types for our use
 */

typedef unsigned __int8		__u8;
typedef signed   __int8		__s8;

typedef signed   __int16		__u16;
typedef unsigned __int16	__s16;

typedef signed   __int32		__u32;
typedef unsigned __int32	__s32;

typedef signed   __int64		__u64;
typedef unsigned __int64	__s64;

typedef __u16				__le16;
typedef __u32				__le32;
typedef __u64				__le64;

typedef __u16				__be16;
typedef __u32				__be32;
typedef __u64				__be64;

 typedef BOOLEAN			__bool;

#include "ext4_endian.h"

typedef __u64				ext4_fsblk_t;
typedef __u32				ext4_lblk_t;
typedef __u32				ext4_grpblk_t;
typedef __u32				ext4_ino_t;
typedef __u32				ext4_group_t;

struct ext4_timespec {
	__s64 tv_sec;			/* Seconds */
	__s64 tv_nsec;			/* Nanoseconds */
};

typedef __u32				ext4_uid_t;
typedef __u32				ext4_gid_t;
typedef __u32				ext4_umode_t;

#include "ext4_atomic.h"