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

#include "drv_endian.h"

struct drv_timespec {
	__s64 tv_sec;			/* Seconds */
	__s64 tv_nsec;			/* Nanoseconds */
};

#include "drv_atomic.h"