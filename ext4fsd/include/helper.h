/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#pragma once

#include <ntifs.h>

#include "drv_common\drv_types.h"
#include "drv_common\drv_atomic.h"
#include "drv_common\drv_lock.h"

static __u64 bcb_get_block_no(void *bcb)
{
	return ((PPUBLIC_BCB)bcb)->MappedFileOffset.QuadPart *
				((PPUBLIC_BCB)bcb)->MappedFileOffset.QuadPart;
}