/**
 * @file llapr.h
 * @author Phoenix
 * @date 2004-11-28
 * @brief Helper functions for using the apache portable runtime library.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2004-2009, Linden Research, Inc.
 *
 * Second Life Viewer Source Code
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; version 2.1 of the License only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA"
 * $/LicenseInfo$
 */

#pragma once

// Modifies APR declarations for static linking under Windows
#include "llpreprocessor.h"

#if LL_LINUX
# include <sys/param.h>		// Need PATH_MAX in APR headers...
#endif
#include "apr.h"
#include "apr_errno.h"
#include "apr_pools.h"
#include "apr_dso.h"

#include "llstring.h"

extern bool gAPRInitialized;

// Function which appropriately logs error or remains quiet on APR_SUCCESS.
// Returns true if status is an error condition.
bool ll_apr_warn_status(apr_status_t status);

// There is a whole other APR error-message function if you pass a DSO handle.
bool ll_apr_warn_status(apr_status_t status, apr_dso_handle_t* handle);

extern "C" apr_pool_t* gAPRPoolp; // Global APR memory pool

// Initializes the common APR constructs: APR itself, the global pool and a
// mutex.
void ll_init_apr();

// Cleans up those common APR constructs.
void ll_cleanup_apr();

// This class manages apr_pool_t and destroys the allocated APR pool in its
// destructor.

class LLAPRPool
{
public:
	LLAPRPool(apr_pool_t* parent = NULL, apr_size_t size = 0,
			  bool release_pool = true);
	virtual ~LLAPRPool();

	virtual apr_pool_t* getAPRPool();

	LL_INLINE apr_status_t getStatus()	{ return mStatus; }

protected:
	void releaseAPRPool();
	void createAPRPool();

protected:
	apr_pool_t*		mPool;		// Pointing to an apr_pool
	apr_pool_t*		mParent;	// Parent pool

	// Max size of mPool, mPool should return memory to system if allocated
	// memory beyond this limit. However it seems not to work.
	apr_size_t		mMaxSize;

	apr_status_t	mStatus;	// Status when creating the pool

	// If set, mPool is destroyed when LLAPRPool is deleted. Default value is
	// true.
	bool			mReleasePoolFlag;
};
