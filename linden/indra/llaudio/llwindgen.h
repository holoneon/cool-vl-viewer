/** 
 * @file windgen.h
 * @brief Templated wind noise generation
 *
 * $LicenseInfo:firstyear=2002&license=viewerlgpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
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

#include "llcommon.h"
#include "llpreprocessor.h"
#include "llrand.h"

template <class MIXBUFFERFORMAT_T>
class LLWindGen
{
public:
	LLWindGen(U32 sample_rate = 44100)
	:	mTargetGain(0.f),
		mTargetFreq(100.f),
		mTargetPanGainR(0.5f),
		mInputSamplingRate(sample_rate),
		mSubSamples(2),
		mFilterBandWidth(50.f),
		mBuf0(0.f),
		mBuf1(0.f),
		mBuf2(0.f),
		mY0(0.f),
		mY1(0.f),
		mCurrentGain(0.f),
		mCurrentFreq(100.f),
		mCurrentPanGainR(0.5f)
	{
		mSamplePeriod = (F32)mSubSamples / (F32)mInputSamplingRate;
		mB2 = expf(-F_TWO_PI * mFilterBandWidth * mSamplePeriod);
	}

	LL_INLINE U32 getInputSamplingRate()		{ return mInputSamplingRate; }
	F32 getNextSample();
	F32 getClampedSample(bool clamp, F32 sample);

	// newbuffer = the buffer passed from the previous DSP unit.
	// numsamples = length in samples-per-channel at this mix time.
	// NOTE: generates L/R interleaved stereo
	MIXBUFFERFORMAT_T* windGenerate(MIXBUFFERFORMAT_T* newbuffer, int numsamples)
	{
		MIXBUFFERFORMAT_T *cursamplep = newbuffer;
		
		// Filter coefficients
		F32 a0 = 0.f, b1 = 0.f;
		
		// No need to clip at normal volumes
		bool clip = mCurrentGain > 2.f;
		
		bool interp_freq = false; 
		
		// If the frequency is not changing much, we do not need to interpolate
		// in the inner loop
		if (fabsf(mTargetFreq - mCurrentFreq) < (mCurrentFreq * 0.112f))
		{
			// calculate resonant filter coefficients
			mCurrentFreq = mTargetFreq;
			b1 = (-4.f * mB2) / (1.f + mB2) *
				 cosf(F_TWO_PI * (mCurrentFreq * mSamplePeriod));
			a0 = (1.f - mB2) * sqrtf(1.f - (b1 * b1) / (4.f * mB2));
		}
		else
		{
			interp_freq = true;
		}
		
		while (numsamples)
		{
			F32 next_sample;
			
			// Start with white noise. This expression is fragile, rearrange it
			// and it will break !
			next_sample = getNextSample();
			
			// Apply a pinking filter. Magic numbers taken from PKE method at:
			// http://www.firstpr.com.au/dsp/pink-noise/
			mBuf0 = mBuf0 * 0.99765f + next_sample * 0.0990460f;
			mBuf1 = mBuf1 * 0.96300f + next_sample * 0.2965164f;
			mBuf2 = mBuf2 * 0.57000f + next_sample * 1.0526913f;
			
			next_sample = mBuf0 + mBuf1 + mBuf2 + next_sample * 0.1848f;
			
			if (interp_freq)
			{
				// calculate and interpolate resonant filter coefficients
				mCurrentFreq = 0.999f * mCurrentFreq + 0.001f * mTargetFreq;
				b1 = (-4.f * mB2) / (1.f + mB2) *
					 cosf(F_TWO_PI * (mCurrentFreq * mSamplePeriod));
				a0 = (1.f - mB2) * sqrtf(1.f - (b1 * b1) / (4.f * mB2));
			}
			
			// Apply a resonant low-pass filter on the pink noise
	        next_sample = a0 * next_sample - b1 * mY0 - mB2 * mY1;
			mY1 = mY0;
			mY0 = next_sample;
			
			mCurrentGain = 0.999f * mCurrentGain + 0.001f * mTargetGain;
			mCurrentPanGainR = 0.999f * mCurrentPanGainR +
							   0.001f * mTargetPanGainR;
			
			// For a 3dB pan law use:
			// next_sample *= mCurrentGain *
			//                ((mCurrentPanGainR * (mCurrentPanGainR - 1) *
			//				  1.652 + 1.413);
		    next_sample *= mCurrentGain;
			
			// delta is used to interpolate between synthesized samples
			F32 delta = (next_sample - mLastSample) / (F32)mSubSamples;
			
			// Fill the audio buffer, clipping if necessary
			MIXBUFFERFORMAT_T sample_left, sample_right;
			for (U8 i = mSubSamples; i && numsamples; --i, --numsamples) 
			{
				mLastSample = mLastSample + delta;
				sample_right =
					(MIXBUFFERFORMAT_T)getClampedSample(clip,
														mLastSample *
														mCurrentPanGainR);
				sample_left =
					(MIXBUFFERFORMAT_T)getClampedSample(clip,
														mLastSample -
														(F32)sample_right);
				*cursamplep++ = sample_left;
				*cursamplep++ = sample_right;
			}
		}
		
		return newbuffer;
	}
	
public:
	F32 mTargetGain;
	F32 mTargetFreq;
	F32 mTargetPanGainR;
	
private:
	U32 mInputSamplingRate;
	U8  mSubSamples;
	F32 mSamplePeriod;
	F32 mFilterBandWidth;
	F32 mB2;
	
	F32 mBuf0;
	F32 mBuf1;
	F32 mBuf2;
	F32 mY0;
	F32 mY1;
	
	F32 mCurrentGain;
	F32 mCurrentFreq;
	F32 mCurrentPanGainR;
	F32 mLastSample;
};

template<class T> LL_INLINE F32 LLWindGen<T>::getNextSample()
{
	constexpr F32 range = 1.f / (F32)(RAND_MAX / (U16_MAX / 8));
	constexpr F32 offset = (F32)(S16_MIN / 8);
	return (F32)rand() * range + offset;
}

template<> LL_INLINE F32 LLWindGen<F32>::getNextSample()
{
	return ll_frand() - .5f;
}

template<class T> LL_INLINE F32 LLWindGen<T>::getClampedSample(bool clamp,
															   F32 sample)
{
	return clamp ? (F32)llclamp((S32)sample, (S32)S16_MIN, (S32)S16_MAX)
				 : sample;
}

template<> LL_INLINE F32 LLWindGen<F32>::getClampedSample(bool clamp,
														  F32 sample)
{
	return sample;
}
