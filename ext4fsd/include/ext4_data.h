#pragma once

/*
* Global data definitions
*/
extern PDEVICE_OBJECT ext4_disk_fsd_object;
extern PDEVICE_OBJECT ext4_cdrom_fsd_object;
extern FAST_IO_DISPATCH ext4_fast_io_dispatch;
extern FS_FILTER_CALLBACKS ext4_fs_filter_callbacks;

extern CACHE_MANAGER_CALLBACKS ext4_cache_manager_callbacks;
extern CACHE_MANAGER_CALLBACKS ext4_cache_manager_noop_callbacks;

#define EXT4_DISK_DEVICE_NAME L"\\Ext4Fsd"
#define EXT4_CDROM_DEVICE_NAME L"\\Ext4CdromFsd"
