/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#pragma once

#include <ntifs.h>

#include "drv_common\drv_types.h"
#include "drv_common\drv_lock.h"

#include "jbd2\jbd2_fs.h"

/**
 * @brief JBD2 log handle
 */
struct jbd2_handle {
	PFILE_OBJECT	jbd2_logfile;	/* Log file handle */
};