/**
 * @file llimagedecodethread.h
 * @brief Image decode thread class declaration.
 *
 * $LicenseInfo:firstyear=2000&license=viewerlgpl$
 *
 * Copyright (c) 2000-2009, Linden Research, Inc.
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

#include <memory>

#include "llimage.h"
#include "llpointer.h"
#include "llrefcount.h"
#include "llthreadpool.h"

class LLImageDecodeThread
{
protected:
	LOG_CLASS(LLImageDecodeThread);

public:
	class Responder : public LLThreadSafeRefCount
	{
	protected:
		LOG_CLASS(LLImageDecodeThread::Responder);

		~Responder() override = default;

	public:
		virtual void completed(bool success, LLImageRaw* raw,
							   LLImageRaw* aux) = 0;
	};

	// 'pool_size' is the number of LLThreads that will be launched. When
	// omitted or equal to 0, this number is determined automatically
	// depending on the available threading concurrency.
	LLImageDecodeThread(U32 pool_size = 0);

	void shutdown();

	size_t getPending();

	bool decodeImage(const LLPointer<LLImageFormatted>& image, S32 discard,
					 bool needs_aux, const LLPointer<Responder>& responder);

private:
	class ImageRequest
	{
	protected:
		LOG_CLASS(LLImageDecodeThread::ImageRequest);

	public:
		ImageRequest(const LLPointer<LLImageFormatted>& image,
					 S32 discard, bool needs_aux,
					 const LLPointer<Responder>& responder);
		~ImageRequest();

		bool processRequest();
		void finishRequest(bool completed);

	private:
		// Input
		LLPointer<LLImageFormatted>					mFormattedImage;
		LLPointer<LLImageRaw>						mDecodedImageRaw;
		LLPointer<LLImageRaw>						mDecodedImageAux;
		LLPointer<LLImageDecodeThread::Responder>	mResponder;
		S32											mDiscardLevel;
		bool										mNeedsAux;
		// Output
		bool										mDecodedRaw;
		bool										mDecodedAux;
	};

private:
	std::unique_ptr<LLThreadPool>					mThreadPoolp;
};

// Global, initialized in llappviewer.cpp and used in newview/. Moved here so
// that LLImageDecodeThread consumers do not need to include llappviewer.h to
// use it. HB
extern LLImageDecodeThread* gImageDecodeThreadp;
