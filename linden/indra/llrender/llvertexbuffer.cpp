/**
 * @file llvertexbuffer.cpp
 * @brief LLVertexBuffer implementation
 *
 * $LicenseInfo:firstyear=2003&license=viewerlgpl$
 *
 * Copyright (c) 2003-2009, Linden Research, Inc.
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

#include <algorithm>				// For std::sort
#include <utility>					// For std::move

#include "llvertexbuffer.h"

#include "llapp.h"					// For LLApp::isExiting()
#include "llglslshader.h"
#include "llmemory.h"				// For ll_aligned_*()
#include "llrender.h"				// For LLRender::sCurrentFrame
#include "llshadermgr.h"
#include "llsys.h"
#include "hbtracy.h"

// Define to 1 to enable (but this is slow).
#define TRACE_WITH_TRACY 0

#if !TRACE_WITH_TRACY
# undef LL_TRACY_TIMER
# define LL_TRACY_TIMER(name)
#endif

// Helper functions

// Next Highest Power Of Two: returns first number > v that is a power of 2, or
// v if v is already a power of 2
U32 nhpo2(U32 v)
{
	U32 r = 2;
	while (r < v) r *= 2;
	return r;
}

// Which power of 2 is i ? Assumes i is a power of 2 > 0.
U32 wpo2(U32 i)
{
	llassert(i > 0 && nhpo2(i) == i);
	U32 r = 0;
	while (i >>= 1) ++r;
	return r;
}

static void flush_vbo(GLenum target, U32 start, U32 end, U8* data)
{
	if (end)
	{
		constexpr U32 block_size = 65536;
		for (U32 i = start; i <= end; i += block_size)
		{
			U32 tend = llmin(i + block_size, end);
			glBufferSubData(target, i, tend - i + 1, data + (i - start));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// LLVBOPool class
// GL name pools for vertex buffers
///////////////////////////////////////////////////////////////////////////////

constexpr U32 POOL_SIZE = 4096;

class LLVBOPool
{
protected:
	LOG_CLASS(LLVBOPool);

public:
	LL_INLINE LLVBOPool()
	:	mAllocated(0),
		mReserved(0),
		mRequested(0),
		mBufferCount(0),
		mTotalHits(0),
		mAllocCount(0),
		mMissCount(0),
		mSkipped(0)
	{
	}

	LL_INLINE ~LLVBOPool()
	{
		logStats();
		clear();
		if (sNameIdx)
		{
			LLGLManager::deleteBuffers(sNameIdx, sNamePool);
			sNameIdx = 0;
		}
	}

	LL_INLINE U32 adjustSize(U32 size)
	{
		U32 block_size = llmax(nhpo2(size) / 8, 16);
		return size + block_size - (size % block_size);
	}

	U8* allocate(S32 type, U32 size, U32& name);

	// Size MUST be the size provided to allocate that returned the given name
	void free(S32 type, U32 size, U32 name, U8* data);

	void clean(bool force = false);

	void clear();

	U64 getVRAMMegabytes() const
	{
		return BYTES2MEGABYTES(mAllocated + mReserved);
	}

	void logStats();

private:
	U32 genBuffer();
	void deleteBuffer(U32 name);

private:
	struct Entry
	{
		LL_INLINE Entry(U8* data, U32 name, U32 current_frame)
		:	mData(data),
			mGLName(name),
			mFrameStamp(current_frame)
		{
		}

		U8*	mData;
		U32	mGLName;
		U32 mFrameStamp;
	};

	typedef std::list<Entry> entry_list_t;
	typedef fast_hmap<U32, entry_list_t > pool_map_t;
	pool_map_t					mVBOPool;
	pool_map_t					mIBOPool;

	S64							mAllocated;
	S64							mReserved;
	S64							mRequested;
	U32							mBufferCount;
	U32							mTotalHits;
	U32							mAllocCount;
	U32							mMissCount;
	U32							mSkipped;

	// Used to avoid calling glGenBuffers() for every VBO creation
	static U32					sNamePool[POOL_SIZE];
	static U32					sNameIdx;
};

static LLVBOPool* sVBOPool = NULL;

// Static members
U32 LLVBOPool::sNamePool[POOL_SIZE];
U32 LLVBOPool::sNameIdx = 0;

U32 LLVBOPool::genBuffer()
{
	if (!sNameIdx)
	{
		if (LLGLManager::sIsAMD)
		{
			// Workaround for AMD bug.
			for (U32 i = 0; i < POOL_SIZE; ++i)
			{
				glGenBuffers(1, sNamePool + i);
			}
		}
		else
		{
			glGenBuffers(POOL_SIZE, sNamePool);
		}
		sNameIdx = POOL_SIZE;
	}
	return sNamePool[--sNameIdx];
}

U8* LLVBOPool::allocate(S32 type, U32 size, U32& name)
{
	U8* ret = NULL;

	++mAllocCount;
	mRequested += size;
	size = adjustSize(size);
	mAllocated += size;

	auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

	pool_map_t::iterator iter = pool.find(size);
	if (iter != pool.end())
	{
		++mTotalHits;
		mReserved -= size;
		if (mReserved < 0)
		{
			llwarns << "Reserved buffers accounting mismatch: "
					<< mReserved << ". Zeroed." << llendl;
			mReserved = 0;
		}
		// Found a free buffer
		entry_list_t& entries = iter->second;
		Entry& entry = entries.back();
		name = entry.mGLName;
		ret = entry.mData;
		// Remove this entry from the list
		entries.pop_back();
		if (entries.empty())
		{
			// Remove this list of empty entries
			pool.erase(iter);
		}
		return ret;
	}

	// Cache miss, allocate a new buffer
	++mMissCount;

	name = genBuffer();
	glBindBuffer(type, name);
	// Note: we now use the GL_DYNAMIC_DRAW hint everywhere: I did test (with
	// a key = usage << 32 + size for the cache) with usage hints preservation,
	// but it simply does not change anything at all to frame rates (my guess
	// is that modern GL drivers find the right usage and ignore the hint,
	// which most programmers get wrong anyway). HB
	glBufferData(type, size, NULL, GL_DYNAMIC_DRAW);
	ret = (U8*)ll_aligned_malloc_16(size);
	if (ret)
	{
		++mBufferCount;
		if (type == GL_ELEMENT_ARRAY_BUFFER)
		{
			LLVertexBuffer::sGLRenderIndices = name;
		}
		else
		{
			LLVertexBuffer::sGLRenderBuffer = name;
		}
	}
	else
	{
		LLMemory::allocationFailed();
		llwarns << "Memory allocation for Vertex Buffer. Do expect a crash soon..."
				<< llendl;
	}
	return ret;
}

void LLVBOPool::free(S32 type, U32 size, U32 name, U8* data)
{
	if (name == LLVertexBuffer::sGLRenderBuffer)
	{
		LLVertexBuffer::unbind();
	}

	mRequested -= size;
	if (mRequested < 0)
	{
		llwarns << "Requested buffers accounting mismatch: " << mRequested
				<< ". Zeroed." << llendl;
		mRequested = 0;
	}
	size = adjustSize(size);
	mAllocated -= size;
	if (mAllocated < 0)
	{
		llwarns << "Allocated buffers accounting mismatch: " << mAllocated
				<< ". Zeroed." << llendl;
		mAllocated = 0;
	}
	mReserved += size;

	auto& pool = type == GL_ELEMENT_ARRAY_BUFFER ? mIBOPool : mVBOPool;

	pool_map_t::iterator iter = pool.find(size);
	if (iter != pool.end())
	{
		// Re-add this freed pool to the existing list
		iter->second.emplace_front(data, name, LLRender::sCurrentFrame);
	}
	else
	{
		// Make a new list and add this entry to it.
		entry_list_t newlist;
		newlist.emplace_front(data, name, LLRender::sCurrentFrame);
		pool.emplace(size, std::move(newlist));
	}
}

void LLVBOPool::clean(bool force)
{
	if (!force && mMissCount < 1024 &&
		// Do not let the VBO cache grow and stay too large either... HB
		(mBufferCount < 5 * POOL_SIZE || mSkipped < 600))
	{
		++mSkipped;
		return;
	}
	mMissCount = mSkipped = 0;

	constexpr U32 MAX_FRAME_AGE = 120;
	U32 current_frame = LLRender::sCurrentFrame;

	std::vector<U32> pending_deletions;

	for (pool_map_t::iterator it = mIBOPool.begin(), end = mIBOPool.end();
		 it != end; )
	{
		auto& entries = it->second;
		while (!entries.empty())
		{
			auto& entry = entries.back();
			if (current_frame - entry.mFrameStamp < MAX_FRAME_AGE)
			{
				break;
			}
			ll_aligned_free_16(entry.mData);
			mReserved -= it->first;
			--mBufferCount;
			pending_deletions.push_back(entry.mGLName);
			entries.pop_back();
		}
		if (entries.empty())
		{
			it = mIBOPool.erase(it);
		}
		else
		{
			++it;
		}
	}

	for (pool_map_t::iterator it = mVBOPool.begin(), end = mVBOPool.end();
		 it != end; )
	{
		auto& entries = it->second;
		while (!entries.empty())
		{
			auto& entry = entries.back();
			if (current_frame - entry.mFrameStamp < MAX_FRAME_AGE)
			{
				break;
			}
			ll_aligned_free_16(entry.mData);
			mReserved -= it->first;
			--mBufferCount;
			pending_deletions.push_back(entry.mGLName);
			entries.pop_back();
		}
		if (entries.empty())
		{
			it = mVBOPool.erase(it);
		}
		else
		{
			++it;
		}
	}

	if (mReserved < 0)
	{
		llwarns << "Reserved buffers accounting mismatch: " << mReserved
				<< ". Zeroed." << llendl;
		mReserved = 0;
	}

	size_t pending = pending_deletions.size();
	if (pending)
	{
		LLGLManager::deleteBuffers(pending, pending_deletions.data());
		// Only log stats when the debug tag is enabled. HB
		bool log_stats = false;
		LL_DEBUGS("VertexBuffer") << "Erased " << pending;
		log_stats = true;
		LL_CONT << " expired buffers." << LL_ENDL;
		if (log_stats)
		{
			logStats();
		}
	}
}

void LLVBOPool::clear()
{
	std::vector<U32> pending_deletions;
	pending_deletions.reserve(mIBOPool.size() + mVBOPool.size());

	for (auto& entries : mIBOPool)
	{
		for (auto& entry : entries.second)
		{
			ll_aligned_free_16(entry.mData);
			--mBufferCount;
			pending_deletions.push_back(entry.mGLName);
		}
	}

	for (auto& entries : mVBOPool)
	{
		for (auto& entry : entries.second)
		{
			ll_aligned_free_16(entry.mData);
			--mBufferCount;
			pending_deletions.push_back(entry.mGLName);
		}
	}

	size_t pending = pending_deletions.size();
	if (pending)
	{
		LLGLManager::deleteBuffers(pending, pending_deletions.data());
	}

	mIBOPool.clear();
	mVBOPool.clear();

	mReserved = 0;
}

void LLVBOPool::logStats()
{
	if (!mRequested || !mAllocCount)
	{
		return;
	}
	llinfos << "VBO pool stats: " << mBufferCount << " total buffers, "
			<< BYTES2MEGABYTES(mRequested) << "MB in use, "
			<< BYTES2MEGABYTES(mAllocated) << "MB allocated (overhead: "
			<< 0.1f * ((mAllocated - mRequested) * 1000 / mRequested) << "%), "
			<< BYTES2MEGABYTES(mReserved) << "MB available in cache, "
			<< BYTES2MEGABYTES(mAllocated + mReserved)
			<< "MB total in VRAM. Cache hit rate: "
			<< 0.1f * (mTotalHits * 1000 / mAllocCount) << "%" << LL_ENDL;
}

///////////////////////////////////////////////////////////////////////////////
// LLVertexBuffer class
///////////////////////////////////////////////////////////////////////////////

// Static variables instantiation.

#if LL_DEBUG_VB_ALLOC
LLVertexBuffer::instances_set_t LLVertexBuffer::sInstances;
#endif

std::vector<LLVertexBuffer*> LLVertexBuffer::sMappedBuffers;
LLPointer<LLVertexBuffer> LLVertexBuffer::sUtilityBuffer;

U32 LLVertexBuffer::sBindCount = 0;
U32 LLVertexBuffer::sSetCount = 0;
S32 LLVertexBuffer::sGLCount = 0;
U32 LLVertexBuffer::sGLRenderBuffer = 0;
U32 LLVertexBuffer::sGLRenderIndices = 0;
U32 LLVertexBuffer::sLastMask = 0;
U32 LLVertexBuffer::sVertexCount = 0;
U32 LLVertexBuffer::sIndexCount = 0;

bool LLVertexBuffer::sVBOActive = false;
bool LLVertexBuffer::sIBOActive = false;

const U32 LLVertexBuffer::sTypeSize[LLVertexBuffer::TYPE_MAX] =
{
	sizeof(LLVector4), // TYPE_VERTEX,
	sizeof(LLVector4), // TYPE_NORMAL,
	sizeof(LLVector2), // TYPE_TEXCOORD0,
	sizeof(LLVector2), // TYPE_TEXCOORD1,
	sizeof(LLVector2), // TYPE_TEXCOORD2,
	sizeof(LLVector2), // TYPE_TEXCOORD3,
	sizeof(LLColor4U), // TYPE_COLOR,
	sizeof(LLColor4U), // TYPE_EMISSIVE, only alpha is used currently
	sizeof(LLVector4), // TYPE_TANGENT,
	sizeof(F32),	   // TYPE_WEIGHT,
	sizeof(LLVector4), // TYPE_WEIGHT4,
	sizeof(LLVector4), // TYPE_CLOTHWEIGHT,
	sizeof(U64),	   // TYPE_JOINT,
	// Actually exists as position.w, no extra data, but stride is 16 bytes
	sizeof(LLVector4), // TYPE_TEXTURE_INDEX
};

static const std::string vb_type_name[] =
{
	"TYPE_VERTEX",
	"TYPE_NORMAL",
	"TYPE_TEXCOORD0",
	"TYPE_TEXCOORD1",
	"TYPE_TEXCOORD2",
	"TYPE_TEXCOORD3",
	"TYPE_COLOR",
	"TYPE_EMISSIVE",
	"TYPE_TANGENT",
	"TYPE_WEIGHT",
	"TYPE_WEIGHT4",
	"TYPE_CLOTHWEIGHT",
	"TYPE_JOINT",
	"TYPE_TEXTURE_INDEX",
	"TYPE_MAX",
	"TYPE_INDEX",
};

const U32 LLVertexBuffer::sGLMode[LLRender::NUM_MODES] =
{
	GL_TRIANGLES,
	GL_TRIANGLE_STRIP,
	GL_TRIANGLE_FAN,
	GL_POINTS,
	GL_LINES,
	GL_LINE_STRIP,
	GL_LINE_LOOP,
};

//static
void LLVertexBuffer::initClass()
{
	if (!sVBOPool)
	{
		sVBOPool = new LLVBOPool();
	}

	if (gUsePBRShaders)
	{
		// Do not allocate the utility buffer for PBR rendering. This would
		// break draw calls using it. *TODO: repair it for PBR. HB
		sUtilityBuffer = NULL;
		return;
	}

	sUtilityBuffer = new LLVertexBuffer(MAP_VERTEX | MAP_NORMAL |
										MAP_TEXCOORD0);
#if LL_DEBUG_VB_ALLOC
	sUtilityBuffer->setOwner("Utility buffer");
#endif
	if (!sUtilityBuffer->allocateBuffer(65536, 65536))
	{
		sUtilityBuffer = NULL;
		llwarns << "Failed to allocate the utility buffer" << llendl;
	}
}

//static
S32 LLVertexBuffer::getVRAMMegabytes()
{
	return sVBOPool ? sVBOPool->getVRAMMegabytes() : 0;
}

//static
void LLVertexBuffer::cleanupVBOPool()
{
	if (sVBOPool)
	{
		sVBOPool->clean();
	}
}

//static
void LLVertexBuffer::cleanupClass()
{
	unbind();
	sLastMask = 0;

	sUtilityBuffer = NULL;

	if (sVBOPool)
	{
		// Note: do *not* destroy the existing VBO pool unless we are exiting;
		// this would cause VB memory accounting mismatches. HB
		if (LLApp::isExiting())
		{
			delete sVBOPool;
			sVBOPool = NULL;
		}
		else
		{
			sVBOPool->clear();
		}
	}
}

LLVertexBuffer::LLVertexBuffer(U32 typemask)
:	mNumVerts(0),
	mNumIndices(0),
	mIndicesType(GL_UNSIGNED_SHORT),
	mIndicesStride(sizeof(U16)),
	mSize(0),
	mIndicesSize(0),
	mTypeMask(typemask),
	mTypeMaskMask(0),
	mGLBuffer(0),
	mGLIndices(0),
	mMappedData(NULL),
	mMappedIndexData(NULL),
	mMapped(false),
	mCachedBuffer(false)
{
	// Zero out offsets
	for (U32 i = 0; i < TYPE_MAX; ++i)
	{
		mOffsets[i] = 0;
	}
#if LL_DEBUG_VB_ALLOC
	sInstances.insert(this);
#endif
}

// Protected, use unref()
//virtual
LLVertexBuffer::~LLVertexBuffer()
{
	if (mMapped)
	{
		// Is on the mapped buffer list but does not need to be flushed
		mMapped = false;
		unmapBuffer();
	}

	destroyGLBuffer();
	destroyGLIndices();

	sVertexCount -= mNumVerts;
	sIndexCount -= mNumIndices;
#if LL_DEBUG_VB_ALLOC
	sInstances.erase(this);
#endif

	if (gDebugGL)
	{
		if (mMappedData)
		{
			llerrs << "Failed to clear vertex buffer vertices" << llendl;
		}
		if (mMappedIndexData)
		{
			llerrs << "Failed to clear vertex buffer indices" << llendl;
		}
	}
}

#if LL_DEBUG_VB_ALLOC
//static
void LLVertexBuffer::dumpInstances()
{
	if (sInstances.empty())
	{
		return;
	}

	llinfos << "Allocated buffers:";
	for (instances_set_t::const_iterator it = sInstances.begin(),
										 end = sInstances.end();
		 it != end; ++it)
	{
		const LLVertexBuffer* vb = *it;
		llcont << "\n - 0x" << std::hex << intptr_t(vb) << std::dec << ": "
			   << vb->mOwner;
	}
	llcont << llendl;
}
#endif

//static
void LLVertexBuffer::setupClientArrays(U32 data_mask)
{
	if (sLastMask != data_mask)
	{
		if (!LLGLManager::sHasVertexAttribIPointer)
		{
			// Make sure texture index is disabled
			data_mask = data_mask & ~MAP_TEXTURE_INDEX;
		}

		for (U32 i = 0; i < TYPE_MAX; ++i)
		{
			U32 mask = 1 << i;
			if (sLastMask & mask)
			{
				// Was enabled
				if (!(data_mask & mask))
				{
					// Needs to be disabled
					glDisableVertexAttribArray((GLint)i);
				}
			}
			else if (data_mask & mask)
			{
				// Was disabled and needs to be enabled
				glEnableVertexAttribArray((GLint)i);
			}
		}

		sLastMask = data_mask;
	}
}

// LL's new (fixed) but slow code, and without normals support.
//static
void LLVertexBuffer::drawArrays(U32 mode, const std::vector<LLVector3>& pos)
{
	gGL.begin(mode);
	for (U32 i = 0, count = pos.size(); i < count; ++i)
	{
		gGL.vertex3fv(pos[i].mV);
	}
	gGL.end(true);
}

//static
void LLVertexBuffer::drawArrays(U32 mode, const std::vector<LLVector3>& pos,
								const std::vector<LLVector3>& norm)
{
	U32 count = pos.size();
	if (count == 0)
	{
		return;
	}
	if (count <= 65536 && sUtilityBuffer.notNull())
	{
		gGL.syncMatrices();

		if (norm.size() < count)
		{
			llwarns_once << "Less normals (" << norm.size()
						 << ") than vertices (" << count
						 << "), aborting." << llendl;
			return;
		}
		// Vertex-buffer based, optimized code
		LLStrider<LLVector3> vertex_strider;
		LLStrider<LLVector3> normal_strider;
		if (!sUtilityBuffer->getVertexStrider(vertex_strider) ||
			!sUtilityBuffer->getNormalStrider(normal_strider))
		{
			llwarns_sparse << "Failed to get striders, aborting." << llendl;
			return;
		}
		for (U32 i = 0; i < count; ++i)
		{
			*(vertex_strider++) = pos[i];
			*(normal_strider++) = norm[i];
		}
		sUtilityBuffer->setBuffer(MAP_VERTEX | MAP_NORMAL);
		sUtilityBuffer->drawArrays(mode, 0, pos.size());
	}
	else
	{
		// Fallback to LL's new (fixed) but slow code, and without normals
		// support
		drawArrays(mode, pos);
	}
}

//static
void LLVertexBuffer::drawElements(U32 num_vertices, const LLVector4a* posp,
								  const LLVector2* tcp, U32 num_indices,
								  const U16* indicesp)
{
	if (!posp || !indicesp || num_vertices <= 0 || num_indices <= 0)
	{
		llwarns << (posp ? "" : "NULL positions pointer - ")
				<< (indicesp ? "" : "NULL indices pointer - ")
				<< num_vertices << " vertices - " << num_indices
				<< " indices. Aborting." << llendl;
		return;
	}

	gGL.syncMatrices();

	if (num_vertices <= 65536 && num_indices <= 65536 &&
		sUtilityBuffer.notNull())
	{
		// Vertex-buffer based, optimized code
		LLStrider<LLVector4a> vertex_strider;
		LLStrider<U16> index_strider;
		if (!sUtilityBuffer->getVertexStrider(vertex_strider) ||
			!sUtilityBuffer->getIndexStrider(index_strider))
		{
			llwarns_sparse << "Failed to get striders, aborting." << llendl;
			return;
		}
		U32 index_size = ((num_indices * sizeof(U16)) + 0xF) & ~0xF;
		LLVector4a::memcpyNonAliased16((F32*)index_strider.get(),
									   (F32*)indicesp, index_size);

		U32 vertex_size = ((num_vertices * 4 * sizeof(F32)) + 0xF) & ~0xF;
		LLVector4a::memcpyNonAliased16((F32*)vertex_strider.get(), (F32*)posp,
									   vertex_size);

		U32 mask = LLVertexBuffer::MAP_VERTEX;
		if (tcp)
		{
			mask |= LLVertexBuffer::MAP_TEXCOORD0;
			LLStrider<LLVector2> tc_strider;
			if (!sUtilityBuffer->getTexCoord0Strider(tc_strider))
			{
				llwarns_sparse << "Failed to get coord strider, aborting."
							   << llendl;
				return;
			}
			U32 tc_size = ((num_vertices * 2 * sizeof(F32)) + 0xF) & ~0xF;
			LLVector4a::memcpyNonAliased16((F32*)tc_strider.get(), (F32*)tcp,
										   tc_size);
		}

		sUtilityBuffer->setBuffer(mask);
		sUtilityBuffer->draw(LLRender::TRIANGLES, num_indices, 0);
	}
	else	// LL's new but slow code
	{
		unbind();

		gGL.begin(LLRender::TRIANGLES);

		if (tcp)
		{
			for (U32 i = 0; i < num_indices; ++i)
			{
				U16 idx = indicesp[i];
				gGL.texCoord2fv(tcp[idx].mV);
				gGL.vertex3fv(posp[idx].getF32ptr());
			}
		}
		else
		{
			for (U32 i = 0; i < num_indices; ++i)
			{
				U16 idx = indicesp[i];
				gGL.vertex3fv(posp[idx].getF32ptr());
			}
		}

		gGL.end(true);
	}
}

bool LLVertexBuffer::validateRange(U32 start, U32 end, U32 count,
								   U32 indices_offset) const
{
	if (start >= mNumVerts || end >= mNumVerts)
	{
		llwarns << "Bad vertex buffer draw range: [" << start << ", " << end
				<< "] vs " << mNumVerts << llendl;
		return false;
	}

	if (indices_offset >= mNumIndices || indices_offset + count > mNumIndices)
	{
		llwarns << "Bad index buffer draw range: [" << indices_offset << ", "
				<< indices_offset + count << "] vs " << mNumIndices << llendl;
		return false;
	}

#if 0	// Not reliable a test for VBOs that are not backed by a CPU buffer
	if (gUsePBRShaders && gDebugGL)
	{
		U16* idx = (U16*)mMappedIndexData + indices_offset;
		for (U32 i = 0; i < count; ++i)
		{
			if (idx[i] < start || idx[i] > end)
			{
				llwarns << "Index out of range:" << idx[i] << " not in ["
						<< start << ", " << end << "]" << llendl;
				return false;
			}
		}

		LLVector4a* v = (LLVector4a*)mMappedData;
		for (U32 i = start; i <= end; ++i)
		{
			if (!v[i].isFinite3())
			{
				llwarns << "Non-finite vertex position data detected."
						<< llendl;
				return false;
			}
		}

		LLGLSLShader* shaderp = LLGLSLShader::sCurBoundShaderPtr;
		if (shaderp && shaderp->mFeatures.mIndexedTextureChannels > 1)
		{
			LLVector4a* v = (LLVector4a*)mMappedData;
			for (U32 i = start; i < end; ++i)
			{
				U32 idx = U32(v[i][3] + 0.25f);
				if (idx >= (U32)shaderp->mFeatures.mIndexedTextureChannels)
				{
					llwarns << "Bad texture index (" << idx
							<< ") found for shader: " << shaderp->mName
							<< ". Max index should be "
							<< shaderp->mFeatures.mIndexedTextureChannels - 1
							<< "." << llendl;
					return false;
				}
			}
		}
	}
#endif

	return true;
}

void LLVertexBuffer::drawRange(U32 mode, U32 start, U32 end, U32 count,
							   U32 indices_offset) const
{
	gGL.syncMatrices();

	if (gDebugGL && !gUsePBRShaders)
	{
		if (!LLGLSLShader::sCurBoundShaderPtr)
		{
			llwarns << "No bound shader." << llendl;
			llassert(false);
		}
		if (mGLIndices != sGLRenderIndices)
		{
			llwarns << "Wrong index buffer bound." << llendl;
			llassert(false);
		}
		if (mGLBuffer != sGLRenderBuffer)
		{
			llwarns << "Wrong vertex buffer bound." << llendl;
			llassert(false);
		}
		if (!validateRange(start, end, count, indices_offset))
		{
			llwarns << "Check failed." << llendl;
			llassert(false);
		}
		if (!gUsePBRShaders)
		{
			GLint elem = 0;
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &elem);
			if ((U32)elem != mGLIndices)
			{
				llwarns << "Wrong index buffer bound." << llendl;
				llassert(false);
			}
		}
	}

	LLGLSLShader::startProfile();
	glDrawRangeElements(sGLMode[mode], start, end, count, mIndicesType,
						(const void*)(indices_offset * mIndicesStride));
	LLGLSLShader::stopProfile();
}

void LLVertexBuffer::drawRangeFast(U32 start, U32 end, U32 count,
								   U32 indices_offset) const
{
	gGL.syncMatrices();
	glDrawRangeElements(sGLMode[LLRender::TRIANGLES], start, end, count,
						mIndicesType,
						(const void*)(indices_offset * mIndicesStride));
}

void LLVertexBuffer::drawRangeFast(U32 mode, U32 start, U32 end, U32 count,
								   U32 indices_offset) const
{
	glDrawRangeElements(sGLMode[mode], start, end, count, mIndicesType,
						(const void*)(indices_offset * mIndicesStride));
}

void LLVertexBuffer::draw(U32 mode, U32 count, U32 indices_offset) const
{
	drawRange(mode, 0, mNumVerts - 1, count, indices_offset);
}

void LLVertexBuffer::drawArrays(U32 mode, U32 first, U32 count) const
{
	gGL.syncMatrices();

	if (gDebugGL && !gUsePBRShaders)
	{
		if (!LLGLSLShader::sCurBoundShaderPtr)
		{
			llwarns << "No bound shader" << llendl;
			llassert(false);
		}
		if (first >= mNumVerts || first + count > mNumVerts)
		{
			llwarns << "Bad vertex buffer draw range: [" << first << ", "
					<< first + count << "] vs " << mNumVerts << ". Aborted."
					<< llendl;
			llassert(false);
		}
		if (mGLBuffer != sGLRenderBuffer || !sVBOActive)
		{
			llwarns << "Wrong vertex buffer bound." << llendl;
			llassert(false);
		}
	}

	LLGLSLShader::startProfile();
	glDrawArrays(sGLMode[mode], first, count);
	LLGLSLShader::stopProfile();
}

//static
void LLVertexBuffer::unbind()
{
	if (sVBOActive)
	{
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		sVBOActive = false;
	}
	if (sIBOActive)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		sIBOActive = false;
	}

	sGLRenderBuffer = sGLRenderIndices = 0;

	if (!gUsePBRShaders)
	{
		setupClientArrays(0);
	}
}

//static
U32 LLVertexBuffer::calcOffsets(U32 typemask, U32* offsets, U32 num_vertices)
{
	U32 offset = 0;
	for (U32 i = 0; i < TYPE_TEXTURE_INDEX; ++i)
	{
		U32 mask = 1 << i;
		if (typemask & mask)
		{
			if (offsets && sTypeSize[i])
			{
				offsets[i] = offset;
				offset += sTypeSize[i] * num_vertices;
				offset = (offset + 0xF) & ~0xF;
			}
		}
	}

	offsets[TYPE_TEXTURE_INDEX] = offsets[TYPE_VERTEX] + 12;

	return offset;
}

//static
U32 LLVertexBuffer::calcVertexSize(U32 typemask)
{
	U32 size = 0;
	for (U32 i = 0; i < TYPE_TEXTURE_INDEX; ++i)
	{
		U32 mask = 1 << i;
		if (typemask & mask)
		{
			size += sTypeSize[i];
		}
	}

	return size;
}

void LLVertexBuffer::genBuffer(U32 size)
{
	if (sVBOPool)
	{
		mSize = size;
		mMappedData = sVBOPool->allocate(GL_ARRAY_BUFFER, size, mGLBuffer);
		++sGLCount;
	}
}

void LLVertexBuffer::genIndices(U32 size)
{
	if (sVBOPool)
	{
		mIndicesSize = size;
		mMappedIndexData = sVBOPool->allocate(GL_ELEMENT_ARRAY_BUFFER, size,
											  mGLIndices);
		++sGLCount;
	}
}

bool LLVertexBuffer::createGLBuffer(U32 size)
{
	if (mGLBuffer || mMappedData)
	{
		destroyGLBuffer();
	}
	if (size == 0)
	{
		return true;
	}

	genBuffer(size);
	return mMappedData != NULL;
}

bool LLVertexBuffer::createGLIndices(U32 size)
{
	if (mGLIndices)
	{
		destroyGLIndices();
	}
	if (size == 0)
	{
		return true;
	}

	genIndices(size);
	return mMappedIndexData != NULL;
}

void LLVertexBuffer::destroyGLBuffer()
{
	if (mGLBuffer || mMappedData)
	{
		if (sVBOPool)
		{
			sVBOPool->free(GL_ARRAY_BUFFER, mSize, mGLBuffer, mMappedData);
		}

		mSize = 0;
		mGLBuffer = 0;
		mMappedData = NULL;
		--sGLCount;
	}
}

void LLVertexBuffer::destroyGLIndices()
{
	if (mGLIndices || mMappedIndexData)
	{
		if (sVBOPool)
		{
			sVBOPool->free(GL_ELEMENT_ARRAY_BUFFER, mIndicesSize, mGLIndices,
						   mMappedIndexData);
		}
		mIndicesSize = 0;
		mGLIndices = 0;
		mMappedIndexData = NULL;
		--sGLCount;
	}
}

bool LLVertexBuffer::updateNumVerts(U32 nverts)
{
	bool success = true;

	if (nverts > (U32)S32_MAX)
	{
		// We store these buffer sizes as S32 elsewhere
		llwarns << "Too many vertices: " << nverts << llendl;
		return false;
	}

	U32 needed_size = calcOffsets(mTypeMask, mOffsets, nverts);
	if (needed_size != mSize)
	{
		success = createGLBuffer(needed_size);
	}

	sVertexCount -= mNumVerts;
	mNumVerts = nverts;
	sVertexCount += mNumVerts;

	return success;
}

bool LLVertexBuffer::updateNumIndices(U32 nindices)
{
	bool success = true;

	U32 needed_size = sizeof(U16) * nindices;
	if (needed_size != mIndicesSize)
	{
		success = createGLIndices(needed_size);
	}

	sIndexCount -= mNumIndices;
	mNumIndices = nindices;
	sIndexCount += mNumIndices;

	return success;
}

bool LLVertexBuffer::allocateBuffer(U32 nverts, U32 nindices)
{
	if (nverts > (U32)S32_MAX)
	{
		// We store these buffer sizes as S32 elsewhere
		llwarns << "Too many vertices: " << nverts << llendl;
		return false;
	}

	bool success = updateNumVerts(nverts);
	success &= updateNumIndices(nindices);

	if (success && !gUsePBRShaders && (nverts || nindices))
	{
		doUnmapBuffer();
	}

	return success;
}

static bool expand_region(LLVertexBuffer::MappedRegion& region, U32 start,
						  U32 end)
{
	if (end < region.mStart || start > region.mEnd)
	{
		// There is a gap, do not merge
		return false;
	}

	region.mStart = llmin(region.mStart, start);
	region.mEnd = llmax(region.mEnd, end);
	return true;
}

void LLVertexBuffer::mapBuffer()
{
	if (!mMapped)
	{
		mMapped = true;
		sMappedBuffers.push_back(this);
	}
}

// Map for data access
U8* LLVertexBuffer::mapVertexBuffer(S32 type, U32 index, S32 count)
{
	mapBuffer();

	if (!mCachedBuffer && !gUsePBRShaders)
	{
		bindGLBuffer(true);
	}

	if (count == -1)
	{
		count = mNumVerts - index;
	}

	U32 start = mOffsets[type] + sTypeSize[type] * index;
	U32 end = start + sTypeSize[type] * count - 1;

	bool mapped = false;
	// Flag region as mapped
	for (U32 i = 0, count = mMappedVertexRegions.size(); i < count; ++i)
	{
		MappedRegion& region = mMappedVertexRegions[i];
		if (expand_region(region, start, end))
		{
			mapped = true;
			break;
		}
	}

	if (!mapped)
	{
		// Not already mapped, map new region
		mMappedVertexRegions.emplace_back(start, end);
	}

	return mMappedData + mOffsets[type] + sTypeSize[type] * index;
}

U8* LLVertexBuffer::mapIndexBuffer(U32 index, S32 count)
{
	mapBuffer();

	bindGLIndices(!mCachedBuffer);

	if (count == -1)
	{
		count = mNumIndices - index;
	}

	U32 start = sizeof(U16) * index;
	U32 end = start + sizeof(U16) * count - 1;

	bool mapped = false;
	// See if range is already mapped
	for (U32 i = 0; i < mMappedIndexRegions.size(); ++i)
	{
		MappedRegion& region = mMappedIndexRegions[i];
		if (expand_region(region, start, end))
		{
			mapped = true;
			break;
		}
	}

	if (!mapped)
	{
		// Not already mapped, map new region
		mMappedIndexRegions.emplace_back(start, end);
	}

	return mMappedIndexData + sizeof(U16) * index;
}

struct SortMappedRegion
{
	LL_INLINE bool operator()(const LLVertexBuffer::MappedRegion& lhs,
							  const LLVertexBuffer::MappedRegion& rhs)
	{
		return lhs.mStart < rhs.mStart;
	}
};

void LLVertexBuffer::doUnmapBuffer()
{
	if (!mMapped)
	{
		return;
	}

	if (mMappedData && !mMappedVertexRegions.empty())
	{
		LL_TRACY_TIMER(TRC_VBO_UNMAP);

		if (mGLBuffer != sGLRenderBuffer)
		{
			glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
			sGLRenderBuffer = mGLBuffer;
		}

		U32 start = 0;
		U32 end = 0;

        std::sort(mMappedVertexRegions.begin(), mMappedVertexRegions.end(),
				  SortMappedRegion());

		for (U32 i = 0, count = mMappedVertexRegions.size(); i < count; ++i)
		{
			const MappedRegion& region = mMappedVertexRegions[i];
			if (region.mStart == end + 1)
			{
				end = region.mEnd;
			}
			else
			{
				flush_vbo(GL_ARRAY_BUFFER, start, end,
						  (U8*)mMappedData + start);
				start = region.mStart;
				end = region.mEnd;
			}
		}
		flush_vbo(GL_ARRAY_BUFFER, start, end, (U8*)mMappedData + start);
		stop_glerror();
	}
	mMappedVertexRegions.clear();

	if (mMappedIndexData && !mMappedIndexRegions.empty())
	{
		LL_TRACY_TIMER(TRC_IBO_UNMAP);

		if (mGLIndices != sGLRenderIndices)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
			sGLRenderIndices = mGLIndices;
		}

		U32 start = 0;
		U32 end = 0;

		std::sort(mMappedIndexRegions.begin(), mMappedIndexRegions.end(),
				  SortMappedRegion());

		for (U32 i = 0, count = mMappedIndexRegions.size(); i < count; ++i)
		{
			const MappedRegion& region = mMappedIndexRegions[i];
			if (region.mStart == end + 1)
			{
				end = region.mEnd;
			}
			else
			{
				flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end,
						  (U8*)mMappedIndexData + start);
				start = region.mStart;
				end = region.mEnd;
			}
		}
		flush_vbo(GL_ELEMENT_ARRAY_BUFFER, start, end,
				  (U8*)mMappedIndexData + start);
		stop_glerror();
	}
	mMappedIndexRegions.clear();
}

// Note: must only be called from main thread
//static
void LLVertexBuffer::flushBuffers()
{
	LL_TRACY_TIMER(TRC_FLUSH_GL_BUFFERS);
	for (U32 i = 0, count = sMappedBuffers.size(); i < count; ++i)
	{
		LLVertexBuffer* buffp = sMappedBuffers[i];
		buffp->doUnmapBuffer();
		buffp->mMapped = false;
	}
	sMappedBuffers.clear();
}

void LLVertexBuffer::resetVertexData()
{
	if (mMappedData && mSize)
	{
		memset((void*)mMappedData, 0, mSize);
	}
}

void LLVertexBuffer::resetIndexData()
{
	if (mMappedIndexData && mIndicesSize)
	{
		memset((void*)mMappedIndexData, 0, mIndicesSize);
	}
}

template <class T, S32 type>
class VertexBufferStrider
{
protected:
	LOG_CLASS(VertexBufferStrider);

public:
	typedef LLStrider<T> strider_t;
	static bool get(LLVertexBuffer& vbo, strider_t& strider, U32 index,
					S32 count)
	{
		if (type == LLVertexBuffer::TYPE_INDEX)
		{
			U8* ptr = vbo.mapIndexBuffer(index, count);
			strider = (T*)ptr;
			if (!ptr)
			{
				llwarns << "mapIndexBuffer() failed !" << llendl;
				return false;
			}
			strider.setStride(0);
			return true;
		}
		else if (vbo.hasDataType(type))
		{
			U8* ptr = vbo.mapVertexBuffer(type, index, count);
			strider = (T*)ptr;
			if (!ptr)
			{
				llwarns << "mapVertexBuffer() failed !" << llendl;
				return false;
			}
			strider.setStride(LLVertexBuffer::sTypeSize[type]);
			return true;
		}

		llwarns << "Could not find valid vertex data." << llendl;
		return false;
	}
};

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector3>& strider,
									  U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3, TYPE_VERTEX>::get(*this, strider,
															 index, count);
}

bool LLVertexBuffer::getVertexStrider(LLStrider<LLVector4a>& strider,
									  U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a, TYPE_VERTEX>::get(*this, strider,
															 index, count);
}

bool LLVertexBuffer::getIndexStrider(LLStrider<U16>& strider,
									 U32 index, S32 count)
{
	if (mIndicesType != GL_UNSIGNED_SHORT)
	{
		llassert(false);
		return false; // Cannot access 32 bits indices with U16 strider...
	}
	return VertexBufferStrider<U16, TYPE_INDEX>::get(*this, strider, index,
													 count);
}

bool LLVertexBuffer::getTexCoord0Strider(LLStrider<LLVector2>& strider,
										 U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2, TYPE_TEXCOORD0>::get(*this, strider,
															   index, count);
}

bool LLVertexBuffer::getTexCoord1Strider(LLStrider<LLVector2>& strider,
										 U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2, TYPE_TEXCOORD1>::get(*this, strider,
															   index, count);
}

bool LLVertexBuffer::getTexCoord2Strider(LLStrider<LLVector2>& strider,
										 U32 index, S32 count)
{
	return VertexBufferStrider<LLVector2, TYPE_TEXCOORD2>::get(*this, strider,
															   index, count);
}

bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector3>& strider,
									  U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3, TYPE_NORMAL>::get(*this, strider,
															index, count);
}

bool LLVertexBuffer::getNormalStrider(LLStrider<LLVector4a>& strider,
									  U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a, TYPE_NORMAL>::get(*this, strider,
															 index, count);
}

bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector3>& strider,
									   U32 index, S32 count)
{
	return VertexBufferStrider<LLVector3, TYPE_TANGENT>::get(*this, strider,
															 index, count);
}

bool LLVertexBuffer::getTangentStrider(LLStrider<LLVector4a>& strider,
									   U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a, TYPE_TANGENT>::get(*this, strider,
															  index, count);
}

bool LLVertexBuffer::getColorStrider(LLStrider<LLColor4U>& strider,
									 U32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U, TYPE_COLOR>::get(*this, strider,
														   index, count);
}

bool LLVertexBuffer::getEmissiveStrider(LLStrider<LLColor4U>& strider,
										U32 index, S32 count)
{
	return VertexBufferStrider<LLColor4U, TYPE_EMISSIVE>::get(*this, strider,
															  index, count);
}

bool LLVertexBuffer::getWeightStrider(LLStrider<F32>& strider,
									  U32 index, S32 count)
{
	return VertexBufferStrider<F32, TYPE_WEIGHT>::get(*this, strider, index,
													  count);
}

bool LLVertexBuffer::getWeight4Strider(LLStrider<LLVector4a>& strider,
									   U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a, TYPE_WEIGHT4>::get(*this, strider,
															  index, count);
}

bool LLVertexBuffer::getClothWeightStrider(LLStrider<LLVector4a>& strider,
										   U32 index, S32 count)
{
	return VertexBufferStrider<LLVector4a, TYPE_CLOTHWEIGHT>::get(*this,
																  strider,
																  index,
																  count);
}

bool LLVertexBuffer::bindGLBuffer(bool force_bind)
{
	if (mGLBuffer &&
		(force_bind || (mGLBuffer != sGLRenderBuffer || !sVBOActive)))
	{
		LL_TRACY_TIMER(TRC_BIND_GL_BUFFER);

		glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
		sGLRenderBuffer = mGLBuffer;
		++sBindCount;
		sVBOActive = true;
		return true;
	}
	return false;
}

bool LLVertexBuffer::bindGLBufferFast()
{
	if (mGLBuffer != sGLRenderBuffer || !sVBOActive)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
		sGLRenderBuffer = mGLBuffer;
		++sBindCount;
		sVBOActive = true;
		return true;
	}
	return false;
}

bool LLVertexBuffer::bindGLIndices(bool force_bind)
{
	if (mGLIndices &&
		(force_bind || (mGLIndices != sGLRenderIndices || !sIBOActive)))
	{
		LL_TRACY_TIMER(TRC_BIND_GL_INDICES);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
		sGLRenderIndices = mGLIndices;
		stop_glerror();
		++sBindCount;
		sIBOActive = true;
		return true;
	}
	return false;
}

bool LLVertexBuffer::bindGLIndicesFast()
{
	if (mGLIndices != sGLRenderIndices || !sIBOActive)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
		sGLRenderIndices = mGLIndices;
		++sBindCount;
		sIBOActive = true;
		return true;
	}
	return false;
}

//static
std::string LLVertexBuffer::listMissingBits(U32 unsatisfied_mask)
{
	std::string report;
	if (unsatisfied_mask & MAP_VERTEX)
	{
		report = "\n - Missing vert pos";
	}
	if (unsatisfied_mask & MAP_NORMAL)
	{
		report += "\n - Missing normals";
	}
	if (unsatisfied_mask & MAP_TEXCOORD0)
	{
		report += "\n - Missing tex coord 0";
	}
	if (unsatisfied_mask & MAP_TEXCOORD1)
	{
		report += "\n - Missing tex coord 1";
	}
	if (unsatisfied_mask & MAP_TEXCOORD2)
	{
		report += "\n - Missing tex coord 2";
	}
	if (unsatisfied_mask & MAP_TEXCOORD3)
	{
		report += "\n - Missing tex coord 3";
	}
	if (unsatisfied_mask & MAP_COLOR)
	{
		report += "\n - Missing vert color";
	}
	if (unsatisfied_mask & MAP_EMISSIVE)
	{
		report += "\n - Missing emissive";
	}
	if (unsatisfied_mask & MAP_TANGENT)
	{
		report += "\n - Missing tangent";
	}
	if (unsatisfied_mask & MAP_WEIGHT)
	{
		report += "\n - Missing weight";
	}
	if (unsatisfied_mask & MAP_WEIGHT4)
	{
		report += "\n - Missing weight4";
	}
	if (unsatisfied_mask & MAP_CLOTHWEIGHT)
	{
		report += "\n - Missing cloth weight";
	}
	if (unsatisfied_mask & MAP_TEXTURE_INDEX)
	{
		report += "\n - Missing tex index";
	}
	if (unsatisfied_mask & TYPE_INDEX)
	{
		report += "\n - Missing indices";
	}
	return report;
}

// Set for rendering. For the legacy EE renderer only.
void LLVertexBuffer::setBuffer(U32 data_mask)
{
	// *HACK: in order to simplify the dual-renderer code and reduce the number
	// of tests in it...
	if (gUsePBRShaders)
	{
		setBuffer();
		return;
	}

	if (mMapped)
	{
		doUnmapBuffer();
	}

	// Set up pointers if the data mask is different ...
	bool setup = sLastMask != data_mask;

	if (data_mask && gDebugGL)
	{
		// Make sure data requirements are fulfilled
		LLGLSLShader* shader = LLGLSLShader::sCurBoundShaderPtr;
		if (shader)
		{
			U32 required_mask = 0;
			for (U32 i = 0; i < LLVertexBuffer::TYPE_TEXTURE_INDEX; ++i)
			{
				if (shader->getAttribLocation(i) > -1)
				{
					U32 required = 1 << i;
					if ((data_mask & required) == 0)
					{
						llwarns << "Missing attribute: "
								<< LLShaderMgr::sReservedAttribs[i] << llendl;
					}

					required_mask |= required;
				}
			}

			U32 unsatisfied_mask = required_mask & ~data_mask;
			if (unsatisfied_mask)
			{
				llwarns << "Shader consumption mismatches data provision:"
						<< listMissingBits(unsatisfied_mask) << llendl;
			}
		}
	}

	bool bind_buffer = mGLBuffer && bindGLBufferFast();
	bool bind_indices = mGLIndices && bindGLIndicesFast();
	setup |= bind_buffer || bind_indices;

	if (gDebugGL)
	{
		GLint buff;
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &buff);
		if ((U32)buff != mGLBuffer)
		{
			llwarns_once << "Invalid GL vertex buffer bound: " << buff
						 << " - Expected: " << mGLBuffer << llendl;
		}

		if (mGLIndices)
		{
			glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buff);
			if ((U32)buff != mGLIndices)
			{
				llerrs << "Invalid GL index buffer bound: " << buff << llendl;
			}
		}
	}

	setupClientArrays(data_mask);

	if (setup && data_mask && mGLBuffer)
	{
		setupVertexBuffer(data_mask);
	}
}

// Set fast for rendering. For the legacy EE renderer only.
void LLVertexBuffer::setBufferFast(U32 data_mask)
{
	// *HACK: in order to simplify the dual-renderer code and reduce the number
	// of tests in it...
	if (gUsePBRShaders)
	{
		setBuffer();
		return;
	}

	// Set up pointers if the data mask is different ...
	bool setup = sLastMask != data_mask;
	bool bind_buffer = bindGLBufferFast();
	bool bind_indices = bindGLIndicesFast();
	setup = setup || bind_buffer || bind_indices;
	setupClientArrays(data_mask);
	if (data_mask && setup)
	{
		setupVertexBuffer(data_mask);
	}
}

// New method used by the PBR renderer
void LLVertexBuffer::setBuffer()
{
	LLGLSLShader* shaderp = LLGLSLShader::sCurBoundShaderPtr;
	if (!shaderp)
	{
		// Issuing a simple warning and returning at this point would cause a
		// crash later on; so just crash now, in a "clean" way and with a
		// prominent error message (most likely, a shader failed to load). HB
		llerrs << "No bound shader !" << llendl;
	}

	U32 data_mask = shaderp->mAttributeMask;

	if (mMapped)
	{
		if (gDebugGL)
		{
			llwarns << "Data was pending on this buffer: 0x" << std::hex
					<< (intptr_t)this << std::dec << llendl;
		}
		doUnmapBuffer();
	}

	if (gDebugGL && (data_mask & mTypeMask) != data_mask)
	{
		llwarns << "Masks mismatch: shader mask = " << std::hex << data_mask
				<< " - VB mask = " << mTypeMask << std::dec << llendl;
	}

	if (sGLRenderBuffer != mGLBuffer)
	{
		glBindBuffer(GL_ARRAY_BUFFER, mGLBuffer);
		sGLRenderBuffer = mGLBuffer;
		setupVertexBuffer(data_mask);
	}
	else if (sLastMask != data_mask)
	{
		setupVertexBuffer(data_mask);
		sLastMask = data_mask;
	}

	if (mGLIndices != sGLRenderIndices)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mGLIndices);
		sGLRenderIndices = mGLIndices;
	}
}

// Only to be used for external (non rendering) purpose, such as with GLOD. HB
void LLVertexBuffer::setBufferNoShader(U32 data_mask)
{
	llassert_always(!LLGLSLShader::sCurBoundShaderPtr);

	doUnmapBuffer();

	bool setup = sLastMask != data_mask;
	bool bind_buffer = mGLBuffer && bindGLBufferFast();
	bool bind_indices = mGLIndices && bindGLIndicesFast();
	setup |= bind_buffer || bind_indices;
	setupClientArrays(data_mask);
	if (setup && data_mask && mGLBuffer)
	{
		setupVertexBuffer(data_mask);
	}
}

//virtual
void LLVertexBuffer::setupVertexBuffer(U32 data_mask)
{
	if (!gUsePBRShaders)
	{
		data_mask &= ~mTypeMaskMask;
	}

	if (gDebugGL && !gUsePBRShaders && (data_mask & mTypeMask) != data_mask)
	{
		for (U32 i = 0; i < LLVertexBuffer::TYPE_MAX; ++i)
		{
			U32 mask = 1 << i;
			if (mask & data_mask && !(mask & mTypeMask))
			{
				// Bit set in data_mask, but not set in mTypeMask
				llwarns << "Missing required component " << vb_type_name[i]
						<< llendl;
			}
		}
		llassert(false);
	}

	void* ptr;
	// NOTE: the 'loc' variable is *required* to pass as reference (passing
	// TYPE_* directly to glVertexAttribPointer() causes a crash), unlike
	// the OpenGL documentation prototype for this function... Go figure !  HB
	GLint loc;
	if (data_mask & MAP_NORMAL)
	{
		loc = TYPE_NORMAL;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_NORMAL]);
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(LLVector4),
							  ptr);
	}
	if (data_mask & MAP_TEXCOORD3)
	{
		loc = TYPE_TEXCOORD3;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_TEXCOORD3]);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(LLVector2),
							  ptr);
	}
	if (data_mask & MAP_TEXCOORD2)
	{
		loc = TYPE_TEXCOORD2;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_TEXCOORD2]);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(LLVector2),
							  ptr);
	}
	if (data_mask & MAP_TEXCOORD1)
	{
		loc = TYPE_TEXCOORD1;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_TEXCOORD1]);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(LLVector2),
							  ptr);
	}
	if (data_mask & MAP_TANGENT)
	{
		loc = TYPE_TANGENT;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_TANGENT]);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(LLVector4),
							  ptr);
	}
	if (data_mask & MAP_TEXCOORD0)
	{
		loc = TYPE_TEXCOORD0;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_TEXCOORD0]);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(LLVector2),
							  ptr);
	}
	if (data_mask & MAP_COLOR)
	{
		loc = TYPE_COLOR;
		// Bind emissive instead of color pointer if emissive is present
		if (data_mask & MAP_EMISSIVE)
		{
			ptr = reinterpret_cast<void*>(mOffsets[TYPE_EMISSIVE]);
		}
		else
		{
			ptr = reinterpret_cast<void*>(mOffsets[TYPE_COLOR]);
		}
		// Note: sTypeSize[TYPE_COLOR] == sTypeSize[TYPE_EMISSIVE]. HB
		glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE,
							  sizeof(LLColor4U), ptr);
	}
	if (data_mask & MAP_EMISSIVE)
	{
		loc = TYPE_EMISSIVE;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_EMISSIVE]);
		glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE,
							  sizeof(LLColor4U), ptr);
		if (!(data_mask & MAP_COLOR))
		{
			// Map emissive to color channel when color is not also being bound
			// to avoid unnecessary shader swaps
			loc = TYPE_COLOR;
			glVertexAttribPointer(loc, 4, GL_UNSIGNED_BYTE, GL_TRUE,
								  sizeof(LLColor4U), ptr);
		}
	}
	if (data_mask & MAP_WEIGHT)
	{
		loc = TYPE_WEIGHT;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_WEIGHT]);
		glVertexAttribPointer(loc, 1, GL_FLOAT, GL_FALSE, sizeof(F32), ptr);
	}
	if (data_mask & MAP_WEIGHT4)
	{
		loc = TYPE_WEIGHT4;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_WEIGHT4]);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(LLVector4),
							  ptr);
	}
	if (data_mask & MAP_JOINT)
	{
		loc = TYPE_JOINT;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_JOINT]);
		glVertexAttribIPointer(loc, 4, GL_UNSIGNED_SHORT, sizeof(U64), ptr);
	}
	if (data_mask & MAP_CLOTHWEIGHT)
	{
		loc = TYPE_CLOTHWEIGHT;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_CLOTHWEIGHT]);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_TRUE, sizeof(LLVector4),
							  ptr);
	}
	if (data_mask & MAP_TEXTURE_INDEX && LLGLManager::sHasVertexAttribIPointer)
	{
		loc = TYPE_TEXTURE_INDEX;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_VERTEX] + 12);
		glVertexAttribIPointer(loc, 1, GL_UNSIGNED_INT, sizeof(LLVector4),
							   ptr);
	}
	if (data_mask & MAP_VERTEX)
	{
		loc = TYPE_VERTEX;
		ptr = reinterpret_cast<void*>(mOffsets[TYPE_VERTEX]);
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(LLVector4),
							  ptr);
	}

	stop_glerror();
	++sSetCount;
}

void LLVertexBuffer::setPositionData(const LLVector4a* data)
{
	mCachedBuffer = true;
	if (!gUsePBRShaders)
	{
		bindGLBuffer();
	}
	flush_vbo(GL_ARRAY_BUFFER, 0, mNumVerts * sizeof(LLVector4a) - 1, (U8*)data);
}

void LLVertexBuffer::setTexCoord0Data(const LLVector2* data)
{
	if (!gUsePBRShaders)
	{
		bindGLBuffer();
	}
	U32 start = mOffsets[TYPE_TEXCOORD0];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + mNumVerts * sizeof(LLVector2) - 1, (U8*)data);
}

void LLVertexBuffer::setTexCoord1Data(const LLVector2* data)
{
	if (!gUsePBRShaders)
	{
		bindGLBuffer();
	}
	U32 start = mOffsets[TYPE_TEXCOORD1];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + mNumVerts * sizeof(LLVector2) - 1, (U8*)data);
}

void LLVertexBuffer::setColorData(const LLColor4U* data)
{
	if (!gUsePBRShaders)
	{
		bindGLBuffer();
	}
	U32 start = mOffsets[TYPE_COLOR];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + mNumVerts * sizeof(LLColor4U) - 1, (U8*)data);
}

// For use by the llgltf library

void LLVertexBuffer::setIndexData(const U16* data)
{
	flush_vbo(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(U16) * mNumIndices - 1,
			  (U8*)data);
}

void LLVertexBuffer::setIndexData(const U32* data)
{
	if (mIndicesType != GL_UNSIGNED_INT)
	{
		// *HACK: vertex buffers are initialized as 16 bits indices, but can be
		// switched to 32 bits indices.
		mIndicesType = GL_UNSIGNED_INT;
		mIndicesStride = sizeof(U32);
		mNumIndices /= 2;
	}
	flush_vbo(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(U32) * mNumIndices - 1,
			  (U8*)data);
}

void LLVertexBuffer::setNormalData(const LLVector4a* data)
{
	U32 start = mOffsets[TYPE_NORMAL];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + sizeof(LLVector4) * mNumVerts - 1, (U8*)data);
}

void LLVertexBuffer::setTangentData(const LLVector4a* data)
{
	U32 start = mOffsets[TYPE_TANGENT];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + sizeof(LLVector4) * mNumVerts - 1, (U8*)data);
}

void LLVertexBuffer::setWeight4Data(const LLVector4a* data)
{
	U32 start = mOffsets[TYPE_WEIGHT4];
	flush_vbo(GL_ARRAY_BUFFER, start,
			  start + sizeof(LLVector4) * mNumVerts - 1, (U8*)data);
}

void LLVertexBuffer::setJointData(const U64* data)
{
	U32 start = mOffsets[TYPE_JOINT];
	flush_vbo(GL_ARRAY_BUFFER, start, start + sizeof(U64) * mNumVerts - 1,
			  (U8*)data);
}

void LLVertexBuffer::setPositionData(const LLVector4a* data, U32 offset, U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector4a);
	flush_vbo(GL_ARRAY_BUFFER, offset * size, (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setTexCoord0Data(const LLVector2* data, U32 offset,
									  U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector2);
	U32 start = mOffsets[TYPE_TEXCOORD0];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setTexCoord1Data(const LLVector2* data, U32 offset,
									  U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector2);
	U32 start = mOffsets[TYPE_TEXCOORD1];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setColorData(const LLColor4U* data, U32 offset, U32 count)
{
	constexpr U32 size = (U32)sizeof(LLColor4U);
	U32 start = mOffsets[TYPE_COLOR];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setIndexData(const U16* data, U32 offset, U32 count)
{
	constexpr U32 size = (U32)sizeof(U16);
	flush_vbo(GL_ELEMENT_ARRAY_BUFFER,
			  offset * size, (offset + count) * size - 1, (U8*)data);
}

void LLVertexBuffer::setIndexData(const U32* data, U32 offset, U32 count)
{
	constexpr U32 size = (U32)sizeof(U32);
	if (mIndicesType != GL_UNSIGNED_INT)
	{
		// *HACK: vertex buffers are initialized as 16 bits indices, but can be
		// switched to 32 bits indices.
		mIndicesType = GL_UNSIGNED_INT;
		mIndicesStride = size;
		mNumIndices /= 2;
	}
	flush_vbo(GL_ELEMENT_ARRAY_BUFFER,
			  offset * size, (offset + count) * size - 1, (U8*)data);
}

void LLVertexBuffer::setNormalData(const LLVector4a* data, U32 offset,
								   U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector4a);
	U32 start = mOffsets[TYPE_NORMAL];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setTangentData(const LLVector4a* data, U32 offset,
									U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector4a);
	U32 start = mOffsets[TYPE_TANGENT];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setWeight4Data(const LLVector4a* data, U32 offset,
									U32 count)
{
	constexpr U32 size = (U32)sizeof(LLVector4a);
	U32 start = mOffsets[TYPE_WEIGHT4];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

void LLVertexBuffer::setJointData(const U64* data, U32 offset, U32 count)
{
	constexpr U32 size = (U32)sizeof(U64);
	U32 start = mOffsets[TYPE_JOINT];
	flush_vbo(GL_ARRAY_BUFFER,
			  start + offset * size, start + (offset + count) * size - 1,
			  (U8*)data);
}

///////////////////////////////////////////////////////////////////////////////
// LLVertexBufferData class
///////////////////////////////////////////////////////////////////////////////

void LLVertexBufferData::draw()
{
	if (mVB.isNull())
	{
		llwarns_once << "NULL vertex buffer. Aborted." << llendl;
		llassert(false);
		return;
	}

	LLGLSLShader* shaderp = LLGLSLShader::sCurBoundShaderPtr;
	if (!shaderp)
	{
		llwarns_once << "No bound shader. Aborted." << llendl;
		llassert(false);
		return;
	}

	if (mTexName)
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mTexName);
	}
	else
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	// Note: attribute mask parameter ignored in PBR mode. HB
	mVB->setBuffer(shaderp->mAttributeMask);
	mVB->drawArrays(mMode, 0, mCount);
}

#if 0	// Not used any more
void LLVertexBufferData::drawWithMatrix()
{
	if (mVB.isNull())
	{
		llwarns_once << "NULL vertex buffer. Aborted." << llendl;
		llassert(false);
		return;
	}

	LLGLSLShader* shaderp = LLGLSLShader::sCurBoundShaderPtr;
	if (!shaderp)
	{
		llwarns_once << "No bound shader. Aborted." << llendl;
		llassert(false);
		return;
	}

	if (mTexName)
	{
		gGL.getTexUnit(0)->bindManual(LLTexUnit::TT_TEXTURE, mTexName);
	}
	else
	{
		gGL.getTexUnit(0)->unbind(LLTexUnit::TT_TEXTURE);
	}

	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.pushMatrix();
	gGL.loadMatrix(mModelView);
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.pushMatrix();
	gGL.loadMatrix(mProjection);
	gGL.matrixMode(LLRender::MM_TEXTURE0);
	gGL.pushMatrix();
	gGL.loadMatrix(mTexture0);

	// Note: attribute mask parameter ignored in PBR mode. HB
	mVB->setBuffer(shaderp->mAttributeMask);
	mVB->drawArrays(mMode, 0, mCount);

	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_PROJECTION);
	gGL.popMatrix();
	gGL.matrixMode(LLRender::MM_MODELVIEW);
	gGL.popMatrix();
}
#endif
