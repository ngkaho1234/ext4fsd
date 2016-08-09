/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "jbd2\jbd2.h"

 /**
  * @brief Cache manager callbacks used by JBD2 driver
  */
CACHE_MANAGER_CALLBACKS jbd2_cc_manager_callbacks;

/**
 * @brief Node comparsion routine for all table
 * @param table	A pointer to the generic table
 * @param a		A pointer to the first item to be compared
 * @param b		A pointer to the second item to be compared
 * @return	GenericLessThan if index of @p a < index of @p b,
 *			GenericGreaterThan if index of @p a > index of @p b,
 *			or GenericEqual if index of @p a == index of @p b.
 */
static RTL_GENERIC_COMPARE_RESULTS
jbd2_generic_compare(
	struct _RTL_GENERIC_TABLE  *table,
	PVOID  a,
	PVOID  b
)
{
	struct jbd2_node_hdr *hdr_a, *hdr_b;
	hdr_a = (struct jbd2_node_hdr *)a;
	hdr_b = (struct jbd2_node_hdr *)b;

	if (hdr_a->th_block < hdr_b->th_block)
		return GenericLessThan;
	if (hdr_a->th_block > hdr_b->th_block)
		return GenericGreaterThan;
	return GenericEqual;
}

/**
 * @brief	Allocate memory for caller-supplied data plus some 
 *		additional memory for use by the lbcb table entry
 * @param table		A pointer to the generic table
 * @param byte_size	The number of bytes to allocate
 * @return Allocated buffer address
 */
static PVOID
jbd2_lbcb_table_alloc(
	struct _RTL_GENERIC_TABLE  *table,
	CLONG  byte_size
)
{
	return ExAllocatePoolWithTag(
				NonPagedPool,
				byte_size,
				JBD2_LBCB_TABLE_TAG);
}

/**
 * @brief Deallocate memory for elements to be deleted from the lbcb table
 * @param table	A pointer to the generic table
 * @param buf		A pointer to the element that is being deleted
 */
static void
jbd2_lbcb_table_free(
	struct _RTL_GENERIC_TABLE  *table,
	PVOID buf
)
{
	ExFreePoolWithTag(buf, JBD2_LBCB_TABLE_TAG);
}

/**
 * @brief	Allocate memory for caller-supplied data plus some
 *		additional memory for use by the revoke table entry
 * @param table		A pointer to the generic table
 * @param byte_size	The number of bytes to allocate
 * @return Allocated buffer address
 */
static PVOID
jbd2_revoke_table_alloc(
	struct _RTL_GENERIC_TABLE  *table,
	CLONG  byte_size
)
{
	return ExAllocatePoolWithTag(
				NonPagedPool,
				byte_size,
				JBD2_REVOKE_TABLE_TAG);
}

/**
 * @brief Deallocate memory for elements to be deleted from the revoke table
 * @param table	A pointer to the generic table
 * @param buf		A pointer to the element that is being deleted
 */
static void
jbd2_revoke_table_free(
	struct _RTL_GENERIC_TABLE  *table,
	PVOID buf
)
{
	ExFreePoolWithTag(buf, JBD2_REVOKE_TABLE_TAG);
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

		/* Calculate the head, tail and nr. of blocks of free area in journal */
		handle->jh_free_start = be32_to_cpu(handle->jh_sb->s_first);
		handle->jh_free_end = handle->jh_blockcnt - 1;
		handle->jh_free_blockcnt = handle->jh_blockcnt - 1;

		/* Initialize block table and revoke table */
		RtlInitializeGenericTable(
			&handle->jh_lbcb_table,
			jbd2_generic_compare,
			jbd2_lbcb_table_alloc,
			jbd2_lbcb_table_free,
			NULL);
		RtlInitializeGenericTable(
			&handle->jh_revoke_table,
			jbd2_generic_compare,
			jbd2_revoke_table_alloc,
			jbd2_revoke_table_free,
			NULL);
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
