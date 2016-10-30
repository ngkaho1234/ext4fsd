/*
 * Locking mechanism routines reside in this header file
 */

#pragma once

#include <ntifs.h>

#include "drv_types.h"

/*
 * Definition of read-write lock and mutex
 */
typedef ERESOURCE drv_res_t;
typedef FAST_MUTEX drv_mutex_t;

/**
 * @brief	Initialize an resource
 *
 * @param resource	The resource to be initialized
 *
 * @return	STATUS_SUCCESS if the operation succeeds.
 */
static inline NTSTATUS drv_resource_init(drv_res_t *resource)
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
static inline NTSTATUS drv_resource_destroy(drv_res_t *resource)
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
static inline NTSTATUS drv_resource_acquire_shared(drv_res_t *resource, __bool wait)
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
static inline NTSTATUS drv_resource_acquire_exclusive(drv_res_t *resource, __bool wait)
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
static inline void drv_resource_release(drv_res_t *resource)
{
	ExReleaseResourceLite(resource);
}

/**
 * @brief	Downgrade to shared access to an resource
 *
 * @param resource	The access to resource to be downgraded
 */
static inline void drv_resource_acquire_downgrade(drv_res_t *resource)
{
	ExConvertExclusiveToSharedLite(resource);
}

/**
 * @brief	Return the number of times the caller has acquired the resource
 *
 * @return	The number of times the caller has acquired the resource
 */
static inline ULONG drv_resource_acquired_count(drv_res_t *resource)
{
	return ExIsResourceAcquiredLite(resource);
}

/**
 * @brief	Return whether the resource is acquired exclusively
 *
 * @return	Whether the resource is acquired exclusively
 */
static inline __bool drv_resource_is_acquired_exclusively(drv_res_t *resource)
{
	return !!ExIsResourceAcquiredExclusiveLite(resource);
}

/**
 * @brief	Initialize a mutex
 *
 * @param mutex	The mutex to be initialized
 */
static inline void drv_mutex_init(drv_mutex_t *mutex)
{
	ExInitializeFastMutex(mutex);
}

/**
* @brief	Destroy a mutex
*
* @param mutex	The mutex to be destroyed
*/
static inline void drv_mutex_destroy(drv_mutex_t *mutex)
{
	/*
	 * NOOP for fast mutex
	 */
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
static inline NTSTATUS drv_mutex_acquire(drv_mutex_t *mutex, __bool wait)
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
static inline void drv_mutex_release(drv_mutex_t *mutex)
{
	ExReleaseFastMutex(mutex);
}