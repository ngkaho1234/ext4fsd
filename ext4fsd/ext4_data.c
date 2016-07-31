/*
 * This module contains some global data definitions for Ext4Fsd
 */

#include "ext4.h"

/*
 * Global data definitions
 */
PDEVICE_OBJECT ext4_disk_fsd_object;
PDEVICE_OBJECT ext4_cdrom_fsd_object;
FAST_IO_DISPATCH ext4_fast_io_dispatch;
FS_FILTER_CALLBACKS ext4_fs_filter_callbacks;

CACHE_MANAGER_CALLBACKS ext4_cache_manager_callbacks;
CACHE_MANAGER_CALLBACKS ext4_cache_manager_noop_callbacks;