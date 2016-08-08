/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "jbd2\jbd2.h"

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