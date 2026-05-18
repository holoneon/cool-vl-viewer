/**
 * @file llimagedecodethread.cpp
 * @brief Image decode thread class implementation.
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 *
 * Copyright (c) 2001-2009, Linden Research, Inc.
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

#include "linden_common.h"

#include "llimagedecodethread.h"

#include "llapp.h"			// For LLApp::isExiting()
#include "llsys.h"			// For LLCPUInfo
#include "lltimer.h"		// For ms_sleep()
#include "hbtracy.h"

// Global variable
LLImageDecodeThread* gImageDecodeThreadp = NULL;

///////////////////////////////////////////////////////////////////////////////
// LLImageDecodeThread::ImageRequest sub-class. Where decode actually happens.
///////////////////////////////////////////////////////////////////////////////

LLImageDecodeThread::ImageRequest::ImageRequest(
	const LLPointer<LLImageFormatted>& image, S32 discard, bool needs_aux,
	const LLPointer<Responder>& responder)

:	mFormattedImage(image),
	mDiscardLevel(discard),
	mNeedsAux(needs_aux),
	mResponder(responder),
	mDecodedRaw(false),
	mDecodedAux(false)
{
}

//virtual
LLImageDecodeThread::ImageRequest::~ImageRequest()
{
	mDecodedImageRaw = NULL;
	mDecodedImageAux = NULL;
	mFormattedImage = NULL;
}

bool LLImageDecodeThread::ImageRequest::processRequest()
{
	if (mFormattedImage.isNull())
	{
		return true;
	}

	bool done = true;

	if (!mDecodedRaw)
	{
		// Decode primary channels
		if (mDecodedImageRaw.isNull())
		{
			// Parse formatted header
			if (!mFormattedImage->updateData())
			{
				return true;	// Done (failed)
			}
			U16 width = mFormattedImage->getWidth();
			U16 height = mFormattedImage->getHeight();
			S8 comps = mFormattedImage->getComponents();
			if (width * height * comps == 0)
			{
				return true;	// Done (failed)
			}
			if (mDiscardLevel >= 0)
			{
				mFormattedImage->setDiscardLevel(mDiscardLevel);
			}
			mDecodedImageRaw = new LLImageRaw(width, height, comps);
		}
		if (mDecodedImageRaw.notNull() && mDecodedImageRaw->getData())
		{
			done = mFormattedImage->decode(mDecodedImageRaw);
			// Some decoders are removing data when task is complete and
			// there were errors
			mDecodedRaw = done && mDecodedImageRaw->getData();
		}
		else
		{
			llwarns_sparse << "Failed to allocate raw image !" << llendl;
			return true;	// Done (failed)
		}
	}

	if (done && mNeedsAux && !mDecodedAux)
	{
		// Decode aux channel
		if (mDecodedImageAux.isNull())
		{
			mDecodedImageAux = new LLImageRaw(mFormattedImage->getWidth(),
											  mFormattedImage->getHeight(),
											  1);
		}
		if (mDecodedImageAux.notNull() && mDecodedImageAux->getData())
		{
			done = mFormattedImage->decodeChannels(mDecodedImageAux, 4, 4);
			// Some decoders are removing data when task is complete and
			// there were errors
			mDecodedAux = done && mDecodedImageAux->getData();
		}
		else
		{
			llwarns_sparse << "Failed to allocate raw image !" << llendl;
		}
	}

	return done;
}

void LLImageDecodeThread::ImageRequest::finishRequest(bool completed)
{
	if (mResponder.notNull())
	{
		bool success = completed && mDecodedRaw &&
					   mDecodedImageRaw && mDecodedImageRaw->getDataSize() &&
					   (!mNeedsAux || mDecodedAux);
		mResponder->completed(success, mDecodedImageRaw, mDecodedImageAux);
	}
	// Will automatically be deleted
}

///////////////////////////////////////////////////////////////////////////////
// LLImageDecodeThread class proper
///////////////////////////////////////////////////////////////////////////////

LLImageDecodeThread::LLImageDecodeThread(U32 pool_size)
{
	if (!pool_size)	// Auto-determined size requested by user
	{
		// Limit the number of threads in the pool to 32 maximum (more than
		// this is totally useless, even when flying over main land with 512m
		// draw distance. HB
		pool_size = llmin(LLCPUInfo::getInstance()->getMaxThreadConcurrency(),
						  32U);
	}
	llinfos << "Initializing with " << pool_size << " worker threads."
			<< llendl;
	mThreadPoolp.reset(new LLThreadPool("Image decode", pool_size));
	mThreadPoolp->start(true);	// true = wait until all threads are started.
}

void LLImageDecodeThread::shutdown()
{
	if (mThreadPoolp)
	{
		mThreadPoolp->close();
		mThreadPoolp.reset(nullptr);
		llinfos << "Thread pool destroyed." << llendl;
	}
}

size_t LLImageDecodeThread::getPending()
{
	return mThreadPoolp ? mThreadPoolp->getQueue().size() : 0;
}

bool LLImageDecodeThread::decodeImage(const LLPointer<LLImageFormatted>& image,
									  S32 discard, bool needs_aux,
									  const LLPointer<Responder>& responder)
{
	if (!mThreadPoolp || LLApp::isExiting())
	{
		return false;
	}

	mThreadPoolp->getQueue().post(
		[req = ImageRequest(image, discard, needs_aux, responder)]() mutable
		{
			LL_TRACY_TIMER(TRC_IMG_DECODE);
			if (!LLApp::isExiting())
			{
				req.finishRequest(req.processRequest());
			}
		});

	return true;
}
