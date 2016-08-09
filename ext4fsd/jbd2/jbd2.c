/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "jbd2\jbd2.h"

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

NTSTATUS jbd2_open_handle(
				PFILE_OBJECT logfile,
				jbd2_handle_t **handle_ret)
{
	jbd2_handle_t *handle;
	ASSERT(handle_ret);

	handle = ExAllocatePoolWithTag(
				NonPagedPool,
				sizeof(jbd2_handle_t),
				JBD2_POOL_TAG);
	if (!handle)
		return STATUS_INSUFFICIENT_RESOURCES;
	
	*handle_ret = handle;
	return STATUS_SUCCESS;
}

void jbd2_flush(
		jbd2_handle_t *handle,
		KEVENT *event,
		NTSTATUS *status)
{

}

NTSTATUS jbd2_close_handle(jbd2_handle_t *handle)
{

}