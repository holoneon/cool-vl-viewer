/**
 * @file llgltfanimation.cpp
 * @brief LLGLTF Animation Implementation
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#include "linden_common.h"

#include "llgltfanimation.h"

#include "llgltfasset.h"
#include "llgltfbufferutil.h"

using namespace LLGLTF;

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::Sampler sub-class
///////////////////////////////////////////////////////////////////////////////

bool Animation::Sampler::prep(Asset& asset)
{
	Accessor& accessor = asset.mAccessors[mInput];
	mMinTime = accessor.mMin[0];
	mMaxTime = accessor.mMax[0];
	mFrameTimes.resize(accessor.mCount);
	LLStrider<F32> frame_times = mFrameTimes.data();
	copy(asset, accessor, frame_times);
	return true;
}

const Animation::Sampler& Animation::Sampler::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "input", mInput);
		copy(src, "output", mOutput);
		copy(src, "interpolation", mInterpolation);
		copy(src, "min_time", mMinTime);
		copy(src, "max_time", mMaxTime);
	}
	return *this;
}

void Animation::Sampler::serialize(lljson& obj) const
{
	write(mInput, "input", obj, INVALID_INDEX);
	write(mOutput, "output", obj, INVALID_INDEX);
	write(mInterpolation, "interpolation", obj, std::string("LINEAR"));
	write(mMinTime, "min_time", obj);
	write(mMaxTime, "max_time", obj);
}

void Animation::Sampler::getFrameInfo(Asset& asset, F32 time, U32& frame_idx,
									  F32& t)
{
	frame_idx = 0;

	if (time < mMinTime)
	{
		t = 0.f;
		return;
	}

	if (mFrameTimes.empty())
	{
		t = 1.f;
		return;
	}

	if (time > mMaxTime)
	{
		if (mFrameTimes.size() > 2)
		{
			frame_idx = mFrameTimes.size() - 2;
		}
		t = 1.f;
		return;
	}

	if (time < mLastFrameTime)
	{
		mLastFrameIndex = 0;
	}
	mLastFrameTime = time;

	for (U32 i = mLastFrameIndex, count = mFrameTimes.size() - 1; i < count;
		 ++i)
	{
		if (time >= mFrameTimes[i] && time < mFrameTimes[i + 1])
		{
			frame_idx = mLastFrameIndex = i;
			t = (time - mFrameTimes[i]) /
				(mFrameTimes[i + 1] - mFrameTimes[i]);
			return;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::Channel sub-class
///////////////////////////////////////////////////////////////////////////////

const Animation::Channel& Animation::Channel::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "sampler", mSampler);
		copy(src, "target", mTarget);
	}
	return *this;
}

void Animation::Channel::serialize(lljson& obj) const
{
	write(mSampler, "sampler", obj, INVALID_INDEX);
	write(mTarget, "target", obj);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::Channel::Target sub-class
///////////////////////////////////////////////////////////////////////////////

bool Animation::Channel::Target::operator==(const Channel::Target& rhs) const
{
	return mNode == rhs.mNode && mPath == rhs.mPath;
}

bool Animation::Channel::Target::operator!=(const Channel::Target& rhs) const
{
	return mNode != rhs.mNode || mPath != rhs.mPath;
}

const Animation::Channel::Target& Animation::Channel::Target::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "node", mNode);
		copy(src, "path", mPath);
	}
	return *this;
}

void Animation::Channel::Target::serialize(lljson& obj) const
{
	write(mNode, "node", obj, INVALID_INDEX);
	write(mPath, "path", obj);
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::RotationChannel sub-class
///////////////////////////////////////////////////////////////////////////////

bool Animation::RotationChannel::prep(Asset& asset, Animation::Sampler& sampler)
{
	Accessor& accessor = asset.mAccessors[sampler.mOutput];
	copy(asset, accessor, mRotations);
	return true;
}

void Animation::RotationChannel::apply(Asset& asset, Sampler& sampler,
									   F32 time)
{
	Node& node = asset.mNodes[mTarget.mNode];

	if (sampler.mFrameTimes.size() < 2)
	{
		node.setRotation(mRotations[0]);
	}
	else
	{
		U32 frame_idx;
		F32 t;
		sampler.getFrameInfo(asset, time, frame_idx, t);

		// Interpolate
		quat qf = glm::slerp(mRotations[frame_idx],
							 mRotations[frame_idx + 1], t);
		qf = glm::normalize(qf);
		node.setRotation(qf);
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::TranslationChannel sub-class
///////////////////////////////////////////////////////////////////////////////

bool Animation::TranslationChannel::prep(Asset& asset,
										 Animation::Sampler& sampler)
{
	Accessor& accessor = asset.mAccessors[sampler.mOutput];
	copy(asset, accessor, mTranslations);
	return true;
}

void Animation::TranslationChannel::apply(Asset& asset, Sampler& sampler,
										  F32 time)
{
	Node& node = asset.mNodes[mTarget.mNode];

	if (sampler.mFrameTimes.size() < 2)
	{
		node.setTranslation(mTranslations[0]);
	}
	else
	{
		U32 frame_idx;
		F32 t;
		sampler.getFrameInfo(asset, time, frame_idx, t);

		// Interpolate
		const vec3& v0 = mTranslations[frame_idx];
		const vec3& v1 = mTranslations[frame_idx + 1];
		node.setTranslation(v0 + t * (v1 - v0));
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation::ScaleChannel sub-class
///////////////////////////////////////////////////////////////////////////////

bool Animation::ScaleChannel::prep(Asset& asset, Animation::Sampler& sampler)
{
	Accessor& accessor = asset.mAccessors[sampler.mOutput];
	copy(asset, accessor, mScales);
	return true;
}

void Animation::ScaleChannel::apply(Asset& asset, Sampler& sampler, F32 time)
{
	Node& node = asset.mNodes[mTarget.mNode];

	if (sampler.mFrameTimes.size() < 2)
	{
		node.setScale(mScales[0]);
	}
	else
	{
		U32 frame_idx;
		F32 t;
		sampler.getFrameInfo(asset, time, frame_idx, t);

		// Interpolate
		const vec3& v0 = mScales[frame_idx];
		const vec3& v1 = mScales[frame_idx + 1];
		node.setScale(v0 + t * (v1 - v0));
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLGLTF::Animation class
///////////////////////////////////////////////////////////////////////////////

bool Animation::prep(Asset& asset)
{
	if (!mSamplers.empty())
	{
		mMinTime = FLT_MAX;
		mMaxTime = -FLT_MAX;
		for (auto& sampler : mSamplers)
		{
			if (!sampler.prep(asset))
			{
				return false;
			}
			mMinTime = llmin(sampler.mMinTime, mMinTime);
			mMaxTime = llmax(sampler.mMaxTime, mMaxTime);
		}
	}
	else
	{
		mMinTime = mMaxTime = 0.f;
	}

	for (auto& channel : mRotationChannels)
	{
		if (!channel.prep(asset, mSamplers[channel.mSampler]))
		{
			return false;
		}
	}

	for (auto& channel : mTranslationChannels)
	{
		if (!channel.prep(asset, mSamplers[channel.mSampler]))
		{
			return false;
		}
	}

	for (auto& channel : mScaleChannels)
	{
		if (!channel.prep(asset, mSamplers[channel.mSampler]))
		{
			return false;
		}
	}

	return true;
}

void Animation::update(Asset& asset, F32 dt)
{
	mTime += dt;
	apply(asset, mTime);
}

void Animation::apply(Asset& asset, F32 time)
{
	// Convert time to animation loop time
	time = fmod(time, mMaxTime - mMinTime) + mMinTime;

	// Apply each channel
	for (auto& channel : mRotationChannels)
	{
		channel.apply(asset, mSamplers[channel.mSampler], time);
	}

	for (auto& channel : mTranslationChannels)
	{
		channel.apply(asset, mSamplers[channel.mSampler], time);
	}

	for (auto& channel : mScaleChannels)
	{
		channel.apply(asset, mSamplers[channel.mSampler], time);
	}
}

const Animation& Animation::operator=(const lljson& src)
{
	if (src.is_object())
	{
		copy(src, "name", mName);
		copy(src, "samplers", mSamplers);

		// Make a temporory copy of generic channels
		std::vector<Channel> channels;
		copy(src, "channels", channels);
		// Break up into channel specific implementations
		for (auto& channel : channels)
		{
			if (channel.mTarget.mPath == "rotation")
			{
				mRotationChannels.push_back(channel);
			}
			else if (channel.mTarget.mPath == "translation")
			{
				mTranslationChannels.push_back(channel);
			}
			else if (channel.mTarget.mPath == "scale")
			{
				mScaleChannels.push_back(channel);
			}
		}
	}
	return *this;
}

void Animation::serialize(lljson& obj) const
{
	write(mName, "name", obj);
	write(mSamplers, "samplers", obj);

	std::vector<Channel> channels;
	channels.insert(channels.end(), mRotationChannels.begin(),
					mRotationChannels.end());
	channels.insert(channels.end(), mTranslationChannels.begin(),
					mTranslationChannels.end());
	channels.insert(channels.end(), mScaleChannels.begin(),
					mScaleChannels.end());
	write(channels, "channels", obj);
}
