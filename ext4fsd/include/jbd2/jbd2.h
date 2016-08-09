/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#pragma once

#include "helper.h"

#include "jbd2_fs.h"

/*
 * Definitions of standard types used by jbd2
 */
typedef __u32	jbd2_logblk_t;
typedef __u64	jbd2_fsblk_t;
typedef __u32	jbd2_tid_t;

/**
 * @brief	State of transaction
 */
enum jbd2_txn_state {
	TXN_RUNNING,
	TXN_LOCKED,
	TXN_COMMITTING,
	TXN_CHECKPOINT,
};

struct jbd2_txn;
struct jbd2_handle;

/**
 * @brief Logged BCB
 */
typedef struct jbd2_lbcb {
	void *			jl_bcb;			/* The bcb to be logged */
	void *			jl_data;			/* Data field of bcb logged */

	struct jbd2_txn *	jl_txn;			/* The transaction this LBCB belongs to */

	LIST_ENTRY		jl_txn_list_node;	/* Chain node of lbcb within a transaction */
	LIST_ENTRY		jl_txn_queue_node;	/*
									 * Queue node of lbcb pointing to same
									 * filesystem block
									 */
} jbd2_lbcb_t;

/**
 * @brief  Revoke entry
 */
typedef struct jbd2_revoke_entry {
	jbd2_fsblk_t	re_block;	/* Block number not to be replayed */
	jbd2_tid_t		re_tid;	/*
						 * For any transaction id smaller
						 * than trans_id, records of @block
						 * in those transactions should not
						 * be replayed
						 */
} jbd2_revoke_entry_t;

/**
 * @brief JBD2 transaction handle
 */
typedef struct jbd2_txn {
	jbd2_tid_t				jt_tid;			/* Transaction ID */
	jbd2_logblk_t			jt_start_blk;		/* Start of log block */
	jbd2_logblk_t			jt_reserved_cnt;	/* Reserved block count of this transaction */
	jbd2_logblk_t			jt_logged_cnt;		/* Logged block count of this transaction */

	enum jbd2_txn_state	jt_state;			/* State of transaction */
	drv_mutex_t			jt_lock;			/* Lock of the transaction */

	LIST_ENTRY			jt_lbcb_list;		/* List of LBCB held by this transaction */

	struct jbd2_handle *	jt_handle;			/* The log handle this transaction belongs to */
	LIST_ENTRY			jt_list_node;		/* List node */
} jbd2_txn_t;

/**
 * @brief JBD2 log handle
 */
typedef struct jbd2_handle {
	PFILE_OBJECT			jh_log_file;		/* Log file handle */

	drv_mutex_t			jh_lock;			/* Lock of the handle */
	KEVENT				jh_event;			/* Wait on the KEVENT till operation is finished */

	__u32				jh_blocksize;		/* Block size of log file */
	__u32				jh_blockcnt;		/* Size of of log file in blocks */
	__u8					jh_uuid[16];		/* UUID of journal */
	__u32				jh_max_txn;		/* Limit of journal blocks per trans */

	jbd2_txn_t *			jh_running_txn;	/* Current running transaction */
	LIST_ENTRY			jh_txn_queue;		/* A queue of transaction committed */

	journal_superblock_t *	jh_sb;			/* Superblock buffer */

	RTL_AVL_TABLE		jh_block_table;		/* Block table logged by JBD2 */
	RTL_AVL_TABLE		jh_revoke_table;	/* Revoke table */

	void (*after_commit)(					/* After commit callback */
			struct jbd2_handle *handle,
			jbd2_txn_t *txn
		);
} jbd2_handle_t;

#define JBD2_POOL_TAG '2BDJ'
#define JBD2_SUPERBLOCK_TAG 'BS2J'
#define JBD2_RECOVER_POOL_TAG 'ER2J'

/* jbd2_cachesup.c */

__bool jbd2_cc_acquire_for_lazywrite(
		void *context,
		__bool wait
);

void jbd2_cc_release_from_lazywrite(
		void *context
);

__bool jbd2_cc_acquire_for_readahead(
		void *context,
		__bool wait
);

void jbd2_cc_release_from_readahead(
		void *context
);
