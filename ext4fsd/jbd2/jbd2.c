/*
 * Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
 */

#include "jbd2\jbd2.h"

struct jbd2_handle *jbd2_open_handle(PFILE_OBJECT logfile)
{
	return NULL;
}

void jbd2_flush_to_lsn(
		jbd2_handle_t *handle,
		jbd_tid_t lsn,
		KEVENT *event,
		NTSTATUS *status)
{

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