/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#pragma once

#include <ntifs.h>

#include "drv_common\drv_types.h"
#include "drv_common\drv_atomic.h"
#include "drv_common\drv_lock.h"

static __inline __u64 bcb_blocknr(void *bcb)
{
	return ((PPUBLIC_BCB)bcb)->MappedFileOffset.QuadPart *
				((PPUBLIC_BCB)bcb)->MappedFileOffset.QuadPart;
}

static __inline __s64 offset_to_blocknr(__s64 offset, unsigned int block_size)
{
	return offset / block_size;
}

static __inline __s64 blocknr_to_offset(__s64 block, unsigned int block_size)
{
	return block * block_size;
}

#if !DEBUG
 #define dbg_print(str, ...)  DbgPrint("[EXT4FSD][%s:%p] "##str, __FILE__, __LINE__,  __VA_ARGS__)
#else
 #define dbg_print(...)
#endif