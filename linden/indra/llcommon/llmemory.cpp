/**
 * @file llmemory.cpp
 * @brief Very special memory allocation/deallocation stuff here
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

#include "linden_common.h"

#include "llmemory.h"

#include "llsys.h"

#if LL_JEMALLOC
# include "jemalloc/jemalloc.h"
#elif LL_MIMALLOC && LL_WINDOWS
# include "mimalloc/mimalloc-new-delete.h"
#endif

#if LL_LINUX
# include <malloc.h>
# include <sys/resource.h>
# include <unistd.h>
#elif LL_WINDOWS
# include <malloc.h>
# include <psapi.h>
#endif

void* gReservedFreeSpace = NULL;

//static
U32 LLMemory::sMaxPhysicalMemInKB = 0;
U32 LLMemory::sMaxVirtualMemInKB = 0;
U32 LLMemory::sAvailPhysicalMemInKB = U32_MAX;
U32 LLMemory::sAvailVirtualMemInKB = U32_MAX;
U32 LLMemory::sAllocatedMemInKB = 0;
U32 LLMemory::sAllocatedPageSizeInKB = 0;
bool LLMemory::sFailedAllocation = false;
bool LLMemory::sFailedAllocationOnce = false;
#if LL_LINUX && !LL_JEMALLOC
// Stats: number of successful malloc timming.
static U32 sTrimmed = 0;
#endif

#if LL_DEBUG
void ll_assert_aligned_error()
{
	llerrs << "Alignment check failed !" << llendl;
}
#endif

//static
void LLMemory::initClass()
{
#if LL_JEMALLOC
	unsigned int oval, nval = 0;
	size_t osz = sizeof(oval);
	size_t nsz = sizeof(nval);
	mallctl("thread.arena", &oval, &osz, &nval, nsz);
#endif

	if (!gReservedFreeSpace)
	{
		// Reserve some space that we will free() on crash to try and avoid out
		// of memory conditions while dumping the stack trace log... 256Kb
		// should be plenty enough.
		gReservedFreeSpace = malloc(262144);
	}
}

//static
void LLMemory::cleanupClass()
{
	if (gReservedFreeSpace)
	{
		free(gReservedFreeSpace);
		gReservedFreeSpace = NULL;
	}
}

//static
void LLMemory::allocationFailed(size_t size)
{
	sFailedAllocation = sFailedAllocationOnce = true;
	if (size > 0)
	{
		llwarns << "Memory allocation failure for size: " << size << llendl;
	}
}

//static
void LLMemory::updateMemoryInfo(bool trim_heap)
{
	// jemalloc v5.0+ does properly release memory to the system, and v5.2+
	// does redirect new and delete calls to its own allocators, so there is
	// no need for trimming with the system malloc() here. HB
#if !LL_JEMALLOC
	if (trim_heap)
	{
		// Trim the heap down from freed memory so that we can compute the
		// actual available virtual space. HB
# if LL_LINUX
		constexpr size_t keep = 100 * 1024 * 1024; 	// Trim all but 100MB
		sTrimmed += malloc_trim(keep);
# elif LL_WINDOWS
		_heapmin();
# endif
	}
#endif

	getMaxMemoryKB(sMaxPhysicalMemInKB, sMaxVirtualMemInKB);
	getAvailableMemoryKB(sAvailPhysicalMemInKB, sAvailVirtualMemInKB);
#if LL_WINDOWS
	HANDLE self = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS counters;

	if (!GetProcessMemoryInfo(self, &counters, sizeof(counters)))
	{
		llwarns << "GetProcessMemoryInfo failed" << llendl;
		sAllocatedPageSizeInKB = sMaxVirtualMemInKB - sAvailVirtualMemInKB;
		sAllocatedMemInKB = 0;
	}
	else
	{
		sAllocatedPageSizeInKB = (U32)(counters.PagefileUsage / 1024);
		sAllocatedMemInKB = (U32)(counters.WorkingSetSize / 1024);
	}
#else
	sAllocatedPageSizeInKB = sMaxVirtualMemInKB - sAvailVirtualMemInKB;
	sAllocatedMemInKB = (U32)(LLMemory::getCurrentRSS() / 1024);
#endif
}

#if LL_JEMALLOC
#define JEMALLOC_STATS_STRING_SIZE 262144
static void jemalloc_write_cb(void* data, const char* s)
{
	if (data && s)
	{
		std::string* buff = (std::string*)data;
		size_t buff_len = buff->length();
		size_t line_len = strlen(s);
		if (buff_len + line_len >= JEMALLOC_STATS_STRING_SIZE)
		{
			line_len = JEMALLOC_STATS_STRING_SIZE - buff_len - 1;
		}
		if (line_len > 0)
		{
			buff->append(s, line_len);
		}
	}
}
#endif

//static
void LLMemory::logMemoryInfo()
{
	updateMemoryInfo();

#if LL_JEMALLOC
	unsigned int arenas, arena;
	size_t sz = sizeof(arenas);
	mallctl("opt.narenas", &arenas, &sz, NULL, 0);
	U32 opt_arenas = arenas;
	mallctl("arenas.narenas", &arenas, &sz, NULL, 0);
	mallctl("thread.arena", &arena, &sz, NULL, 0);
	llinfos << "jemalloc initialized with " << opt_arenas
			<< " arenas, using now " << arenas
			<< " arenas, main thread using arena " << arena << "." << llendl;

	bool stats_enabled = false;
	sz = sizeof(stats_enabled);
	mallctl("config.stats", &stats_enabled, &sz, NULL, 0);
	if (stats_enabled)
	{
		std::string malloc_stats_str;
		// IMPORTANT: we cannot reserve memory during jemalloc_write_cb() call
		// by malloc_stats_print(), so we reserve a fixed string buffer.
		malloc_stats_str.reserve(JEMALLOC_STATS_STRING_SIZE);
		malloc_stats_print(jemalloc_write_cb, &malloc_stats_str, NULL);
		llinfos << "jemalloc stats:\n" << malloc_stats_str << llendl;
	}
#endif

	llinfos << "System memory information: Max physical memory: "
			<< sMaxPhysicalMemInKB << "KB - Allocated physical memory: "
			<< sAllocatedMemInKB << "KB - Available physical memory: "
			<< sAvailPhysicalMemInKB << "KB - Allocated virtual memory: "
			<< sAllocatedPageSizeInKB << "KB"
#if LL_LINUX && !LL_JEMALLOC
			<< " - Number of actual glibc malloc() heap trimming occurrences: "
			<< sTrimmed
#endif
			<< llendl;
}

#if LL_WINDOWS

U64 LLMemory::getCurrentRSS()
{
	HANDLE self = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS counters;

	if (!GetProcessMemoryInfo(self, &counters, sizeof(counters)))
	{
		llwarns_once << "GetProcessMemoryInfo() failed !" << llendl;
		return 0;
	}

	return counters.WorkingSetSize;
}

#elif LL_LINUX

U64 LLMemory::getCurrentRSS()
{
	struct rusage usage;
	if (getrusage(RUSAGE_SELF, &usage))
	{
		llwarns_once << "getrusage() failed !" << llendl;
		return 0;
	}
	return usage.ru_maxrss * 1024;
}

#else

U64 LLMemory::getCurrentRSS()
{
	return 0;
}

#endif

///////////////////////////////////////////////////////////////////////////////
// The following methods used to be part of LLMemoryInfo (in llsys.h/cpp),
// which I removed to merge here, since they did not even relate with global
// system info, but instead with the memory consumption of the viewer itself...
// HB

//static
U32 LLMemory::getPhysicalMemoryKB()
{
#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);
	U32 amount = state.ullTotalPhys >> 10;

	// *HACK: for some reason, the reported amount of memory is always wrong.
	// The original adjustment assumes it is always off by one meg, however
	// errors of as much as 2520 KB have been observed in the value returned
	// from the GetMemoryStatusEx function.  Here we keep the original
	// adjustment from llfoaterabout.cpp until this can be fixed somehow. HB
	amount += 1024;

	return amount;
#elif LL_LINUX
	U64 phys = (U64)(sysconf(_SC_PAGESIZE)) * (U64)(sysconf(_SC_PHYS_PAGES));
	return (U32)(phys >> 10);
#else
	return 0;
#endif
}

//static
void LLMemory::getMaxMemoryKB(U32& max_physical_mem_kb,
							  U32& max_virtual_mem_kb)
{
	static U32 saved_max_physical_mem_kb = 0;
	static U32 saved_max_virtual_mem_kb = 0;
	if (saved_max_virtual_mem_kb)
	{
		max_physical_mem_kb = saved_max_physical_mem_kb;
		max_virtual_mem_kb = saved_max_virtual_mem_kb;
		return;
	}

	U32 addressable = U32_MAX;	// No limit...
#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);
	max_physical_mem_kb = (U32)(state.ullAvailPhys / 1024);
	U32 total_virtual_memory = (U32)(state.ullTotalVirtual / 1024);
	max_virtual_mem_kb = total_virtual_memory;
#elif LL_LINUX
	max_physical_mem_kb = getPhysicalMemoryKB();
	max_virtual_mem_kb = addressable;
#else
	max_physical_mem_kb = max_virtual_mem_kb = addressable;
#endif
	max_virtual_mem_kb = llmin(max_virtual_mem_kb, addressable);

#if LL_WINDOWS
	LL_DEBUGS("Memory") << "Total physical memory: "
						<< max_physical_mem_kb / 1024
						<< "MB - Total available virtual memory: "
						<< total_virtual_memory / 1024
						<< "MB - Retained max virtual memory: "
						<< max_virtual_mem_kb / 1024 << "MB" << LL_ENDL;
#else
	LL_DEBUGS("Memory") << "Total physical memory: "
						<< max_physical_mem_kb / 1024
						<< "MB - Retained max virtual memory: "
						<< max_virtual_mem_kb / 1024 << "MB" << LL_ENDL;
#endif
	saved_max_physical_mem_kb = max_physical_mem_kb;
	saved_max_virtual_mem_kb = max_virtual_mem_kb;
}

#if LL_LINUX
static U32 get_process_virtual_size_kb()
{
	U32 virtual_size = 0;
	LLFILE* status_filep = LLFile::open("/proc/self/status", "rb");
	if (status_filep)
	{
		constexpr S32 STATUS_SIZE = 8192;
		static char buff[STATUS_SIZE];

		size_t nbytes = fread(buff, 1, STATUS_SIZE - 1, status_filep);
		buff[nbytes] = '\0';
		LLFile::close(status_filep);

		// All these guys return numbers in KB
		U32 temp = 0;
		char* memp = strstr(buff, "VmRSS:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &temp);
			virtual_size = temp;
		}
		memp = strstr(buff, "VmStk:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &temp);
			virtual_size += temp;
		}
		memp = strstr(buff, "VmExe:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &temp);
			virtual_size += temp;
		}
		memp = strstr(buff, "VmLib:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &temp);
			virtual_size += temp;
		}
		memp = strstr(buff, "VmPTE:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &temp);
			virtual_size += temp;
		}
	}
	return virtual_size;
}
#endif	// LL_LINUX

//static
void LLMemory::getAvailableMemoryKB(U32& avail_physical_mem_kb,
									U32& avail_virtual_mem_kb)
{
	U32 max_physical_mem_kb, max_virtual_mem_kb;
	getMaxMemoryKB(max_physical_mem_kb, max_virtual_mem_kb);
#if LL_LINUX
	avail_physical_mem_kb = U32_MAX;
	LLFILE* meminfo_filep = LLFile::open("/proc/meminfo", "rb");
	if (meminfo_filep)
	{
		constexpr S32 MEMINFO_SIZE = 4096;
		static char buff[MEMINFO_SIZE];
		size_t nbytes = fread(buff, 1, MEMINFO_SIZE - 1, meminfo_filep);
		buff[nbytes] = '\0';
		LLFile::close(meminfo_filep);
		char* memp = strstr(buff, "MemAvailable:");
		if (memp)
		{
			sscanf(memp, "%*s %u", &avail_physical_mem_kb);
			if (avail_physical_mem_kb == 0)	// Unknown available mem.
			{
				avail_physical_mem_kb = U32_MAX;
			}
		}
		if (avail_physical_mem_kb == U32_MAX)
		{
			memp = strstr(buff, "MemFree:");
			if (memp)
			{
				sscanf(memp, "%*s %u", &avail_physical_mem_kb);
			}
		}
	}
	if (avail_physical_mem_kb == U32_MAX)
	{
		avail_physical_mem_kb = max_physical_mem_kb;
	}
#endif

#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);
	avail_virtual_mem_kb = (U32)(state.ullAvailVirtual / 1024);
	avail_physical_mem_kb = (U32)(state.ullAvailPhys / 1024);

	LL_DEBUGS("Memory") << "Memory check: reported available virtual space: "
						<< avail_virtual_mem_kb / 1024 << "MB" << LL_ENDL;
#else
	U32 virtual_size_kb = get_process_virtual_size_kb();
	if (virtual_size_kb < max_virtual_mem_kb)
	{
		avail_virtual_mem_kb = max_virtual_mem_kb - virtual_size_kb;
	}
	else
	{
		avail_virtual_mem_kb = 0;
	}
#endif

	LL_DEBUGS("Memory") << "Free physical memory: "
						<< avail_physical_mem_kb / 1024
						<< "MB - Available virtual space: "
						<< avail_virtual_mem_kb / 1024 << "MB" << LL_ENDL;
}

//static
std::string LLMemory::getInfo()
{
	std::ostringstream s;

#if LL_WINDOWS
	MEMORYSTATUSEX state;
	state.dwLength = sizeof(state);
	GlobalMemoryStatusEx(&state);

	s << "Percent Memory use: " << (U32)state.dwMemoryLoad << '%' << std::endl;
	s << "Total Physical KB:  " << (U32)(state.ullTotalPhys / 1024) << std::endl;
	s << "Avail Physical KB:  " << (U32)(state.ullAvailPhys / 1024) << std::endl;
	s << "Total page KB:      " << (U32)(state.ullTotalPageFile / 1024) << std::endl;
	s << "Avail page KB:      " << (U32)(state.ullAvailPageFile / 1024) << std::endl;
	s << "Total Virtual KB:   " << (U32)(state.ullTotalVirtual / 1024) << std::endl;
	s << "Avail Virtual KB:   " << (U32)(state.ullAvailVirtual / 1024) << std::endl;
#else
	LLFILE* meminfo = LLFile::open("/proc/meminfo", "rb");
	if (meminfo)
	{
		static char line[MAX_STRING];
		memset(line, 0, MAX_STRING);
		while (fgets(line, MAX_STRING, meminfo))
		{
			line[strlen(line) - 1] = ' ';
			s << line;
		}
		LLFile::close(meminfo);
	}
	else
	{
		s << "Unable to collect memory information";
	}
#endif

	return s.str();
}

// End of former LLMemoryInfo methods
///////////////////////////////////////////////////////////////////////////////

template <typename T> T* LL_NEXT_ALIGNED_ADDRESS_64(T* address) 
{ 
	return reinterpret_cast<T*>(
		(reinterpret_cast<uintptr_t>(address) + 0x3F) & ~0x3F);
}

// Used to be force-inlined in llmemory.h, but does not really deserve it. HB
void ll_memcpy_nonaliased_aligned_16(char* __restrict dst,
									 const char* __restrict src, size_t bytes)
{
	llassert(src != NULL && dst != NULL);
	llassert(bytes > 0 && bytes % sizeof(F32) == 0 && bytes % 16 == 0);
	llassert(src < dst ? src + bytes <= dst : dst + bytes <= src);
	ll_assert_aligned(src, 16);
	ll_assert_aligned(dst, 16);

	char* end = dst + bytes;

	if (bytes > 64)
	{
		// Find start of 64 bytes-aligned area within block
		void* begin_64 = LL_NEXT_ALIGNED_ADDRESS_64(dst);

		// At least 64 bytes before the end of the destination, switch to 16
		// bytes copies
		void* end_64 = end - 64;

		// Prefetch the head of the 64 bytes area now
		_mm_prefetch((char*)begin_64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 64, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 128, _MM_HINT_NTA);
		_mm_prefetch((char*)begin_64 + 192, _MM_HINT_NTA);

		// Copy 16 bytes chunks until we're 64 bytes aligned
		while (dst < begin_64)
		{

			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			dst += 16;
			src += 16;
		}

		// Copy 64 bytes chunks up to your tail
		//
		// Might be good to shmoo the 512b prefetch offset (characterize
		// performance for various values)
		while (dst < end_64)
		{
			_mm_prefetch((char*)src + 512, _MM_HINT_NTA);
			_mm_prefetch((char*)dst + 512, _MM_HINT_NTA);
			_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
			_mm_store_ps((F32*)(dst + 16), _mm_load_ps((F32*)(src + 16)));
			_mm_store_ps((F32*)(dst + 32), _mm_load_ps((F32*)(src + 32)));
			_mm_store_ps((F32*)(dst + 48), _mm_load_ps((F32*)(src + 48)));
			dst += 64;
			src += 64;
		}
	}

	// Copy remainder 16 bytes tail chunks (or ALL 16 bytes chunks for
	// sub-64 bytes copies)
	while (dst < end)
	{
		_mm_store_ps((F32*)dst, _mm_load_ps((F32*)src));
		dst += 16;
		src += 16;
	}
}
