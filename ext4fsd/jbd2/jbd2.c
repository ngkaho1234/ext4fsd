/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "jbd2\jbd2.h"

enum {
	JBD2_PHASE_SCAN,
	JBD2_PHASE_SCAN_REVOKE,
	JBD2_PHASE_REPLAY
};

 /**
  * @details	Maintain information about the progress of the recovery job, so that
  *			the different passes can carry information between them.
  * @remarks	Copied from e2fsprogs/e2fsck/recovery.c
  */
struct recovery_info {
	jbd2_tid_t	ri_start_txn;
	jbd2_tid_t	ri_end_txn;

	jbd2_logblk_t	ri_start_blocknr;
	jbd2_logblk_t	ri_end_blocknr;

	int		ri_nr_replays;
	int		ri_nr_revokes;
};

/*
 * Make sure we wrap around the log correctly
 */
#define jbd2_wrap(handle, var)							\
do {													\
	if (var > (handle)->jh_end)							\
		var -= ((handle)->jh_end - (handle)->jh_first + 1);		\
} while (0)

 /**
  * @brief Cache manager callbacks used by JBD2 driver
  */
CACHE_MANAGER_CALLBACKS jbd2_cc_manager_callbacks;

/**
 * @brief Node comparsion routine for all table
 * @param a	A pointer to the first item to be compared
 * @param b	A pointer to the second item to be compared
 * @return	-1 if index of @p a < index of @p b,
 *			1 if index of @p a > index of @p b,
 *			or 0 if index of @p a == index of @p b.
 */
static int
jbd2_generic_table_cmp(
	struct jbd2_node_hdr *hdr_a,
	struct jbd2_node_hdr *hdr_b
)
{
	if (hdr_a->th_block < hdr_b->th_block)
		return -1;
	if (hdr_a->th_block > hdr_b->th_block)
		return 1;
	return 0;
}

/**
 * @brief	Allocate memory for caller-supplied data plus some 
 *		additional memory for use by the lbcb table entry
 * @param handle Handle to journal file
 * @return Allocated buffer address
 */
static jbd2_lbcb_t *
jbd2_lbcb_alloc(jbd2_handle_t *handle)
{
	return ExAllocateFromNPagedLookasideList(
				&handle->jh_lbcb_cache);
}

/**
 * @brief Deallocate memory for a lbcb
 * @param handle	Handle to journal file
 * @param lbcb	A pointer to the lbcb that is being deallocated
 */
static void
jbd2_lbcb_free(
	jbd2_handle_t *handle,
	jbd2_lbcb_t *lbcb
)
{
	ExFreeToNPagedLookasideList(&handle->jh_lbcb_cache, lbcb);
}

/**
 * @brief	Allocate memory for caller-supplied data plus some
 *		additional memory for use by the revoke table entry
 * @param handle Handle to journal file
 * @return Allocated buffer address
 */
static jbd2_revoke_entry_t *
jbd2_revoke_entry_alloc(jbd2_handle_t *handle)
{
	return ExAllocateFromNPagedLookasideList(
				&handle->jh_revoke_cache);
}

/**
 * @brief Deallocate memory for elements to be deleted
 * @param handle	Handle to journal file
 * @param re		A pointer to the revoke entry that is being deallocated
 */
static void
jbd2_revoke_entry_free(
	jbd2_handle_t *handle,
	jbd2_revoke_entry_t *re
)
{
	ExFreeToNPagedLookasideList(&handle->jh_revoke_cache, re);
}

RB_GENERATE(jbd2_generic_table, jbd2_node_hdr, th_node, jbd2_generic_table_cmp);

/**
 * @brief	Get and reference an LBCB from LBCB table which represents
 *		given @p blocknr. 
 *		If the LBCB doesn't exist, an LBCB will be allocated an inserted into
 *		the LBCB table, and its jl_is_new flag will be set to TRUE.
 * @param handle	Handle to journal file
 * @param blocknr	Block number
 * @return an LBCB
 */
static jbd2_lbcb_t *
jbc2_lbcb_get(
	jbd2_handle_t *handle,
	jbd2_fsblk_t blocknr)
{
	jbd2_lbcb_t *lbcb_tmp, *lbcb_ret;
	lbcb_tmp = jbd2_lbcb_alloc(handle);
	if (!lbcb_tmp)
		return NULL;

	lbcb_tmp->jl_header.th_block = blocknr;
	lbcb_tmp->jl_header.th_node_type = JBD2_NODE_LBCB;
	drv_atomic_init(&lbcb_tmp->jl_header.th_refcount, 0);
	lbcb_tmp->jl_is_new = TRUE;

	lbcb_ret = (jbd2_lbcb_t *)RB_INSERT(
							jbd2_generic_table,
							&handle->jh_lbcb_table,
							&lbcb_tmp->jl_header);
	if (lbcb_ret != lbcb_tmp) {
		/* If there's already an existing node, free the temporary node */
		jbd2_lbcb_free(handle, lbcb_tmp);
		lbcb_ret->jl_is_new = FALSE;
	}
	/* Increment the reference count of the returned object */
	drv_atomic_inc(&lbcb_ret->jl_header.th_refcount);
	return lbcb_ret;
}

/**
 * @brief	Dereference an LBCB
 * @param handle	Handle to journal file
 * @param lbcb	The LBCB caller got from jbc2_lbcb_get()
 */
static void
jbd2_lbcb_put(
	jbd2_handle_t *handle,
	jbd2_lbcb_t *lbcb
)
{
	if (!drv_atomic_sub_and_test(&lbcb->jl_header.th_refcount, 1)) {
		RB_REMOVE(
			jbd2_generic_table,
			&handle->jh_lbcb_table,
			&lbcb->jl_header);
		jbd2_lbcb_free(handle, lbcb);
	}
}

/**
 * @brief	Get and reference revoke entry from revoke entry table which
 *		represents given @p blocknr.
 *		If the LBCB doesn't exist, an LBCB will be allocated an inserted into
 *		the LBCB table, and its jl_is_new flag will be set to TRUE.
 * @param handle	Handle to journal file
 * @param blocknr	Block number
 * @return an LBCB
 */
static jbd2_revoke_entry_t *
jbc2_revoke_entry_get(
	jbd2_handle_t *handle,
	jbd2_fsblk_t blocknr)
{
	jbd2_revoke_entry_t *re_tmp, *re_ret;
	re_tmp = jbd2_revoke_entry_alloc(handle);
	if (!re_tmp)
		return NULL;

	re_tmp->re_header.th_block = blocknr;
	re_tmp->re_header.th_node_type = JBD2_NODE_LBCB;
	drv_atomic_init(&re_tmp->re_header.th_refcount, 0);
	re_tmp->re_is_new = TRUE;

	re_ret = (jbd2_revoke_entry_t *)RB_INSERT(
								jbd2_generic_table,
								&handle->jh_revoke_table,
								&re_tmp->re_header);
	if (re_ret != re_tmp) {
		/* If there's already an existing node, free the temporary node */
		jbd2_revoke_entry_free(handle, re_tmp);
		re_ret->re_is_new = FALSE;
	}
	/* Increment the reference count of the returned object */
	drv_atomic_inc(&re_ret->re_header.th_refcount);
	return re_ret;
}

/**
 * @brief	Dereference a revoke entry
 * @param handle	Handle to journal file
 * @param lbcb	The revoke entry caller got from jbc2_revoke_entry_get()
 */
static void
jbd2_revoke_entry_put(
	jbd2_handle_t *handle,
	jbd2_revoke_entry_t *re
)
{
	if (!drv_atomic_sub_and_test(&re->re_header.th_refcount, 1)) {
		RB_REMOVE(
			jbd2_generic_table,
			&handle->jh_revoke_table,
			&re->re_header);
		jbd2_revoke_entry_free(handle, re);
	}
}


/**
 * @brief Helper to calculate CRC32 checksum
 * @param buf	Buffer
 * @param bufsz	Size of buffer
 * @return CRC32 checksum of the buffer
 */
static __u32 jbd2_crc32(__u32 crc, void *buf, size_t bufsz)
{
	/*
	 * ngkaho1234:	Not sure whether crc32/crc32c helpers
	 * 		are provided by NT kernel...
	 */
	return drv_crc32(crc, buf, bufsz);
}

/**
 * @brief Helper to calculate CRC32C checksum
 * @param buf	Buffer
 * @param bufsz	Size of buffer
 * @return CRC32C checksum of the buffer
 */
static __u32 jbd2_crc32c(__u32 crc, void *buf, size_t bufsz)
{
	/*
	 * ngkaho1234:	Not sure whether crc32/crc32c helpers
	 * 		are provided by NT kernel...
	 */
	return drv_crc32c(crc, buf, bufsz);
}

static inline __u32
jbd2_chksum(
	jbd2_handle_t *handle,
	__u32 crc,
	const void *buf,
	size_t bufsz)
{
	UNREFERENCED_PARAMETER(handle);
	return jbd2_crc32c(crc, buf, bufsz);
}

 /**
  * @brief Verify JBD2 superblock.
  * @param sb	JBD2 superblock
  * @return TRUE if JBD2 superblock is valid, otherwise FALSE
  */
static __bool jbd2_verify_superblock(journal_superblock_t *sb)
{
	journal_header_t *hdr = &sb->s_header;
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	if (be32_to_cpu(hdr->h_blocktype) != JBD2_SUPERBLOCK_V1 &&
	    be32_to_cpu(hdr->h_blocktype) != JBD2_SUPERBLOCK_V2)
		return FALSE;

	return TRUE;
}

/**
 * @brief Verify whether features in superblock are supported
 * @param features	Feature field
 * @param mask	Feature mask
 * @return TRUE if all features are supported, otherwise FALSE
 */
static __bool jbd2_features_supported(__be32 features, __u32 mask)
{
	return !(be32_to_cpu(features) & ~mask);
}

static __bool jbd2_descr_block_csum_verify(jbd2_handle_t *handle,
					void *buf)
{
	journal_block_tail_t *tail;
	__u32 provided;
	__u32 calculated;

	if (!jbd2_has_csum_v2or3(handle))
		return TRUE;

	tail = (journal_block_tail_t *)((char *)buf + handle->jh_blocksize -
			sizeof(journal_block_tail_t));
	provided = tail->t_checksum;
	tail->t_checksum = 0;
	calculated = jbd2_chksum(handle, handle->jh_csum_seed, buf, handle->jh_blocksize);
	tail->t_checksum = provided;

	return (provided == cpu_to_be32(calculated))
			? TRUE : FALSE;
}

static __bool jbd2_verify_descr_block(
			jbd2_handle_t *handle,
			journal_header_t *jh_buf)
{
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	return jbd2_descr_block_csum_verify(handle, jh_buf);
}

/*
 * @brief		Count the number of in-use tags in a journal descriptor block.
 * @remarks	Copied from e2fsprogs/lib/ext2fs/kernel-jbd.h
 * @param handle	Handle to journal file
 * @return size of a block tag in bytes
 */
size_t jbd2_journal_tag_bytes(jbd2_handle_t *handle)
{
	size_t sz;

	if (jbd2_has_feature_csum3(handle))
		return sizeof(journal_block_tag3_t);

	sz = sizeof(journal_block_tag_t);

	if (jbd2_has_feature_csum2(handle))
		sz += sizeof(__u16);

	if (jbd2_has_feature_64bit(handle))
		return sz;

	return sz - sizeof(__u32);
}

/*
 * @brief Count the number of in-use tags in a journal descriptor block.
 * @remarks	Copied from e2fsprogs/e2fsck/recovery.c
 * @param handle	Handle to journal file
 * @param buf		Block buffer
 * @return nr. of tags in a descriptor block
 */
static int jbd2_count_tags(jbd2_handle_t *handle, void *buf)
{
	char *				tagp;
	journal_block_tag_t *	tag;
	int			nr = 0, size = handle->jh_blocksize;
	int			tag_bytes = jbd2_journal_tag_bytes(handle);

	if (jbd2_has_csum_v2or3(handle))
		size -= sizeof(journal_block_tail_t);

	tagp = (char *)buf + sizeof(journal_header_t);

	while ((tagp - buf + tag_bytes) <= size) {
		tag = (journal_block_tag_t *)tagp;

		nr++;
		tagp += tag_bytes;
		if (!(tag->t_flags & cpu_to_be16(JBD2_FLAG_SAME_UUID)))
			tagp += 16;

		if (tag->t_flags & cpu_to_be16(JBD2_FLAG_LAST_TAG))
			break;
	}

	return nr;
}

/**
 * @brief Initialize global data of jbd2 driver
 */
void jbd2_init()
{
	jbd2_cc_manager_callbacks.AcquireForLazyWrite = jbd2_cc_acquire_for_lazywrite;
	jbd2_cc_manager_callbacks.ReleaseFromLazyWrite = jbd2_cc_release_from_lazywrite;
	jbd2_cc_manager_callbacks.AcquireForReadAhead = jbd2_cc_acquire_for_readahead;
	jbd2_cc_manager_callbacks.ReleaseFromReadAhead = jbd2_cc_release_from_readahead;
}

NTSTATUS jbd2_replay_one_pass(
			jbd2_handle_t *handle,
			struct recover_info *recover_info,
			int phase)
{
	NTSTATUS status = STATUS_SUCCESS;
	jbd2_logblk_t curr_blocknr = be32_to_cpu(handle->jh_sb->s_start);
	jbd2_tid_t curr_tid = be32_to_cpu(handle->jh_sb->s_sequence);
	RtlZeroMemory(recover_info, sizeof(struct recover_info));

	if (phase == JBD2_PHASE_SCAN) {
		recover_info->ri_start_txn = curr_tid;
		recover_info->ri_start_blocknr = curr_blocknr;
	}

	while (1) {
		void *bcb;
		__bool cc_ret, veri_ret;
		journal_header_t *jh_buf;

		__try {
			cc_ret = CcPinRead(
					log_file,
					&tmp,
					block_size,
					PIN_WAIT,
					&bcb,
					&jh_buf);
			if (!cc_ret) {
				status = STATUS_UNEXPECTED_IO_ERROR;
				__leave;
			}

			if (be32_to_cpu(jh_buf->h_sequence) != curr_tid) {
				if (phase == JBD2_PHASE_SCAN)
					status = STATUS_SUCCESS;
				else
					status = STATUS_DISK_CORRUPT_ERROR;
				__leave;
			}

			switch (be32_to_cpu(jh_buf->h_blocktype)) {
			case JBD2_DESCRIPTOR_BLOCK:
				veri_ret = jbd2_verify_descr_block(handle, jh_buf);
				if (!veri_ret) {
					if (phase == JBD2_PHASE_SCAN)
						status = STATUS_SUCCESS;
					else
						status = STATUS_DISK_CORRUPT_ERROR;
					__leave;
				}

			case JBD2_COMMIT_BLOCK:
			case JBD2_REVOKE_BLOCK:
			}
		} __finally {
			if (cc_ret)
				CcUnpinData(bcb);
		}
	}
	if (status == STATUS_SUCCESS && phase == JBD2_PHASE_SCAN) {

	}
	return status;
}

/**
 * @brief Replay a journal file
 * @param handle Handle to journal file
 * @return	STATUS_SUCCESS
 */
NTSTATUS jbd2_replay_journal(jbd2_handle_t *handle)
{
}

/**
 * @brief Open a journal file (we won't append the file of course...)
 * @param log_file		FILE_OBJECT of journal file
 * @param log_size		Size of journal file in bytes
 * @param block_size	Block size (must match the block size of client)
 * @param handle_ret	New handle returned if the journal file is successfully
 *					opened
 * @return	STATUS_SUCCESS if we successfully open a journal file, 
 *			STATUS_DISK_CORRUPT_ERROR if the journal file is corrupted, 
 *			STATUS_UNRECOGNIZED_VOLUME if there are unsupported, 
 *				features enabled, 
 *			STATUS_INSUFFICIENT_RESOURCES if there isn't enough resources, 
 *			STATUS_UNSUCCESSFUL otherwise.
 */
NTSTATUS jbd2_open_handle(
				PFILE_OBJECT log_file,
				__s64 log_size,
				unsigned int block_size,
				jbd2_handle_t **handle_ret)
{
	void *bcb = NULL;
	__bool cc_ret = FALSE;
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	CC_FILE_SIZES cc_size;
	jbd2_handle_t *handle;
	LARGE_INTEGER tmp;
	journal_superblock_t *sb_buf;
	NT_ASSERT(handle_ret);
	cc_size.AllocationSize.QuadPart = log_size;
	cc_size.FileSize.QuadPart = log_size;
	cc_size.ValidDataLength.QuadPart = log_size;

	handle = ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(jbd2_handle_t),
				JBD2_POOL_TAG);
	if (!handle)
		return STATUS_INSUFFICIENT_RESOURCES;

	RtlZeroMemory(handle, sizeof(jbd2_handle_t));

	CcInitializeCacheMap(
		log_file,
		&cc_size,
		TRUE,
		&jbd2_cc_manager_callbacks,
		NULL);

	__try {
		tmp.QuadPart = blocknr_to_offset(log_size, block_size);
		cc_ret = CcPinRead(
					log_file,
					&tmp,
					block_size,
					PIN_WAIT,
					&bcb,
					&sb_buf);
		if (!cc_ret)
			__leave;

		/* Verify whether the information in superblock is correct */
		if (!jbd2_verify_superblock(sb_buf)) {
			status = STATUS_DISK_CORRUPT_ERROR;
			__leave;
		}
		if (be32_to_cpu(sb_buf->s_blocksize) != block_size) {
			status = STATUS_DISK_CORRUPT_ERROR;
			__leave;
		}
		if (be32_to_cpu(sb_buf->s_maxlen) <
			offset_to_blocknr(log_size, block_size)) {

			status = STATUS_DISK_CORRUPT_ERROR;
			__leave;
		}

		/* Verify supported features */
		dbg_print(
			"be32_to_cpu(sb_buf->s_feature_compat) : %d\n",
			be32_to_cpu(sb_buf->s_feature_compat));
		dbg_print(
			"be32_to_cpu(sb_buf->s_feature_ro_compat) : %d\n",
			be32_to_cpu(sb_buf->s_feature_ro_compat));
		dbg_print(
			"be32_to_cpu(sb_buf->s_feature_incompat) : %d\n",
			be32_to_cpu(sb_buf->s_feature_incompat));

		if (jbd2_features_supported(
			sb_buf->s_feature_incompat,
			JBD2_KNOWN_INCOMPAT_FEATURES)) {

			dbg_print(
				"be32_to_cpu(sb_buf->s_feature_incompat) : %d unsupported!\n",
				be32_to_cpu(sb_buf->s_feature_incompat));
			status = STATUS_UNRECOGNIZED_VOLUME;
			__leave;
		}
		/* Will there ever be read-only features for journal ??? */
		if (jbd2_features_supported(
			sb_buf->s_feature_ro_compat,
			JBD2_KNOWN_ROCOMPAT_FEATURES)) {

			dbg_print(
				"be32_to_cpu(sb_buf->s_feature_ro_compat) : %d unsupported!\n",
				be32_to_cpu(sb_buf->s_feature_ro_compat));
			status = STATUS_UNRECOGNIZED_VOLUME;
			__leave;
		}

		handle->jh_sb = ExAllocatePoolWithTag(
							NonPagedPool,
							sizeof(journal_superblock_t),
							JBD2_SUPERBLOCK_TAG);
		if (!handle->jh_sb) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		/* Keep an in-memory copy of superblock fields */
		RtlCopyMemory(handle->jh_sb, sb_buf, sizeof(journal_superblock_t));

		/* Initialize the fields in journal handle */
		handle->jh_log_file = log_file;
		drv_mutex_init(&handle->jh_lock);
		handle->jh_blocksize = block_size;
		handle->jh_blockcnt = be32_to_cpu(handle->jh_sb->s_maxlen);
		RtlCopyMemory(handle->jh_uuid, handle->jh_sb, UUID_SIZE);
		handle->jh_max_txn = be32_to_cpu(handle->jh_sb->s_max_transaction);
		handle->jh_running_txn = NULL;
		InitializeListHead(&handle->jh_txn_queue);

		/* Calculate the head, tail of logging area in journal */
		handle->jh_start = be32_to_cpu(handle->jh_sb->s_first);
		handle->jh_end = handle->jh_blockcnt - 1;

		/* Calculate the head, tail and nr. of blocks of free area in journal */
		handle->jh_free_start = be32_to_cpu(handle->jh_sb->s_first);
		handle->jh_free_end = handle->jh_blockcnt - 1;
		handle->jh_free_blockcnt = handle->jh_blockcnt - 1;

		/* Initialize block table allocator and revoke table allocator */
		ExInitializeNPagedLookasideList(
				&handle->jh_lbcb_cache,
				NULL,
				NULL,
				0,
				sizeof(jbd2_lbcb_t),
				JBD2_LBCB_TABLE_TAG,
				0);
		ExInitializeNPagedLookasideList(
				&handle->jh_revoke_cache,
				NULL,
				NULL,
				0,
				sizeof(jbd2_revoke_entry_t),
				JBD2_REVOKE_TABLE_TAG,
				0);

		/* Initialize block table and revoke table */
		RB_INIT(&handle->jh_lbcb_table);
		RB_INIT(&handle->jh_revoke_table);
	} __finally {
		if (bcb)
			CcUnpinData(bcb);

		if (!NT_SUCCESS(status)) {
			if (handle->jh_sb)
				ExFreePoolWithTag(handle->jh_sb, JBD2_SUPERBLOCK_TAG);

			ExFreePoolWithTag(handle, JBD2_POOL_TAG);
		} else {
			*handle_ret = handle;
		}

	}
	return status;
}

void jbd2_flush(
		jbd2_handle_t *handle,
		KEVENT *event,
		NTSTATUS *status)
{

}

NTSTATUS jbd2_close_handle(jbd2_handle_t *handle)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	KEVENT event;

	jbd2_flush(handle, &event, &status);
	return status;
}
