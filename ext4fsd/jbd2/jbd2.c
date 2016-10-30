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
  *				the different passes can carry information between them.
  * @remarks	Copied from e2fsprogs/e2fsck/recovery.c
  */
struct recovery_info {
	jbd2_tid_t	ri_start_txn;
	jbd2_tid_t	ri_end_txn;

	__bool		ri_has_txn;
};

/*
 * Make sure we wrap around the log correctly
 */
#define jbd2_wrap(handle, var)							\
do {													\
	if (var > (handle)->jh_end)							\
		var -= ((handle)->jh_end - (handle)->jh_start + 1);		\
} while (0)

/*
 * To prevent overflowing we need these inline routines for
 * tid comparsion.
 */
static int jbd2_tid_cmp(jbd2_tid_t a, jbd2_tid_t b)
{
	if ((__s32)(a - b) < 0)
		return -1;
	if ((__s32)(a - b) > 0)
		return 1;
	return 0;
}

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
 * @param lbcb		A pointer to the lbcb that is being deallocated
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
 *			additional memory for use by the revoke table entry
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
 *			given @p blocknr. 
 *			If the LBCB doesn't exist, an LBCB will be allocated an inserted into
 *			the LBCB table, and its jl_is_new flag will be set to TRUE.
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
 * @param lbcb		The LBCB caller got from jbc2_lbcb_get()
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
 *			represents given @p blocknr.
 *			If the LBCB doesn't exist, an LBCB will be allocated an inserted into
 *			the LBCB table, and its jl_is_new flag will be set to TRUE.
 * @param handle	Handle to journal file
 * @param blocknr	Block number
 * @return an LBCB
 */
static jbd2_revoke_entry_t *
jbd2_revoke_entry_get(
	jbd2_handle_t *handle,
	jbd2_fsblk_t blocknr)
{
	jbd2_revoke_entry_t *re_tmp, *re_ret;
	re_tmp = jbd2_revoke_entry_alloc(handle);
	if (!re_tmp)
		return NULL;

	re_tmp->re_header.th_block = blocknr;
	re_tmp->re_header.th_node_type = JBD2_NODE_REVOKE;
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

/*
 * @brief	Get and reference revoke entry from revoke entry table which
 *			represents given @p blocknr.
 * @param handle	Handle to journal file
 * @param blocknr	Block number
 * @return	an LBCB if it exists in the revoke entry table,
 * 			otherwise NULL is returned
 */
static jbd2_revoke_entry_t *
jbd2_revoke_entry_find(
	jbd2_handle_t *handle,
	jbd2_fsblk_t blocknr)
{
	jbd2_revoke_entry_t re_tmp, *re_ret;
	re_tmp.re_header.th_block = blocknr;
	re_tmp.re_header.th_node_type = JBD2_NODE_REVOKE;

	re_ret = (jbd2_revoke_entry_t *)RB_FIND(
								jbd2_generic_table,
								&handle->jh_revoke_table,
								&re_tmp.re_header);
	if (re_ret) {
		/* Increment the reference count of the returned object */
		drv_atomic_inc(&re_ret->re_header.th_refcount);
	}

	return re_ret;
}

/**
 * @brief	Dereference a revoke entry
 * @param handle	Handle to journal file
 * @param lbcb		The revoke entry caller got from jbc2_revoke_entry_get()
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

static void
jbd2_revoke_table_clear(jbd2_handle_t *handle)
{
	jbd2_revoke_entry_t *re, *tmp;
	RB_FOREACH_SAFE(re,
			jbd2_generic_table,
			&handle->jh_revoke_table,
			tmp) {
		jbd2_revoke_entry_put(handle, re);
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
 * @brief Verify JBD2 metadata block.
 * @param hdr	JBD2 metadata block header
 * @return TRUE if JBD2 metadata block is valid, otherwise FALSE
 */
static __bool jbd2_verify_metadata_block(journal_header_t *hdr)
{
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	/* We support h_blocktype from 1 to 5 */
	if (!be32_to_cpu(hdr->h_blocktype)
			|| be32_to_cpu(hdr->h_blocktype) > JBD2_REVOKE_BLOCK)
		return FALSE;

	return TRUE;
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
 * @param mask		Feature mask
 * @return TRUE if all features are supported, otherwise FALSE
 */
static __bool jbd2_features_supported(__be32 features, __u32 mask)
{
	return !(be32_to_cpu(features) & ~mask);
}

static __bool jbd2_descr_block_csum_verify(
				jbd2_handle_t *handle,
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
			void *buf)
{
	journal_header_t *hdr = (journal_header_t *)buf;
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	return jbd2_descr_block_csum_verify(handle, buf);
}

static __bool jbd2_commit_block_csum_verify(
				jbd2_handle_t *handle,
				void *buf)
{
	journal_commit_header_t *commit_hdr;
	__u32 provided;
	__u32 calculated;
	commit_hdr = (journal_commit_header_t *)buf;

	if (!jbd2_has_csum_v2or3(handle))
		return TRUE;

	provided = commit_hdr->h_chksum[0];
	commit_hdr->h_chksum[0] = 0;
	calculated = jbd2_chksum(handle, handle->jh_csum_seed, buf, handle->jh_blocksize);
	commit_hdr->h_chksum[0] = provided;

	return (provided == cpu_to_be32(calculated))
			? TRUE : FALSE;
}

static __bool jbd2_verify_commit_block(
			jbd2_handle_t *handle,
			void *buf)
{
	journal_header_t *hdr = (journal_header_t *)buf;
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	return jbd2_commit_block_csum_verify(handle, buf);
}

static __bool jbd2_revoke_block_csum_verify(
				jbd2_handle_t *handle,
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

static __bool jbd2_verify_revoke_block(
			jbd2_handle_t *handle,
			void *buf)
{
	journal_header_t *hdr = (journal_header_t *)buf;
	if (be32_to_cpu(hdr->h_magic) != JBD2_MAGIC_NUMBER)
		return FALSE;

	return jbd2_revoke_block_csum_verify(handle, buf);
}

/*
 * @brief	Calculate the minimal size of a block tag
 * @remarks	Copied from e2fsprogs/lib/ext2fs/kernel-jbd.h
 * @param handle	Handle to journal file
 * @return minimal size of a block tag in bytes
 */
size_t jbd2_min_tag_size(jbd2_handle_t *handle)
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

/**
 * @brief Extract blocknr from block tag
 * @param handle	Handle to journal file
 * @param tag		Block tag
 * @return blocknr
 */
static jbd2_fsblk_t
jbd2_tag_blocknr(jbd2_handle_t *handle, journal_block_tag_t *tag)
{
	jbd2_fsblk_t blocknr = 0;
	blocknr = be32_to_cpu(tag->t_blocknr);
	if (jbd2_has_feature_64bit(handle))
		blocknr |= be32_to_cpu(tag->t_blocknr_high);

	return blocknr;
}

/**
 * @brief Calculate the size of a revoke entry
 * @param handle	Handle to journal file
 * @return size of a revoke entry in bytes
 */
static size_t
jbd2_revoke_entry_size(jbd2_handle_t *handle)
{
	if (jbd2_has_feature_64bit(handle))
		return sizeof(__be64);

	return sizeof(__be32);
}

/**
 * @brief Extract blocknr from revoke entry
 * @param handle	Handle to journal file
 * @param entry		Revoke entry
 * @return blocknr
 */
static jbd2_fsblk_t
jbd2_revoke_entry_blocknr(jbd2_handle_t *handle, void *entry)
{
	if (jbd2_has_feature_64bit(handle))
		return be64_to_cpu(*(__be64 *)entry);

	return be32_to_cpu(*(__be32 *)entry);
}

/*
 * @brief	Calculate the size of a block tag
 * @param handle	Handle to journal file
 * @param tag		Block tag
 * @return size of a block tag in bytes
 */
static size_t
jbd2_tag_size(
	jbd2_handle_t *handle,
	void *tag)
{
	size_t sz = jbd2_min_tag_size(handle);
	if (jbd2_has_feature_csum3(handle)) {
		journal_block_tag3_t *tag3;
		tag3 = (journal_block_tag3_t *)tag;

		if (!(be32_to_cpu(tag3->t_flags) & JBD2_FLAG_SAME_UUID))
			sz += UUID_SIZE;
	} else {
		journal_block_tag_t *tag1;
		tag1 = (journal_block_tag_t *)tag;

		if (!(be16_to_cpu(tag1->t_flags) & JBD2_FLAG_SAME_UUID))
			sz += UUID_SIZE;
	}
	return sz;
}

/**
 * @brief Test the whether @p flag is set
 * @param handle	Handle to journal file
 * @param tag		Tag
 * @param flag		Flag to be tested on
 * @return	TRUE or FALSE
 */
static __bool
jbd2_test_flag(
	jbd2_handle_t *handle,
	void *tag,
	int flag)
{
	if (jbd2_has_feature_csum3(handle)) {
		journal_block_tag3_t *tag3;
		tag3 = (journal_block_tag3_t *)tag;

		if (be32_to_cpu(tag3->t_flags) & flag)
			return TRUE;
	} else {
		journal_block_tag_t *tag1;
		tag1 = (journal_block_tag3_t *)tag;

		if (be16_to_cpu(tag1->t_flags) & flag)
			return TRUE;
	}
	return FALSE;
}

/**
 * @brief Obtain the checksum stored in block tag
 * @param handle	Handle to journal file
 * @param tag		Tag
 * @return	Checksum value stored in block tag body
 */
static __u32
jbd2_tag_csum(
	jbd2_handle_t *handle,
	void *tag)
{
	__u32 csum = ~0;
	if (jbd2_has_feature_csum3(handle)) {
		journal_block_tag3_t *tag3;
		tag3 = (journal_block_tag3_t *)tag;

		csum = be32_to_cpu(tag3->t_checksum);
	} else {
		journal_block_tag_t *tag1;
		tag1 = (journal_block_tag3_t *)tag;

		csum = be16_to_cpu(tag1->t_checksum);
	}
	return csum;
}

/*
 * @brief Count the number of in-use tags in a journal descriptor block.
 * @param handle	Handle to journal file
 * @param buf		Block buffer
 * @return nr. of tags in a descriptor block
 */
static int jbd2_count_tags(jbd2_handle_t *handle, void *buf)
{
	char *tagp;
	int nr = 0;
	int blocksize = handle->jh_blocksize;
	int tag_bytes = jbd2_min_tag_size(handle);

	if (jbd2_has_csum_v2or3(handle))
		blocksize -= sizeof(journal_block_tail_t);

	tagp = (char *)buf + sizeof(journal_header_t);

	for (; tagp - buf + tag_bytes <= blocksize;
			tagp += jbd2_tag_size(handle, tagp), nr++) {
		journal_block_tag_t *tag;
		tag = (journal_block_tag_t *)tagp;

		if (jbd2_test_flag(handle, tag, JBD2_FLAG_LAST_TAG))
			break;
	}

	return nr;
}

/**
 * @brief Verify checksum of the blocks mentioned in a descriptor block
 * @param handle	Handle to journal file
 * @param tid		Transaction ID the descriptor block belongs to
 * @param blocknr	Block number of the descriptor block in log file
 * @param buf		Descriptor block buffer
 * @return	STATUS_SUCCESS indicating that the operation succeeds,
 * 		otherwise the operation fails.
 */
static NTSTATUS
jbd2_blocks_csum_verify(jbd2_handle_t *handle,
			jbd2_logblk_t blocknr,
			void *buf)
{
	char *tagp;
	jbd2_logblk_t nr = 1;
	NTSTATUS status = STATUS_SUCCESS;
	int blocksize = handle->jh_blocksize;
	int tag_bytes = jbd2_min_tag_size(handle);

	if (!jbd2_has_csum_v2or3(handle))
		return STATUS_SUCCESS;

	tagp = (char *)buf + sizeof(journal_header_t);

	for (; tagp - buf + tag_bytes <= blocksize;
			tagp += jbd2_tag_size(handle, tagp), nr++) {
		__bool cc_ret;
		__u32 calculated;
		LARGE_INTEGER tmp;
		journal_block_tag_t *tag;
		jbd2_fsblk_t fs_blocknr;
		void *from_bcb, *from_buf;
		tag = (journal_block_tag_t *)tagp;

		fs_blocknr = jbd2_tag_blocknr(handle, tag);

		/*
		 * XXX:	We do not support multiple clients, since
		 *	as of linux 4.x multiple clients support isn't
		 *	available
		 */
		tmp.QuadPart = blocknr_to_offset(
					blocknr + nr,
					handle->jh_blocksize);
		cc_ret = CcPinRead(
				handle->jh_log_file,
				&tmp,
				handle->jh_blocksize,
				PIN_WAIT,
				&from_bcb,
				&from_buf);
		if (!cc_ret) {
			status = STATUS_UNEXPECTED_IO_ERROR;
			break;
		}

		if (jbd2_has_feature_csum2(handle)) {
			calculated = jbd2_chksum(
							handle,
							handle->jh_csum_seed,
							from_buf,
							handle->jh_blocksize) & 0xffff;
		} else {
			calculated = jbd2_chksum(
							handle,
							handle->jh_csum_seed,
							from_buf,
							handle->jh_blocksize);
		}
		if (calculated != jbd2_tag_csum(handle, tag)) {
			dbg_print("Checksum calculation fails on handle %p\n", handle);
			status = STATUS_UNSUCCESSFUL;
		}

		CcUnpinData(from_bcb);

		if (status == STATUS_UNSUCCESSFUL)
			break;

		if (jbd2_test_flag(handle, tag, JBD2_FLAG_LAST_TAG))
			break;
	}

	return status;
}

/**
 * @brief Replay the blocks mentioned in a descriptor block
 * @param handle	Handle to journal file
 * @param tid		Transaction ID the descriptor block belongs to
 * @param blocknr	Block number of the descriptor block in log file
 * @param buf		Descriptor block buffer
 * @return	STATUS_SUCCESS indicating that the operation succeeds,
 * 		otherwise the operation fails.
 */
static NTSTATUS
jbd2_replay_descr_block(jbd2_handle_t *handle,
			jbd2_tid_t tid,
			jbd2_logblk_t blocknr,
			void *buf)
{
	char *tagp;
	jbd2_logblk_t nr = 1;
	NTSTATUS status = STATUS_SUCCESS;
	int blocksize = handle->jh_blocksize;
	int tag_bytes = jbd2_min_tag_size(handle);

	if (jbd2_has_csum_v2or3(handle))
		blocksize -= sizeof(journal_block_tail_t);

	tagp = (char *)buf + sizeof(journal_header_t);

	for (; tagp - buf + tag_bytes <= blocksize;
			tagp += jbd2_tag_size(handle, tagp), nr++) {
		__bool cc_ret;
		jbd2_tid_t re_tid;
		LARGE_INTEGER tmp;
		journal_block_tag_t *tag;
		jbd2_fsblk_t fs_blocknr;
		jbd2_revoke_entry_t *entry;
		void *from_bcb, *from_buf, *to_bcb, *to_buf;
		tag = (journal_block_tag_t *)tagp;

		fs_blocknr = jbd2_tag_blocknr(handle, tag);
		entry = jbd2_revoke_entry_find(handle, fs_blocknr);
		if (entry) {
			re_tid = entry->re_tid;
			jbd2_revoke_entry_put(handle, entry);
			if (jbd2_tid_cmp(tid, re_tid) <= 0)
				continue;
		}

		/*
		 * XXX:	We do not support multiple clients, since
		 *	as of linux 4.x multiple clients support isn't
		 *	available
		 */
		tmp.QuadPart = blocknr_to_offset(
					blocknr + nr,
					handle->jh_blocksize);
		cc_ret = CcPinRead(
				handle->jh_log_file,
				&tmp,
				handle->jh_blocksize,
				PIN_WAIT,
				&from_bcb,
				&from_buf);
		if (!cc_ret) {
			status = STATUS_UNEXPECTED_IO_ERROR;
			break;
		}

		tmp.QuadPart = blocknr_to_offset(
					fs_blocknr,
					handle->jh_blocksize);
		cc_ret = CcPreparePinWrite(
					handle->jh_client_file,
					&tmp,
					handle->jh_blocksize,
					TRUE,
					PIN_WAIT,
					&to_bcb,
					&to_buf);
		if (!cc_ret) {
			status = STATUS_UNEXPECTED_IO_ERROR;
			CcUnpinData(from_bcb);
			break;
		}

		RtlCopyMemory(to_buf, from_buf, handle->jh_blocksize);
		if (jbd2_test_flag(handle, tag, JBD2_FLAG_ESCAPE)) {
			journal_header_t *jh_buf = to_buf;
			jh_buf->h_magic = cpu_to_be32(JBD2_MAGIC_NUMBER);
		}
		CcSetDirtyPinnedData(to_bcb, NULL);

		CcUnpinData(from_bcb);
		CcUnpinData(to_bcb);

		if (jbd2_test_flag(handle, tag, JBD2_FLAG_LAST_TAG))
			break;
	}

	return status;
}

/**
 * @brief	Scan revoke entries in a revocation block and
 *			insert them into revoke table
 * @param handle	Handle to journal file
 * @param tid		Transaction ID
 * @param buf		Block buffer
 * @return STATUS_SUCCESS if all the revoke entries are inserted
 */
static NTSTATUS
jbd2_scan_revoke_entries(
		jbd2_handle_t *handle,
		jbd2_tid_t tid,
		void *buf)
{
	size_t entry_sz = jbd2_revoke_entry_size(handle);
	size_t csum_size = 0;
	char *bufp = (char *)buf;
	journal_revoke_header_t *revoke_hdr =
			(journal_revoke_header_t *)buf;
	size_t rcount;

	if (jbd2_has_csum_v2or3(handle))
		csum_size = sizeof(journal_block_tail_t);

	rcount = be32_to_cpu(revoke_hdr->r_count);

	/* Check for corrupted revocation block */
	if (rcount > handle->jh_blocksize - csum_size)
		return STATUS_DISK_CORRUPT_ERROR;

	for (bufp += sizeof(journal_header_t);
			bufp + entry_sz <= (char *)buf + rcount;
			bufp += entry_sz) {
		jbd2_fsblk_t fs_blocknr = jbd2_revoke_entry_blocknr(bufp);
		jbd2_revoke_entry_t *revoke_entry;

		revoke_entry = jbd2_revoke_entry_get(handle, fs_blocknr);
		if (!revoke_entry)
			return STATUS_INSUFFICIENT_RESOURCES;

		revoke_entry->re_tid = tid;
		jbd2_revoke_entry_put(handle, revoke_entry);
	}
	return STATUS_SUCCESS;
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
			struct recovery_info *recovery_info,
			int phase)
{
	LARGE_INTEGER tmp;
	NTSTATUS status = STATUS_SUCCESS;
	jbd2_logblk_t curr_blocknr = be32_to_cpu(handle->jh_sb->s_start);
	jbd2_tid_t curr_tid = be32_to_cpu(handle->jh_sb->s_sequence);
	RtlZeroMemory(recovery_info, sizeof(struct recovery_info));

	if (phase == JBD2_PHASE_SCAN) {
		recovery_info->ri_start_txn = curr_tid;
		recovery_info->ri_has_txn = FALSE;
	}

	while (1) {
		void *bcb;
		__bool cc_ret, veri_ret;
		journal_header_t *jh_buf;

		if (phase != JBD2_PHASE_SCAN &&
				jbd2_tid_cmp(curr_tid, recovery_info->ri_end_txn) > 0)
			break;

		__try {
			int nr_tags;
			tmp.QuadPart = blocknr_to_offset(
						curr_blocknr,
						handle->jh_blocksize);
			cc_ret = CcPinRead(
					handle->jh_log_file,
					&tmp,
					handle->jh_blocksize,
					PIN_WAIT,
					&bcb,
					&jh_buf);
			if (!cc_ret) {
				status = STATUS_UNEXPECTED_IO_ERROR;
				goto end;
			}

			if (!jbd2_verify_metadata_block(jh_buf)
					|| be32_to_cpu(jh_buf->h_sequence) != curr_tid) {
				if (phase == JBD2_PHASE_SCAN)
					status = STATUS_SUCCESS;
				else
					status = STATUS_DISK_CORRUPT_ERROR;

				goto end;
			}

			switch (be32_to_cpu(jh_buf->h_blocktype)) {
			case JBD2_DESCRIPTOR_BLOCK:
				veri_ret = jbd2_verify_descr_block(handle, jh_buf);
				if (!veri_ret) {
					if (phase == JBD2_PHASE_SCAN)
						status = STATUS_SUCCESS;
					else
						status = STATUS_DISK_CORRUPT_ERROR;

					goto end;
				}

				nr_tags = jbd2_count_tags(handle, jh_buf);
				if (phase != JBD2_PHASE_REPLAY) {
					if (phase == JBD2_PHASE_SCAN) {
						status = jbd2_blocks_csum_verify(
									handle,
									curr_blocknr,
									jh_buf);
						if (!NT_SUCCESS(status))
							goto end;
					}
					curr_blocknr += nr_tags + 1;
					jbd2_wrap(handle, curr_blocknr);
					break;
				} else {
					status = jbd2_replay_descr_block(
								handle,
								curr_tid,
								curr_blocknr,
								jh_buf);
					if (!NT_SUCCESS(status))
						goto end;

					curr_blocknr += nr_tags + 1;
					jbd2_wrap(handle, curr_blocknr);
				}

				break;
			case JBD2_COMMIT_BLOCK:
				veri_ret = jbd2_verify_commit_block(handle, jh_buf);
				if (!veri_ret) {
					if (phase == JBD2_PHASE_SCAN)
						status = STATUS_SUCCESS;
					else
						status = STATUS_DISK_CORRUPT_ERROR;

					goto end;
				}

				if (phase == JBD2_PHASE_SCAN) {
					recovery_info->ri_end_txn = curr_tid;
					recovery_info->ri_has_txn = TRUE;
				}
				curr_tid++;
				curr_blocknr++;
				jbd2_wrap(handle, curr_blocknr);
				break;
			case JBD2_REVOKE_BLOCK:
				veri_ret = jbd2_verify_revoke_block(handle, jh_buf);
				if (!veri_ret) {
					if (phase == JBD2_PHASE_SCAN)
						status = STATUS_SUCCESS;
					else
						status = STATUS_DISK_CORRUPT_ERROR;

					goto end;
				}

				if (phase == JBD2_PHASE_SCAN_REVOKE)
					jbd2_scan_revoke_entries(handle, curr_tid, jh_buf);

				curr_blocknr++;
				jbd2_wrap(handle, curr_blocknr);
			}
		} __finally {
			if (cc_ret)
				CcUnpinData(bcb);
		}
	}
end:
	return status;
}

/**
 * @brief Replay a journal file
 * @param handle Handle to journal file
 * @return	STATUS_SUCCESS
 */
NTSTATUS jbd2_replay_journal(jbd2_handle_t *handle)
{
	struct recovery_info recovery_info;
	NTSTATUS status = jbd2_replay_one_pass(
					handle,
					&recovery_info,
					JBD2_PHASE_SCAN);
	if (!status)
		goto cleanup;

	status = jbd2_replay_one_pass(
				handle,
				&recovery_info,
				JBD2_PHASE_SCAN_REVOKE);
	if (!status)
		goto cleanup;

	status = jbd2_replay_one_pass(
				handle,
				&recovery_info,
				JBD2_PHASE_REPLAY);
	if (!status)
		goto cleanup;
cleanup:
	jbd2_revoke_table_clear(handle);
	return status;
}

/**
 * @brief	The routine performs a CcUnitializeCacheMap to LargeZero synchronously.
 * 			That is it waits on the Cc event.  This call is useful when we want to be certain
 * 			when a close will actually some in.
 */
void jbd2_cache_sync_uninit_map(PFILE_OBJECT file_object)
{
	CACHE_UNINITIALIZE_EVENT uninit_complete_event;
	NTSTATUS wait_status;
	LARGE_INTEGER zero_offset;

	zero_offset.QuadPart = 0;

	KeInitializeEvent(
		&uninit_complete_event.Event,
		SynchronizationEvent,
		FALSE);

	CcUninitializeCacheMap(
		file_object,
		&zero_offset,
		&uninit_complete_event);

	wait_status = KeWaitForSingleObject(
		&uninit_complete_event.Event,
		Executive,
		KernelMode,
		FALSE,
		NULL);

	NT_ASSERT(wait_status == STATUS_SUCCESS);
}

/**
 * @brief Open a journal file (we won't append the file of course...)
 * @param client_file	FILE_OBJECT of client file
 * @param log_file		FILE_OBJECT of journal file
 * @param log_size		Size of journal file in bytes
 * @param blocksize		Block size (must match the block size of client)
 * @param handle_ret	New handle returned if the journal file is successfully
 *						opened
 * @return	STATUS_SUCCESS if we successfully open a journal file, 
 * 			STATUS_DISK_CORRUPT_ERROR if the journal file is corrupted, 
 * 			STATUS_UNRECOGNIZED_VOLUME if there are unsupported, 
 * 				features enabled, 
 * 			STATUS_INSUFFICIENT_RESOURCES if there isn't enough resources, 
 * 			STATUS_UNSUCCESSFUL otherwise.
 */
NTSTATUS jbd2_open_handle(
				PFILE_OBJECT client_file,
				PFILE_OBJECT log_file,
				__s64 log_size,
				unsigned int blocksize,
				jbd2_handle_t **handle_ret)
{
	void *bcb = NULL;
	__bool lbcb_cache_inited = FALSE;
	__bool revoke_cache_inited = FALSE;
	__bool lock_inited = FALSE;
	journal_superblock_t *jh_sb = NULL;

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
		tmp.QuadPart = blocknr_to_offset(log_size, blocksize);
		cc_ret = CcPinRead(
					log_file,
					&tmp,
					blocksize,
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
		if (be32_to_cpu(sb_buf->s_blocksize) != blocksize) {
			status = STATUS_DISK_CORRUPT_ERROR;
			__leave;
		}
		if (be32_to_cpu(sb_buf->s_maxlen) <
			offset_to_blocknr(log_size, blocksize)) {

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

		jh_sb = ExAllocatePoolWithTag(
						NonPagedPool,
						sizeof(journal_superblock_t),
						JBD2_SUPERBLOCK_TAG);
		if (!jh_sb) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			__leave;
		}

		drv_mutex_init(&handle->jh_lock);
		lock_inited = TRUE;
		handle->jh_sb = jh_sb;

		/* Keep an in-memory copy of superblock fields */
		RtlCopyMemory(jh_sb, sb_buf, sizeof(journal_superblock_t));

		/* Initialize the fields in journal handle */
		handle->jh_log_file = log_file;
		handle->jh_client_file = client_file;

		handle->jh_blocksize = blocksize;
		handle->jh_blockcnt = be32_to_cpu(jh_sb->s_maxlen);
		RtlCopyMemory(handle->jh_uuid, jh_sb, UUID_SIZE);
		handle->jh_max_txn = be32_to_cpu(jh_sb->s_max_transaction);
		handle->jh_running_txn = NULL;
		InitializeListHead(&handle->jh_txn_queue);

		/* Calculate the head, tail of logging area in journal */
		handle->jh_start = be32_to_cpu(jh_sb->s_first);
		handle->jh_end = handle->jh_blockcnt - 1;

		/* Calculate the head, tail and nr. of blocks of free area in journal */
		handle->jh_free_start = be32_to_cpu(jh_sb->s_first);
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
		lbcb_cache_inited = TRUE;
		ExInitializeNPagedLookasideList(
				&handle->jh_revoke_cache,
				NULL,
				NULL,
				0,
				sizeof(jbd2_revoke_entry_t),
				JBD2_REVOKE_TABLE_TAG,
				0);
		revoke_cache_inited = TRUE;

		/* Initialize block table and revoke table */
		RB_INIT(&handle->jh_lbcb_table);
		RB_INIT(&handle->jh_revoke_table);
	} __finally {
		if (bcb)
			CcUnpinData(bcb);

		if (!NT_SUCCESS(status)) {
			if (jh_sb)
				ExFreePoolWithTag(jh_sb, JBD2_SUPERBLOCK_TAG);
			if (lock_inited)
				drv_mutex_destroy(&handle->jh_lock);
			if (lbcb_cache_inited)
				ExDeleteNPagedLookasideList(&handle->jh_lbcb_cache);
			if (revoke_cache_inited)
				ExDeleteNPagedLookasideList(&handle->jh_revoke_cache);

			jbd2_cache_sync_uninit_map(log_file);
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

	ExFreePoolWithTag(handle->jh_sb, JBD2_SUPERBLOCK_TAG);
	drv_mutex_destroy(&handle->jh_lock);
	ExDeleteNPagedLookasideList(&handle->jh_lbcb_cache);
	ExDeleteNPagedLookasideList(&handle->jh_revoke_cache);
	jbd2_cache_sync_uninit_map(handle->jh_log_file);
	ExFreePoolWithTag(handle, JBD2_POOL_TAG);
	return status;
}

/*
 * NOTES:
 *
 * It sounds better if we remove a transaction from checkpoint queue
 * when all its buffer is flushed to persistent storage...
 */
