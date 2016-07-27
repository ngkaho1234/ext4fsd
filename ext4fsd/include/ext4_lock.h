/*
 * Locking mechanism routines reside in this header file
 */

#pragma once

#include <ntifs.h>

#include "ext4_types.h"

/*
 * Definition of read-write lock and mutex
 */
typedef ERESOURCE ext4_res_t;
typedef FAST_MUTEX ext4_mutex_t;

/**
 * @brief	Initialize an resource
 *
 * @param resource	The resource to be initialized
 *
 * @return	STATUS_SUCCESS if the operation succeeds.
 */
static inline NTSTATUS ext4_resource_init(ext4_res_t *resource)
{
	return ExInitializeResourceLite(resource);
}

/**
 * @brief	Destroy an resource
 *
 * @param resource	The resource to be destroyed
 *
 * @return	STATUS_SUCCESS if the operation succeeds.
 */
static inline NTSTATUS ext4_resource_destroy(ext4_res_t *resource)
{
	return ExDeleteResourceLite(resource);
}

/**
 * @brief	Acquire shared access to an resource
 *
 * @param resource	The resource to be acquired
 * @param wait		Whether the operation should block or not
 *
 * @return	STATUS_SUCCESS if the operation succeeds.
 *			If the operation can't succeed without blocking,
 *			STATUS_CANT_WAIT will be returned.
 */
static inline NTSTATUS ext4_resource_acquire_shared(ext4_res_t *resource, __bool wait)
{
	return (ExAcquireResourceSharedLite(resource, wait) == TRUE) ?
				STATUS_SUCCESS :
				STATUS_CANT_WAIT;
}

/**
 * @brief	Acquire exclusive access to an resource
 *
 * @param resource	The resource to be acquired
 * @param wait		Whether the operation should block or not
 *
 * @return	STATUS_SUCCESS if the operation succeeds.
 *			If the operation can't succeed without blocking,
 *			STATUS_CANT_WAIT will be returned.
 */
static inline NTSTATUS ext4_resource_acquire_exclusive(ext4_res_t *resource, __bool wait)
{
	return (ExAcquireResourceSharedLite(resource, wait) == TRUE) ?
				STATUS_SUCCESS :
				STATUS_CANT_WAIT;
}

/**
 * @brief	Release an resource
 *
 * @param resource	The resource to be released
 */
static inline void ext4_resource_release(ext4_res_t *resource)
{
	ExReleaseResourceLite(resource);
}

/**
 * @brief	Downgrade to shared access to an resource
 *
 * @param resource	The access to resource to be downgraded
 */
static inline void ext4_resource_acquire_downgrade(ext4_res_t *resource)
{
	ExConvertExclusiveToSharedLite(resource);
}

/**
 * @brief	Return the number of times the caller has acquired the resource
 *
 * @return	The number of times the caller has acquired the resource
 */
static inline ULONG ext4_resource_acquired_count(ext4_res_t *resource)
{
	return ExIsResourceAcquiredLite(resource);
}

/**
 * @brief	Return whether the resource is acquired exclusively
 *
 * @return	Whether the resource is acquired exclusively
 */
static inline __bool ext4_resource_is_acquired_exclusively(ext4_res_t *resource)
{
	return !!ExIsResourceAcquiredExclusiveLite(resource);
}

/**
 * @brief	Initialize a mutex
 *
 * @param mutex	The mutex to be initialized
 */
static inline void ext4_mutex_init(ext4_mutex_t *mutex)
{
	ExInitializeFastMutex(mutex);
}

/**
 * @brief	Acquire access to mutex
 *
 * @param mutex	The mutex to be acquired
 * @param wait	Whether the operation should block or not
 *
 * @return	STATUS_SUCCESS if wait == FALSE, otherwise it
 *			depends on whether the mutex could be acquired
 *			without blocking or not. In case the mutex cannot
 *			be acquired exclusively, STATUS_CANT_WAIT will
 *			be returned
 */
static inline NTSTATUS ext4_mutex_acquire(ext4_mutex_t *mutex, __bool wait)
{
	if (wait) {
		ExAcquireFastMutex(mutex);
		return STATUS_SUCCESS;
	}
	return (ExTryToAcquireFastMutex(mutex) == TRUE) ?
				STATUS_SUCCESS :
				STATUS_CANT_WAIT;
}

/**
 * @brief	Release the mutex
 *
 * @param mutex	The mutex to be released
 */
static inline void ext4_mutex_release(ext4_mutex_t *mutex)
{
	ExReleaseFastMutex(mutex);
}