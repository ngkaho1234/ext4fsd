/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#include "ext4.h"
#include "ext4_data.h"

/**
 * @brief	Wrapper over CcInitializeCacheMap and CcSetAdditionalCacheAttributesEx to initialize
 *		caching and enable IO accounting on a file.
 */
void ext4_cache_init_map(
		PFILE_OBJECT file_object,
		PCC_FILE_SIZES file_size,
		__bool pin_access,
		PCACHE_MANAGER_CALLBACKS callbacks,
		void *lazy_write_ctx)
{
	CcInitializeCacheMap(
		file_object,
		file_size,
		pin_access,
		callbacks,
		lazy_write_ctx);
}

/**
 * @brief	The routine performs a CcUnitializeCacheMap to LargeZero synchronously. 
 *		That is it waits on the Cc event.  This call is useful when we want to be certain
 *		when a close will actually some in.
 */
void ext4_cache_sync_uninit_map(
		struct ext4_irp_ctx *irp_ctx,
		PFILE_OBJECT file_object)
{
	CACHE_UNINITIALIZE_EVENT uninit_complete_event;
	NTSTATUS wait_status;
	LARGE_INTEGER zero_offset;

	zero_offset.QuadPart = 0;
	UNREFERENCED_PARAMETER(irp_ctx);

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
 * @brief This routine is a wraper to CcPinRead.
 */
__bool ext4_cache_pin_read(
		PFILE_OBJECT file_object,
		__s64 file_offset,
		ULONG len,
		__bool wait,
		__bool exclusive,
		void **bcb,
		void **buf)
{
	ULONG flags = 0;
	if (wait)
		flags = PIN_WAIT;
	if (exclusive)
		flags |= PIN_EXCLUSIVE;

	return CcPinRead(
				file_object,
				(PLARGE_INTEGER)&file_offset,
				len,
				flags,
				bcb,
				buf);
}

/**
 * @brief This routine is a wraper to CcPreparePinWrite.
 */
__bool ext4_cache_pin_write(
		PFILE_OBJECT file_object,
		__s64 file_offset,
		ULONG len,
		__bool zero,
		__bool wait,
		void **bcb,
		void **buf)
{
	ULONG flags = PIN_EXCLUSIVE;
	if (wait)
		flags |= PIN_WAIT;

	return CcPreparePinWrite(
				file_object,
				(PLARGE_INTEGER)&file_offset,
				len,
				zero,
				flags,
				bcb,
				buf);
}

/**
 * @brief This routine is a wraper to CcSetDirtyPinnedData.
 */
void ext4_cache_set_dirty(void *bcb, __u32 lsn)
{
	CcSetDirtyPinnedData(bcb, lsn);
}

/**
 * @brief This routine is a wraper to CcUnpinData.
 */
void ext4_cache_unpin_bcb(void *bcb)
{
	CcUnpinData(bcb);
}

/**
 * @brief This routine is a wraper to CcRepinBcb.
 */
void ext4_cache_repin_bcb(void *bcb)
{
	CcRepinBcb(bcb);
}

/**
 * @brief This routine is a wraper to CcUnpinRepinnedBcb.
 */
void ext4_cache_unpin_repinned_bcb(void *bcb)
{
	IO_STATUS_BLOCK io_status;
	CcUnpinRepinnedBcb(bcb, FALSE, &io_status);
	NT_ASSERT(NT_SUCCESS(io_status.Status));
}