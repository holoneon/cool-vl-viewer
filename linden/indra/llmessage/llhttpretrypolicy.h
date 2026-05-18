/**
 * @file llhttpretrypolicy.h
 * @brief Declarations for http retry policy class.
 *
 * $LicenseInfo:firstyear=2004&license=viewerlgpl$
 *
 * Copyright (c) 2013, Linden Research, Inc.
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

#include "llcorehttpheaders.h"
#include "llcorehttpresponse.h"
#include "llerror.h"
#include "llthread.h"
#include "lltimer.h"

// This is intended for use with HTTP Clients/Responders, but is not
// specifically coupled with those classes.
class LLHTTPRetryPolicy : public LLThreadSafeRefCount
{
public:
	LLHTTPRetryPolicy()				{}

	~LLHTTPRetryPolicy() override	{}

	// Call after a sucess to reset retry state.
	virtual void onSuccess() = 0;

	// Call once after an HTTP failure to update state.
	virtual void onFailure(S32 status, const LLSD& headers) = 0;
	virtual void onFailure(const LLCore::HttpResponse* response) = 0;

	virtual bool shouldRetry(F32& seconds_to_wait) const = 0;

	virtual void reset() = 0;
};

// Very general policy with geometric back-off after failures, up to a maximum
// delay, and maximum number of retries.
class LLAdaptiveRetryPolicy : public LLHTTPRetryPolicy
{
protected:
	LOG_CLASS(LLAdaptiveRetryPolicy);

public:
	LLAdaptiveRetryPolicy(F32 min_delay, F32 max_delay, F32 backoff_factor,
						  U32 max_retries, bool retry_on_4xx = false);

	void onSuccess() override;

	void onFailure(S32 status, const LLSD& headers) override;
	void onFailure(const LLCore::HttpResponse* response) override;

	bool shouldRetry(F32& seconds_to_wait) const override;

	void reset() override;

	static bool getSecondsUntilRetryAfter(const std::string& retry_after,
										  F32& seconds_to_wait);

protected:
	void init();

	bool getRetryAfter(const LLSD& headers, F32& retry_header_time);
	bool getRetryAfter(const LLCore::HttpHeaders::ptr_t& headers,
					   F32& retry_header_time);

	void onFailureCommon(S32 status, bool has_retry_header_time,
						 F32 retry_header_time);

private:
	const F32	mMinDelay;		// Delay never less than this value
	const F32	mMaxDelay;		// Delay never exceeds this value

	// Delay increases by this factor after each retry, up to mMaxDelay.
	const F32	mBackoffFactor;

	// Maximum number of times shouldRetry will return true.
	const U32	mMaxRetries;

	F32			mDelay;			// Current default delay.
	U32			mRetryCount;	// Number of times shouldRetry has been called.
	LLTimer		mRetryTimer;	// Time until next retry.

	// Becomes false after too many retries, or the wrong sort of status
	// received etc.
	bool		mShouldRetry;

	bool		mRetryOn4xx;	// Normally only retry on 5xx server errors.
};
