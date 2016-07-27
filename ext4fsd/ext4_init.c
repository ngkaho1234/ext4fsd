/*++

Copyright (c) 2016 Kaho Ng <ngkaho1234@gmail.com>

Module Name:

ext4_init.c

Abstract:

This module implements the DRIVER_INITIALIZATION routine for Ext4Fsd


--*/


#include "ext4.h"
#include "ext4_data.h"

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING registry_path
);

_Function_class_(DRIVER_UNLOAD)
VOID
ext4_unload(
	_In_ _Unreferenced_parameter_ PDRIVER_OBJECT driver_object
);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(INIT, ext4_unload)
#endif

/**
 * @brief	This is the initialization routine for the ext4fsd file system device driver.
 *
 * This routine creates the device object for the FileSystem device and performs all
 * other driver initialization.
 *
 * @param driver_object	Pointer to driver object created by the system
 * @param registry_path	Registry Path of the driver
 *
 * @return The final status from the initialization operation.
 */
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT driver_object,
	_In_ PUNICODE_STRING registry_path
)
{
	NTSTATUS status;
	UNICODE_STRING device_name;

	UNREFERENCED_PARAMETER(registry_path);

	driver_object->DriverUnload = ext4_unload;

	/*
	 * Create Ext4Fsd cdrom fs deivce
	 */

	RtlInitUnicodeString(&device_name, EXT4_CDROM_DEVICE_NAME);
	status = IoCreateDevice(
				driver_object,
				0,
				&device_name,
				FILE_DEVICE_CD_ROM_FILE_SYSTEM,
				0,
				FALSE,
				&ext4_cdrom_fsd_object);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	/*
	* Create Ext4Fsd disk fs deivce
	*/

	RtlInitUnicodeString(&device_name, EXT4_DISK_DEVICE_NAME);
	status = IoCreateDevice(
				driver_object,
				0,
				&device_name,
				FILE_DEVICE_DISK_FILE_SYSTEM,
				0,
				FALSE,
				&ext4_disk_fsd_object);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	/*
	 * Initialize the driver object with this driver's entry points.
	 */

#if 0
	driver_object->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)FatFsdCreate;
	driver_object->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)FatFsdClose;
	driver_object->MajorFunction[IRP_MJ_READ] = (PDRIVER_DISPATCH)FatFsdRead;
	driver_object->MajorFunction[IRP_MJ_WRITE] = (PDRIVER_DISPATCH)FatFsdWrite;
	driver_object->MajorFunction[IRP_MJ_QUERY_INFORMATION] = (PDRIVER_DISPATCH)FatFsdQueryInformation;
	driver_object->MajorFunction[IRP_MJ_SET_INFORMATION] = (PDRIVER_DISPATCH)FatFsdSetInformation;
	driver_object->MajorFunction[IRP_MJ_QUERY_EA] = (PDRIVER_DISPATCH)FatFsdQueryEa;
	driver_object->MajorFunction[IRP_MJ_SET_EA] = (PDRIVER_DISPATCH)FatFsdSetEa;
	driver_object->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = (PDRIVER_DISPATCH)FatFsdFlushBuffers;
	driver_object->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)FatFsdQueryVolumeInformation;
	driver_object->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = (PDRIVER_DISPATCH)FatFsdSetVolumeInformation;
	driver_object->MajorFunction[IRP_MJ_CLEANUP] = (PDRIVER_DISPATCH)FatFsdCleanup;
	driver_object->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = (PDRIVER_DISPATCH)FatFsdDirectoryControl;
	driver_object->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = (PDRIVER_DISPATCH)FatFsdFileSystemControl;
	driver_object->MajorFunction[IRP_MJ_LOCK_CONTROL] = (PDRIVER_DISPATCH)FatFsdLockControl;
	driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)FatFsdDeviceControl;
	driver_object->MajorFunction[IRP_MJ_SHUTDOWN] = (PDRIVER_DISPATCH)FatFsdShutdown;
	driver_object->MajorFunction[IRP_MJ_PNP] = (PDRIVER_DISPATCH)FatFsdPnp;

	driver_object->FastIoDispatch = &ext4_fast_io_dispatch;;

	RtlZeroMemory(&ext4_fast_io_dispatch, sizeof(ext4_fast_io_dispatch));

	ext4_fast_io_dispatch.SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
	ext4_fast_io_dispatch.FastIoCheckIfPossible = FatFastIoCheckIfPossible;  //  CheckForFastIo
	ext4_fast_io_dispatch.FastIoRead = FsRtlCopyRead;             //  Read
	ext4_fast_io_dispatch.FastIoWrite = FsRtlCopyWrite;            //  Write
	ext4_fast_io_dispatch.FastIoQueryBasicInfo = FatFastQueryBasicInfo;     //  QueryBasicInfo
	ext4_fast_io_dispatch.FastIoQueryStandardInfo = FatFastQueryStdInfo;       //  QueryStandardInfo
	ext4_fast_io_dispatch.FastIoLock = FatFastLock;               //  Lock
	ext4_fast_io_dispatch.FastIoUnlockSingle = FatFastUnlockSingle;       //  UnlockSingle
	ext4_fast_io_dispatch.FastIoUnlockAll = FatFastUnlockAll;          //  UnlockAll
	ext4_fast_io_dispatch.FastIoUnlockAllByKey = FatFastUnlockAllByKey;     //  UnlockAllByKey
	ext4_fast_io_dispatch.FastIoQueryNetworkOpenInfo = FatFastQueryNetworkOpenInfo;
	ext4_fast_io_dispatch.AcquireForCcFlush = FatAcquireForCcFlush;
	ext4_fast_io_dispatch.ReleaseForCcFlush = FatReleaseForCcFlush;
	ext4_fast_io_dispatch.MdlRead = FsRtlMdlReadDev;
	ext4_fast_io_dispatch.MdlReadComplete = FsRtlMdlReadCompleteDev;
	ext4_fast_io_dispatch.PrepareMdlWrite = FsRtlPrepareMdlWriteDev;
	ext4_fast_io_dispatch.MdlWriteComplete = FsRtlMdlWriteCompleteDev;

	/*
	 *  Initialize the cache manager callback routines
	 */

	ext4_cache_manager_callbacks.AcquireForLazyWrite = &FatAcquireFcbForLazyWrite;
	ext4_cache_manager_callbacks.ReleaseFromLazyWrite = &FatReleaseFcbFromLazyWrite;
	ext4_cache_manager_callbacks.AcquireForReadAhead = &FatAcquireFcbForReadAhead;
	ext4_cache_manager_callbacks.ReleaseFromReadAhead = &FatReleaseFcbFromReadAhead;

	ext4_cache_manager_noop_callbacks.AcquireForLazyWrite = &FatNoOpAcquire;
	ext4_cache_manager_noop_callbacks.ReleaseFromLazyWrite = &FatNoOpRelease;
	ext4_cache_manager_noop_callbacks.AcquireForReadAhead = &FatNoOpAcquire;
	ext4_cache_manager_noop_callbacks.ReleaseFromReadAhead = &FatNoOpRelease;

	/*
	 *  Initialize the filter callbacks we use.
	 */

	RtlZeroMemory(
		&ext4_fs_filter_callbacks,
		sizeof(FS_FILTER_CALLBACKS));

	ext4_fs_filter_callbacks.SizeOfFsFilterCallbacks = sizeof(FS_FILTER_CALLBACKS);
//	ext4_fs_filter_callbacks.PreAcquireForSectionSynchronization = FatFilterCallbackAcquireForCreateSection;

	status = FsRtlRegisterFileSystemFilterCallbacks(
				driver_object,
				&ext4_fs_filter_callbacks);

	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(ext4_disk_fsd_object);
		IoDeleteDevice(ext4_cdrom_fsd_object);
		return status;
	}
#endif /* #if 0 */

	/*
	 * Register the file system with the I/O system
	 */

	IoRegisterFileSystem(ext4_disk_fsd_object);
	ObReferenceObject(ext4_disk_fsd_object);
	IoRegisterFileSystem(ext4_cdrom_fsd_object);
	ObReferenceObject(ext4_cdrom_fsd_object);

	/*
	 * Return to our caller
	 */

	return STATUS_SUCCESS;
}

/**
* @brief	This is the unload routine for the filesystem
*
* @param driver_object	Pointer to driver object created by the system
*/
_Function_class_(DRIVER_UNLOAD)
VOID
ext4_unload(
	_In_ _Unreferenced_parameter_ PDRIVER_OBJECT driver_object
)
{
	UNREFERENCED_PARAMETER(driver_object);
	
	/*
	 * Delete disk and cdrom file system device created
	 */
	IoDeleteDevice(ext4_disk_fsd_object);
	IoDeleteDevice(ext4_cdrom_fsd_object);
}