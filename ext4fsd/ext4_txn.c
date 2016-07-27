/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "ext4.h"
#include "ext4_data.h"

/*
 * NOTES:
 *	1. If a transaction does not have definite amount of blocks to be preserved,
 *	    that transaction will be committed (flushed) immediately when it is closed.
 */


void ext4_txn_start(
		struct ext4_irp_ctx *irp_ctx,
		ext4_lblk_t blk_cnt)
{
}

void ext4_txn_stop(struct ext4_irp_ctx *irp_ctx)
{
}