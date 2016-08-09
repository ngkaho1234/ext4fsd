/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#pragma once

/* Use AVL table instead of SPLAY table */
#ifndef RTL_USE_AVL_TABLES
 #define RTL_USE_AVL_TABLES
#endif

#include "helper.h"
#include "drv_common\drv_tree.h"

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
 * @brief Node type of in-tree entries
 */
enum jbd2_node_type {
	JBD2_NODE_LBCB,
	JBD2_NODE_REVOKE
};

/**
 * @brief Node header of in-tree entries
 */
struct jbd2_node_hdr {
	enum jbd2_node_type	th_node_type;	/* Node type of entry */
	jbd2_fsblk_t			th_block;		/* Block nr. of this block pointing to */
	__bool				th_newly;		/* The entry is just newly allocated */
	drv_atomic_t			th_refcount;	/* Reference counter of the object */

	RB_ENTRY(
		jbd2_node_hdr)	th_node;		/* Tree node */
};

/**
 * @brief Logged BCB
 */
typedef struct jbd2_lbcb {
	struct jbd2_node_hdr	jl_header;			/* Node header of LBCB */
	void *				jl_bcb;			/* The bcb to be logged */
	void *				jl_data;			/* Data field of bcb logged */

	struct jbd2_txn *		jl_txn;			/* The transaction this LBCB belongs to */
	struct jbd2_txn *		jl_cp_txn;			/*
										 * The the most recent transaction on checkpoint
										 * queue this LBCB belonged to 
										 */

	LIST_ENTRY			jl_txn_list_node;	/* Chain node of lbcb within a transaction */
	LIST_ENTRY			jl_cp_txn_list_node;	/*
										 * Chain node of lbcb within the most recent
										 * transaction on checkpoint
										 */
} jbd2_lbcb_t;

/**
 * @brief  Revoke entry
 */
typedef struct jbd2_revoke_entry {
	struct jbd2_node_hdr	re_header;		/* Node header of LBCB */
	jbd2_tid_t				re_tid;			/*
										 * For any transaction id smaller
										 * than trans_id, records of @block
										 * in those transactions should not
										 * be replayed
										 */
} jbd2_revoke_entry_t;

/**
 * @brief JBD2 transaction unit
 */
typedef struct jbd2_txn {
	jbd2_tid_t				jt_tid;			/* Transaction ID */
	jbd2_logblk_t			jt_start_blk;		/* Start of log block */
	jbd2_logblk_t			jt_reserved_cnt;	/* Reserved block count of this transaction */
	drv_atomic_t			jt_logged_cnt;		/* Logged block count of this transaction */

	enum jbd2_txn_state	jt_state;			/* State of transaction */
	drv_mutex_t			jt_lock;			/* Lock of the transaction */

	LIST_ENTRY			jt_lbcb_list;		/* List of LBCB held by this transaction */
	drv_atomic_t			jt_unwritten_cnt;	/* Unwritten block count of this transaction */

	struct jbd2_handle *	jt_handle;			/* The log handle this transaction belongs to */
	LIST_ENTRY			jt_list_node;		/* List node */
} jbd2_txn_t;

/**
 * @brief Handle to JBD2 transaction handle
 */
typedef struct jbd2_txn_handle {
	__u8				th_uuid[UUID_SIZE];	/* UUID of the client */
	jbd2_logblk_t		th_reserved_cnt;		/* Reserved block count of this transaction handle */
	jbd2_txn_t *		th_txn;				/* Transaction unit */
} jbd2_txn_handle_t;

typedef RB_HEAD(jbd2_generic_table, jbd2_node_hdr) jbd2_generic_table_t;

/**
 * @brief JBD2 log handle
 */
typedef struct jbd2_handle {
	PFILE_OBJECT			jh_log_file;		/* Log file handle */

	drv_mutex_t			jh_lock;			/* Lock of the handle */

	__u32				jh_blocksize;		/* Block size of log file */
	__u32				jh_blockcnt;		/* Size of of log file in blocks */
	__u8					jh_uuid[UUID_SIZE];	/* UUID of journal */
	__u32				jh_max_txn;		/* Limit of journal blocks per trans */

	jbd2_txn_t *			jh_running_txn;	/* Current running transaction */
	LIST_ENTRY			jh_txn_queue;		/* A queue of transaction committed */

	jbd2_logblk_t			jh_free_start;		/* Start of unused blocknr of journal */
	jbd2_logblk_t			jh_free_end;		/* End of unused blocknr of journal */
	jbd2_logblk_t			jh_free_blockcnt;	/* Nr. of free blocks in journal */

	journal_superblock_t *	jh_sb;			/* Superblock buffer */

	jbd2_generic_table_t	jh_lbcb_table;		/* LBCB table logged by JBD2 */
	jbd2_generic_table_t	jh_revoke_table;	/* Revoke table */

	NPAGED_LOOKASIDE_LIST	jh_lbcb_cache;		/* Allocation cache for lbcb */
	NPAGED_LOOKASIDE_LIST	jh_revoke_cache;	/* Allocation cache for revoke entries */

	void					(*after_commit)(	/* After commit callback */
							struct jbd2_handle *handle,
							jbd2_txn_t *txn
						);
} jbd2_handle_t;

/* JBD2 pool tags */
#define JBD2_POOL_TAG			'2BDJ'
#define JBD2_SUPERBLOCK_TAG		'BS2J'
#define JBD2_RECOVER_POOL_TAG	'ER2J'
#define JBD2_LBCB_TABLE_TAG		'TL2J'
#define JBD2_REVOKE_TABLE_TAG		'TR2J'

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
