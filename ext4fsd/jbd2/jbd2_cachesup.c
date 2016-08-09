/*
* Copyright (c) 2015 Kaho Ng (ngkaho1234@gmail.com)
*/

#include "jbd2\jbd2.h"

/**
 * @brief Routine to be called by lazywriter before writing something
 * @param context	User-specified context
 * @param wait	If blocking is desired
 * @return TRUE
 */
__bool jbd2_cc_acquire_for_lazywrite(
		void *context,
		__bool wait
)
{
	return TRUE;
}

/**
 * @brief Routine to be called by lazywriter before writing something
 * @param context	User-specified context
 * @param wait	If blocking is desired
 * @return TRUE
 */
void jbd2_cc_release_from_lazywrite(
		void *context
)
{
}

/**
 * @brief Routine to be called by lazywriter before writing something
 * @param context	User-specified context
 * @param wait	If blocking is desired
 * @return TRUE
 */
__bool jbd2_cc_acquire_for_readahead(
		void *context,
		__bool wait
)
{
	return TRUE;
}

/**
 * @brief Routine to be called by lazywriter before writing something
 * @param context	User-specified context
 * @param wait	If blocking is desired
 * @return TRUE
 */
void jbd2_cc_release_from_readahead(
		void *context
)
{
}
