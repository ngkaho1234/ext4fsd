/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#pragma once

#include "drv_types.h"

__u32 drv_crc32(__u32 crc, const void *buf, size_t size);
__u32 drv_crc32c(__u32 crc, const void *buf, size_t size);
