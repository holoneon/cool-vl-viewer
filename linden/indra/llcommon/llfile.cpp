/**
 * @file llfile.cpp
 * @author Michael Schlachter
 * @date 2006-03-23
 * @brief Implementation of cross-platform POSIX file buffer and c++
 * stream classes.
 *
 * $LicenseInfo:firstyear=2006&license=viewerlgpl$
 *
 * Copyright (c) 2006-2009, Linden Research, Inc.
 * Copyright (c) 2009-2025, Henri Beauchamp.
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

#include "linden_common.h"			// Also includes llfile.h

#if LL_WINDOWS
# include <stdlib.h>				// Windows errno
# include <io.h>                 	// _get_osfhandle()
#else
# include <errno.h>
# include <fcntl.h>					// fcntl(), flock, etc
# include <unistd.h>				// symlink(), etc
#endif
#include <stdio.h>

#include <utility>

#include "zlib.h"

#include "llstring.h"

#ifndef S_ISREG
# define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
# define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif

#if !LL_WINDOWS && !defined(S_ISLNK)
# define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)
#endif

using namespace std;

//static
bool LLFile::sWriteError = false;
bool LLFile::sFlushOnWrite = false;

// Many of the methods below use OS-level functions that mess with errno. Wrap
// variants of strerror() to report errors.

#if LL_WINDOWS
// On Windows, use strerror_s().
std::string strerr(int errn)
{
	char buffer[256];
	strerror_s(buffer, errn);	// infers sizeof(buffer) -- love it !
	return buffer;
}

typedef std::basic_ios<char, std::char_traits<char> > _Myios;

#else
// On POSIX we want to call strerror_r(), but alarmingly, there are two
// different variants. The one that returns int always populates the passed
// buffer (except in case of error), whereas the other one always returns a
// valid char* but might or might not populate the passed buffer. How do we
// know which one we are getting ?  Define adapters for each and let the
// compiler select the applicable adapter.

// strerror_r() returns char*
std::string message_from(int, const char*, size_t, const char* strerror_ret)
{
	return strerror_ret;
}

// strerror_r() returns int
std::string message_from(int orig_errno, const char* buffer, size_t bufflen,
						 int strerror_ret)
{
	if (strerror_ret == 0)
	{
		return buffer;
	}
	// Here strerror_r() has set errno. Since strerror_r() has already failed,
	// seems like a poor bet to call it again to diagnose its own error...
	int stre_errno = errno;
	if (stre_errno == ERANGE)
	{
		return llformat("strerror_r() cannot explain errno %d (%d-byte buffer too small)",
						orig_errno, bufflen);
	}
	if (stre_errno == EINVAL)
	{
		return llformat("unknown errno %d", orig_errno);
	}
	// Here we do not even understand the errno from strerror_r() !
	return llformat("strerror_r() cannot explain errno %d (error %d)",
					orig_errno, stre_errno);
}

std::string strerr(int errn)
{
	char buffer[256];
	// Select message_from() function matching the strerror_r() we have on hand
	return message_from(errn, buffer, sizeof(buffer),
						strerror_r(errn, buffer, sizeof(buffer)));
}
#endif	// LL_WINDOWS

// On either system, shorthand call just infers global 'errno'.
std::string strerr()
{
	return strerr(errno);
}

LLFile::LLFile(const std::string& filename, const char* mode, S64* size)
:	mFileName(filename)
{
	if (size)
	{
		llstat st;
		*size = LLFile::stat(filename, &st) == 0 &&
				S_ISREG(st.st_mode) ? (S64)st.st_size : 0;
	}
	mFile = LLFile::open(filename, mode);
}

LLFile::~LLFile()
{
	if (mFile)
	{
		fclose(mFile);
	}
}

LLFile& LLFile::operator=(LLFILE* fp)
{
	if (mFile)
	{
		fclose(mFile);
	}
	mFile = fp;
	mFileName.clear();
	return *this;
}

LLFile& LLFile::operator=(LLFile&& other)
{
	if (mFile)
	{
		fclose(mFile);
	}
	std::swap(mFile, other.mFile);
	std::swap(mFileName, other.mFileName);
	return *this;
}

S64 LLFile::read(U8* bufferp, S64 bytes)
{
	if (!mFile)
	{
		return 0;
	}
	return fread((void*)bufferp, 1, bytes, mFile);
}

S64 LLFile::write(const U8* bufferp, S64 bytes)
{
	if (!mFile)
	{
		return 0;
	}
	S64 written = fwrite((const void*)bufferp, 1, bytes, mFile);
	if (sFlushOnWrite)
	{
		fflush(mFile);
	}
	if (written < bytes)
	{
		sWriteError = true;
	}
	return written;
}

bool LLFile::flush()
{
	return mFile && fflush(mFile) == 0;
}

S64 LLFile::seek(S64 position, bool delta)
{
	if (!mFile)
	{
		return -1;
	}
	if (position < 0)
	{
		fseek(mFile, 0, SEEK_END);
	}
	else if (delta)
	{
		fseek(mFile, position, SEEK_CUR);
	}
	else
	{
		fseek(mFile, position, SEEK_SET);
	}
	return ftell(mFile);
}

bool LLFile::eof()
{
	return !mFile || feof(mFile) != 0;
}

// Implementation borrowed/adapted from APR. HB
bool LLFile::lock(bool exclusive)
{
	if (!mFile)
	{
		return false;
	}
#if LL_WINDOWS
	// Windows does not work like POSIX OSes... An exclusive lock on a file
	// prevents any read access to it from another process and a shared lock
	// prevents writes by anyone, including the lock holder. HB
	DWORD flags = LOCKFILE_FAIL_IMMEDIATELY;
	if (exclusive)
	{
		flags |= LOCKFILE_EXCLUSIVE_LOCK;
	}
    constexpr DWORD len = 0xffffffff;
	OVERLAPPED offset;
	memset(&offset, 0, sizeof(offset));
	HANDLE h = (HANDLE)_get_osfhandle(_fileno(mFile));
	memset(&offset, 0, sizeof(offset));
	return LockFileEx(h, flags, 0, len, len, &offset);
#else
	// With POSIX OSes a write lock still allows the lock holder to write to
	// the file, while all others can still read from it. We therefore can use
	// the write lock and ignore entirely the 'exclusive' boolean. HB
	struct flock l = { 0 };
	l.l_whence = SEEK_SET;		// Lock from start of file
	l.l_start = 0;				// Begin lock at this offset
	l.l_len = 0;				// Lock to end of file
	l.l_type = F_WRLCK;			// Write lock ('exclusive' ignored).
	constexpr int fc = F_SETLK;	// Non-blocking lock.
	// Keep trying if fcntl() gets interrupted (by a signal)
	int rc;
	while ((rc = fcntl(fileno(mFile), fc, &l)) < 0 && errno == EINTR) ;
	return rc != -1;
#endif
}

bool LLFile::unlock()
{
	if (!mFile)
	{
		return false;
	}
#if LL_WINDOWS
    constexpr DWORD len = 0xffffffff;
	OVERLAPPED offset;
	memset(&offset, 0, sizeof(offset));
	HANDLE h = (HANDLE)_get_osfhandle(_fileno(mFile));
	memset(&offset, 0, sizeof(offset));
	return UnlockFileEx(h, 0, len, len, &offset);
#else
	struct flock l = { 0 };
	l.l_whence = SEEK_SET;		// Lock from start of file
	l.l_start = 0;				// Begin lock at this offset
	l.l_len = 0;				// Lock to end of file
	l.l_type = F_UNLCK;			// Unlock.
	constexpr int fc = F_SETLK;	// Non-blocking lock.
	// Keep trying if fcntl() gets interrupted (by a signal)
	int rc;
	while ((rc = fcntl(fileno(mFile), fc, &l)) < 0 && errno == EINTR) ;
	return rc != -1;
#endif
}

//static
bool LLFile::mkdir(const std::string& dirname, U16 perms)
{
#if LL_WINDOWS
	// Permissions are ignored on Windows
	int rc = _wmkdir(ll_convert_string_to_wide(dirname).c_str());
#else
	int rc = ::mkdir(dirname.c_str(), (mode_t)perms);
#endif
	if (rc < 0)
	{
		// Capture errno before we start emitting output
		int errn = errno;
		// We often use mkdir() to ensure the existence of a directory that
		// might already exist. Consider it a success when that directory
		// already exists.
		if (errn != EEXIST)
		{
			llwarns << "Failed on '" << dirname << "' (errno " << errn
					<< "): " << strerr(errn) << llendl;
			return false;
		}
	}
	return true;
}

//static
bool LLFile::rmdir(const std::string& dirname)
{
#if LL_WINDOWS
	// Permissions are ignored on Windows
	int rc = _wrmdir(ll_convert_string_to_wide(dirname).c_str());
#else
	int rc = ::rmdir(dirname.c_str());
#endif
	if (rc < 0)
	{
		// Capture errno before we start emitting output
		int errn = errno;
		llwarns << "Failed on '" << dirname << "' (errno " << errn
				<< "): " << strerr(errn) << llendl;
		return false;
	}
	return true;
}

//static
LLFILE* LLFile::open(const std::string& filename, const char* mode)
{
#if LL_WINDOWS
	return _wfopen(ll_convert_string_to_wide(filename).c_str(),
				   ll_convert_string_to_wide(mode).c_str());
#else
	return fopen(filename.c_str(), mode);
#endif
}

//static
LLFILE* LLFile::open(const char* filename, const char* mode)
{
#if LL_WINDOWS
	return _wfopen(ll_convert_string_to_wide(filename).c_str(),
				   ll_convert_string_to_wide(mode).c_str());
#else
	return fopen(filename, mode);
#endif
}

//static
void LLFile::close(LLFILE* fp)
{
	if (fp)
	{
		// Note: we do not care about errors when closing
		fclose(fp);
	}
}

//static
bool LLFile::remove(const std::string& filename)
{
#if LL_WINDOWS
	int rc = _wremove(ll_convert_string_to_wide(filename).c_str());
#else
	int rc = ::remove(filename.c_str());
#endif
	if (rc)
	{
		// We do not care if the file to be removed does not exist.
		// Do not spam the log with such warnings either.
		if (errno != ENOENT)
		{
			// Capture errno before we start emitting output
			int errn = errno;
			llwarns << "Failed on '" << filename << "' (errno " << errn
					<< "): " << strerr(errn) << llendl;
			return false;
		}
	}
	return true;
}

//static
bool LLFile::rename(const std::string& filename, const std::string& newname,
					bool ignore_cross_linking)
{
#if LL_WINDOWS
	int rc = _wrename(ll_convert_string_to_wide(filename).c_str(),
					  ll_convert_string_to_wide(newname).c_str());
	// Capture errno before we (possibly) start emitting output
	int errn = errno;
#else
	int rc = ::rename(filename.c_str(), newname.c_str());
	int errn = errno;	// Capture errno before it (possibly) changes
	if (rc && errn == EXDEV)
	{
		if (LLFile::copy(filename, newname))
		{
			if (!ignore_cross_linking)
			{
				llinfos << "Rename across mounts detected; moving "
						<< filename << " to " << newname << " instead."
						<< llendl;
			}
			unlink(filename.c_str());
			return true;
		}
	}
#endif
	if (rc)
	{
		llwarns << "Failed to rename \""<< filename << "\" to \"" << newname
				<< " \" (errno " << errn << "): " << strerr(errn) << llendl;
		return false;
	}
	return true;
}

//static
bool LLFile::copy(const std::string& from, const std::string& to)
{
	bool copied = false;

	LLFILE* in = LLFile::open(from, "rb");
	if (in)
	{
		LLFILE* out = LLFile::open(to, "wb");
		if (out)
		{
			char buf[16384];
			size_t readbytes;
			bool write_ok = true;
			while (write_ok && (readbytes = fread(buf, 1, 16384, in)))
			{
				if (fwrite(buf, 1, readbytes, out) != readbytes)
				{
					llwarns << "Short write to: " << to << llendl;
					write_ok = false;
					sWriteError = true;
				}
			}
			if (write_ok)
			{
				copied = true;
			}
			fclose(out);
		}
		fclose(in);
	}

	return copied;
}

// Returns the OS stat error code (with errno reflecting the actual error, when
// it occurs): we do not warn on failures here, since this call is used in
// places where the appropriate action will be taken when a failure occurs
// (most of the time, we use stat to just check that a file exists). HB
//static
S32 LLFile::stat(const std::string& filename, llstat* filestatusp)
{
#if LL_WINDOWS
	return _wstat(ll_convert_string_to_wide(filename).c_str(), filestatusp);
#else
	return ::stat(filename.c_str(), filestatusp);
#endif
}

//static
bool LLFile::exists(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0;
}

//static
bool LLFile::isdir(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0 && S_ISDIR(st.st_mode);
}

//static
bool LLFile::isfile(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0 && S_ISREG(st.st_mode);
}

#if !LL_WINDOWS
//static
bool LLFile::islink(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0 && S_ISLNK(st.st_mode);
}
#endif

//static
size_t LLFile::getFileSize(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0 &&
		   S_ISREG(st.st_mode) ? st.st_size : 0;
}

//static
time_t LLFile::lastModified(const std::string& filename)
{
	llstat st;
	return LLFile::stat(filename, &st) == 0 ? st.st_mtime : 0;
}

//static
const char* LLFile::tmpdir()
{
	static std::string utf8path;
	if (utf8path.empty())
	{
#if LL_WINDOWS
		std::vector<wchar_t> utf16path(MAX_PATH + 1);
		GetTempPathW(MAX_PATH, utf16path.data());
		utf8path = ll_convert_wide_to_string(utf16path.data());
#else
		char* env_tmp = getenv("TMP");
		if (!env_tmp)
		{
			env_tmp = getenv("TMPDIR");
		}
		utf8path = env_tmp ? env_tmp : "/tmp/";
#endif
		if (utf8path.back() != LL_DIR_DELIM_CHR)
		{
			utf8path += LL_DIR_DELIM_CHR;
		}
	}
	return utf8path.c_str();
}

//static
S64 LLFile::readEx(const std::string& filename, void* buf, S64 offset,
				   S64 nbytes)
{
	if (offset < 0)
	{
		llwarns << "Negative offset passed to read: " << filename << llendl;
		llassert(false);
		return 0;
	}

	LLFile infile(filename, "rb");
	if (!infile)
	{
		llwarns << "Failed to open for reading: " << filename << llendl;
		return 0;
	}

	if (offset > 0 && infile.seek(offset) != offset)
	{
		llwarns << "Failed to seek at offset " << offset << " in file: "
				<< filename << llendl;
		return 0;
	}

	S64 bytes_read = infile.read((U8*)buf, nbytes);
	if (bytes_read != nbytes)
	{
		llwarns << "Failed to read " << nbytes << " bytes from file: "
				<< filename << llendl;
		return 0;
	}
	llassert_always(bytes_read <= 0x7fffffff);
	return bytes_read;
}

//static
S64 LLFile::writeEx(const std::string& filename, void* buf, S64 offset,
					S64 nbytes)
{
	bool exists = LLFile::exists(filename);
	const char* flags = exists ? (offset < 0 ? "ab" : "r+b") : "wb";
	LLFile outfile(filename, flags);
	if (!outfile)
	{
		llwarns << "Failed to open for writing: " << filename << llendl;
		return 0;
	}

	if (offset > 0 && outfile.seek(offset) != offset)
	{
		llwarns << "Failed to seek at offset " << offset << " in file: "
				<< filename << llendl;
		return 0;
	}

	S64 bytes_written = outfile.write((U8*)buf, nbytes);
	if (bytes_written != nbytes)
	{
		llwarns << "Failed to write " << nbytes << " bytes to file: "
				<< filename << llendl;
		sWriteError = true;
		return 0;
	}
	llassert_always(bytes_written <= 0x7fffffff);
	return bytes_written;
}

//static
std::string LLFile::getContents(const std::string& filename)
{
	std::string contents;
	LLFILE* fp = LLFile::open(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		size_t length = (size_t)ftell(fp);
		if (length > 0)
		{
			fseek(fp, 0, SEEK_SET);
			std::vector<char> buffer(length);
			size_t nread = fread(buffer.data(), 1, length, fp);
			if (nread > 0)
			{
				contents.assign(buffer.data(), nread);
			}
		}
		fclose(fp);
	}
	return contents;
}

//static
bool LLFile::touch(const std::string& filename)
{
	if (filename.empty())
	{
		return false;
	}
    llstat st;
	if (LLFile::stat(filename, &st) != 0)
	{
		// File does not exist: create an empty file.
		llofstream outfile(filename);
		if (!outfile.is_open())
		{
			llwarns << "Failed to create an empty file for non-existent "
					<< filename << llendl;
			return false;
		}
	}
	else if (!S_ISREG(st.st_mode))
	{
		llwarns << "Target " << filename << " is not a regular file."
				<< llendl;
		return false;
	}
	return true;
}

//static
bool LLFile::createFileSymlink(const std::string& filename,
							   const std::string& link)
{
	if (link.empty() || !touch(filename))
	{
		return false;
	}
	int ec;
#if LL_WINDOWS
	// We cannot create soft links under Windows unless the program is elevated
	// to admin privileges. So, just use a hard link.
	auto w_fname = ll_convert_string_to_wide(filename);
	auto w_link = ll_convert_string_to_wide(link);
	ec = CreateHardLinkA((LPCSTR)w_link.c_str(), (LPCSTR)w_fname.c_str(),
						 NULL);
#else
	ec = symlink(filename.c_str(), link.c_str());
#endif
	if (ec)
	{
		llwarns << "Failed to create symbolic link " << link << " for file "
				<< filename << llendl;
		return false;
	}
	return true;
}

//static
bool LLFile::gzip(const std::string& srcfile, const std::string& dstfile)
{
	constexpr S32 COMPRESS_BUFFER_SIZE = 32768;
	U8 buffer[COMPRESS_BUFFER_SIZE];

	std::string tmpfile = dstfile + ".tmp";
#if LL_WINDOWS
	gzFile dst = gzopen_w(ll_convert_string_to_wide(tmpfile).c_str(), "wb");
#else
	gzFile dst = gzopen(tmpfile.c_str(), "wb");
#endif
	if (!dst)
	{
		return false;
	}

	LLFILE* src = LLFile::open(srcfile, "rb");
	if (!src)
	{
		gzclose(dst);
		return false;
	}

	do
	{
		size_t bytes = fread(buffer, sizeof(U8), COMPRESS_BUFFER_SIZE, src);
		gzwrite(dst, buffer, bytes);
	}
	while (!feof(src));

	LLFile::close(src);
	gzclose(dst);

	LLFile::remove(dstfile);

	return LLFile::rename(tmpfile, dstfile);
}

//static
bool LLFile::gunzip(const std::string& srcfile, const std::string& dstfile)
{
	constexpr S32 UNCOMPRESS_BUFFER_SIZE = 32768;
	U8 buffer[UNCOMPRESS_BUFFER_SIZE];

#if LL_WINDOWS
	gzFile src = gzopen_w(ll_convert_string_to_wide(srcfile).c_str(), "rb");
#else
	gzFile src = gzopen(srcfile.c_str(), "rb");
#endif
	if (!src)
	{
		return false;
	}

	std::string tmpfile = dstfile + ".tmp";
	LLFILE* dst = LLFile::open(tmpfile, "wb");
	if (!dst)
	{
		gzclose(src);
		return false;
	}

	do
	{
		size_t bytes = gzread(src, buffer, UNCOMPRESS_BUFFER_SIZE);
		size_t nwrit = fwrite(buffer, sizeof(U8), bytes, dst);
		if (nwrit < bytes)
		{
			llwarns << "Short write on " << tmpfile << ": Wrote " << nwrit
					<< " of " << bytes << " bytes." << llendl;
			LLFile::setWriteError();
			gzclose(src);
			LLFile::close(dst);
			return false;
		}
	}
	while (!gzeof(src));

	gzclose(src);
	LLFile::close(dst);

	LLFile::remove(dstfile);

	return LLFile::rename(tmpfile, dstfile);
}

#if LL_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// Modified file stream created to overcome the incorrect behaviour of POSIX
// fopen in windows
///////////////////////////////////////////////////////////////////////////////

// Input file stream

llifstream::llifstream()
{
}

//explicit
llifstream::llifstream(const std::string& filename, ios_base::openmode mode)
:	std::ifstream(ll_convert_string_to_wide(filename).c_str(),
				  mode | ios_base::in)
{
}

void llifstream::open(const std::string& filename, ios_base::openmode mode)
{
	std::ifstream::open(ll_convert_string_to_wide(filename).c_str(),
						mode | ios_base::in);
}

// Output file stream

llofstream::llofstream()
{
}

//explicit
llofstream::llofstream(const std::string& filename, ios_base::openmode mode)
:	std::ofstream(ll_convert_string_to_wide(filename), mode | ios_base::out)
{
}

void llofstream::open(const std::string& filename, ios_base::openmode mode)
{
	std::ofstream::open(ll_convert_string_to_wide(filename).c_str(),
						mode | ios_base::out);
}

bool has_wine_append_file_write_bug(const std::string& temp_file)
{
	LLFile::remove(temp_file);
	const char* bufferp = "01234567890";
	FILE* fp = LLFile::open(temp_file, "wb");
	fwrite((void*)bufferp, 1, strlen(bufferp), fp);
	LLFile::close(fp);
	fp = LLFile::open(temp_file, "a+b");
	fwrite((void*)bufferp, 1, strlen(bufferp), fp);
	U32 pos1 = (U32)ftell(fp);
	fflush(fp);
	U32 pos2 = (U32)ftell(fp);
	LLFile::close(fp);
	LLFile::remove(temp_file);
	if (pos1 != pos2)
	{
		LLFile::sFlushOnWrite = true;
		return true;
	}
	return false;
}

#endif	// LL_WINDOWS
