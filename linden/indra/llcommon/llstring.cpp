/**
 * @file llstring.cpp
 * @brief String utility functions and the std::string class.
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

#include <cstdarg>

#if LL_WINDOWS
# include <winnls.h>	// For WideCharToMultiByte
# include <vector>
#endif

#include "llstring.h"

U8 hex_as_nybble(char hex)
{
	if (hex >= '0' && hex <= '9')
	{
		return (U8)(hex - '0');
	}
	if (hex >= 'a' && hex <='f')
	{
		return (U8)(10 + hex - 'a');
	}
	if (hex >= 'A' && hex <='F')
	{
		return (U8)(10 + hex - 'A');
	}
	return 0; // uh - oh, not hex any more...
}

// See http://www.unicode.org/Public/BETA/CVTUTF-1-2/ConvertUTF.c
// for the Unicode implementation - this doesn't match because it was written
// before finding it.

std::ostream& operator<<(std::ostream& s, const LLWString& wstr)
{
	std::string utf8_str = wstring_to_utf8str(wstr);
	s << utf8_str;
	return s;
}

std::ptrdiff_t wchar_to_utf8chars(llwchar in_char, char* outchars)
{
	U32 cur_char = (U32)in_char;
	char* base = outchars;
	if (cur_char < 0x80)
	{
		*outchars++ = (U8)cur_char;
	}
	else if (cur_char < 0x800)
	{
		*outchars++ = 0xC0 | (cur_char >> 6);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x10000)
	{
		*outchars++ = 0xE0 | (cur_char >> 12);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x200000)
	{
		*outchars++ = 0xF0 | (cur_char >> 18);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x4000000)
	{
		*outchars++ = 0xF8 | (cur_char >> 24);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else if (cur_char < 0x80000000)
	{
		*outchars++ = 0xFC | (cur_char >> 30);
		*outchars++ = 0x80 | ((cur_char >> 24) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 18) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 12) & 0x3F);
		*outchars++ = 0x80 | ((cur_char >> 6) & 0x3F);
		*outchars++ = 0x80 | (cur_char & 0x3F);
	}
	else
	{
		llwarns << "Invalid Unicode character " << cur_char << "!" << llendl;
		*outchars++ = LL_UNKNOWN_CHAR;
	}
	return outchars - base;
}

static std::ptrdiff_t utf16chars_to_wchar(const llchar16* inchars,
										  llwchar* outchar)
{
	const llchar16* base = inchars;
	llchar16 cur_char = *inchars++;
	llwchar char32 = cur_char;
	if ((cur_char >= 0xD800) && (cur_char <= 0xDFFF))
	{
		// Surrogates
		char32 = ((llwchar)(cur_char - 0xD800)) << 10;
		cur_char = *inchars++;
		char32 += (llwchar)(cur_char - 0xDC00) + 0x0010000UL;
	}
	else
	{
		char32 = (llwchar)cur_char;
	}
	*outchar = char32;
	return std::ptrdiff_t(inchars - base);
}

llutf16string wstring_to_utf16str(const LLWString& utf32str, S32 len)
{
	llutf16string out;
	out.reserve(3 * len / 2);
	S32 i = 0;
	while (i < len)
	{
		U32 cur_char = utf32str[i++];
		if (cur_char > 0xFFFF)
		{
			out += (0xD7C0 + (cur_char >> 10));
			out += (0xDC00 | (cur_char & 0x3FF));
		}
		else
		{
			out += cur_char;
		}
	}
	return out;
}

LLWString utf16str_to_wstring(const llutf16string& utf16str, S32 len)
{
	LLWString wout;
	if (len <= 0 || utf16str.empty()) return wout;

	S32 i = 0;
	// Craziness to make gcc happy (llutf16string.c_str() is tweaked on linux):
	const llchar16* chars16 = &(*(utf16str.begin()));
	while (i < len)
	{
		llwchar cur_char;
		i += utf16chars_to_wchar(chars16 + i, &cur_char);
		wout += cur_char;
	}
	return wout;
}

// Length in llwchar (UTF-32) of the first len units (16 bits) of the given
// UTF-16 string.
S32 utf16str_wstring_length(const llutf16string& utf16str, S32 utf16_len)
{
	S32 surrogate_pairs = 0;
	// ... craziness to make gcc happy (llutf16string.c_str() is tweaked on
	// linux):
	const llchar16* const utf16_chars = &(*(utf16str.begin()));
	S32 i = 0;
	while (i < utf16_len)
	{
		const llchar16 c = utf16_chars[i++];
		if (c >= 0xD800 && c <= 0xDBFF)	// See http://en.wikipedia.org/wiki/UTF-16
		{ 
			 // Have first byte of a surrogate pair
			if (i >= utf16_len)
			{
				break;
			}
			const llchar16 d = utf16_chars[i];
			if (d >= 0xDC00 && d <= 0xDFFF)
			{
				// Have valid second byte of a surrogate pair
				++surrogate_pairs;
				++i;
			}
		}
	}
	return utf16_len - surrogate_pairs;
}

// Length in utf16string (UTF-16) of wlen wchars beginning at woffset.
S32 wstring_utf16_length(const LLWString& wstr, S32 woffset, S32 wlen)
{
	const S32 end = llmin((S32)wstr.length(), woffset + wlen);
	if (end < woffset)
	{
		return 0;
	}

	S32 length = end - woffset;
	for (S32 i = woffset; i < end; ++i)
	{
		if (wstr[i] >= 0x10000)
		{
			++length;
		}
	}
	return length;
}

// Given a wstring and an offset in it, returns the length as wstring (i.e.,
// number of llwchars) of the longest substring that starts at the offset
// and whose equivalent utf-16 string does not exceeds the given utf16_length.
S32 wstring_length_from_utf16_length(const LLWString& wstr, S32 woffset,
									 S32 utf16_length, bool* unaligned)
{
	const S32 end = wstr.length();
	bool u = false;
	S32 n = woffset + utf16_length;
	S32 i = woffset;
	while (i < end)
	{
		if (wstr[i] >= 0x10000)
		{
			--n;
		}
		if (i >= n)
		{
			u = (i > n);
			break;
		}
		++i;
	}
	if (unaligned)
	{
		*unaligned = u;
	}
	return i - woffset;
}

S32 wchar_utf8_length(const llwchar wc)
{
	if (wc < 0x80)
	{
		// This case will also catch negative values which are
		// technically invalid.
		return 1;
	}
	if (wc < 0x800)
	{
		return 2;
	}
	if (wc < 0x10000)
	{
		return 3;
	}
	if (wc < 0x200000)
	{
		return 4;
	}
	if (wc < 0x4000000)
	{
		return 5;
	}
	return 6;
}

S32 wstring_utf8_length(const LLWString& wstr)
{
	S32 len = 0;
	for (S32 i = 0, count = wstr.length(); i < count; ++i)
	{
		len += wchar_utf8_length(wstr[i]);
	}
	return len;
}

LLWString utf8str_to_wstring(const std::string& utf8str, S32 len)
{
	LLWString wout;
	wout.reserve(len);
	S32 max_len = utf8str.length();
	S32 i = 0;
	while (i < len)
	{
		llwchar unichar;
		U8 cur_char = utf8str[i];

		if (cur_char < 0x80)
		{
			// Ascii character, just add it
			unichar = cur_char;
		}
		else
		{
			S32 cont_bytes = 0;
			if ((cur_char >> 5) == 0x6)			// Two byte UTF8 -> 1 UTF32
			{
				unichar = 0x1F & cur_char;
				cont_bytes = 1;
			}
			else if ((cur_char >> 4) == 0xe)	// Three byte UTF8 -> 1 UTF32
			{
				unichar = 0x0F & cur_char;
				cont_bytes = 2;
			}
			else if ((cur_char >> 3) == 0x1e)	// Four byte UTF8 -> 1 UTF32
			{
				unichar = 0x07 & cur_char;
				cont_bytes = 3;
			}
			else if ((cur_char >> 2) == 0x3e)	// Five byte UTF8 -> 1 UTF32
			{
				unichar = 0x03 & cur_char;
				cont_bytes = 4;
			}
			else if ((cur_char >> 1) == 0x7e)	// Six byte UTF8 -> 1 UTF32
			{
				unichar = 0x01 & cur_char;
				cont_bytes = 5;
			}
			else
			{
				wout += LL_UNKNOWN_CHAR;
				++i;
				continue;
			}

			// Check that this character doesn't go past the end of the string
			S32 end = len < i + cont_bytes ? len : i + cont_bytes;
			do
			{
				if (++i >= max_len)
				{
					// Malformed sequence - roll back to look at this as a new
					// char
					unichar = LL_UNKNOWN_CHAR;
					--i;
					break;
				}

				cur_char = utf8str[i];
				if ((cur_char >> 6) == 0x2)
				{
					unichar <<= 6;
					unichar += 0x3F & cur_char;
				}
				else
				{
					// Malformed sequence - roll back to look at this as a new
					// char
					unichar = LL_UNKNOWN_CHAR;
					--i;
					break;
				}
			}
			while (i < end);

			// Handle overlong characters and NULL characters
			if ((cont_bytes == 1 && unichar < 0x80) ||
				(cont_bytes == 2 && unichar < 0x800) ||
				(cont_bytes == 3 && unichar < 0x10000) ||
				(cont_bytes == 4 && unichar < 0x200000) ||
				(cont_bytes == 5 && unichar < 0x4000000))
			{
				unichar = LL_UNKNOWN_CHAR;
			}
		}

		wout += unichar;
		++i;
	}
	return wout;
}

std::string wstring_to_utf8str(const LLWString& utf32str, S32 len)
{
	std::string out;
	out.reserve(len);
	S32 i = 0;
	while (i < len)
	{
		char tchars[8];
		S32 n = wchar_to_utf8chars(utf32str[i++], tchars);
		tchars[n] = 0;
		out += tchars;
	}
	return out;
}

std::string utf8str_trim(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::trim(wstr);
	return wstring_to_utf8str(wstr);
}

std::string utf8str_tolower(const std::string& utf8str)
{
	LLWString out_str = utf8str_to_wstring(utf8str);
	LLWStringUtil::toLower(out_str);
	return wstring_to_utf8str(out_str);
}

S32 utf8str_compare_insensitive(const std::string& lhs, const std::string& rhs)
{
	LLWString wlhs = utf8str_to_wstring(lhs);
	LLWString wrhs = utf8str_to_wstring(rhs);
	return LLWStringUtil::compareInsensitive(wlhs, wrhs);
}

std::string utf8str_truncate(const std::string& utf8str, S32 max_len)
{
	if (!max_len)
	{
		return std::string();
	}
	if ((S32)utf8str.length() <= max_len)
	{
		return utf8str;
	}

	S32 cur_char = max_len;

	// If we are ASCII, we do not need to do anything
	if ((U8)utf8str[cur_char] > 0x7f)
	{
		// If first two bits are (10), it is the tail end of a multibyte char.
		// We need to shift back to the first character
		while ((0xc0 & utf8str[cur_char]) == 0x80)
		{
			// Keep moving forward until we hit the first char;
			if (--cur_char == 0)
			{
				// Make sure we do not trash memory if we've got a bogus
				// string.
				break;
			}
		}
	}
	// The byte index we are on is one we want to get rid of, so we only want
	// to copy up to (cur_char-1) chars
	return utf8str.substr(0, cur_char);
}

std::string utf8str_substChar(const std::string& utf8str,
							  const llwchar target_char,
							  const llwchar replace_char)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::replaceChar(wstr, target_char, replace_char);
	//wstr = wstring_substChar(wstr, target_char, replace_char);
	return wstring_to_utf8str(wstr);
}

std::string utf8str_makeASCII(const std::string& utf8str)
{
	LLWString wstr = utf8str_to_wstring(utf8str);
	LLWStringUtil::_makeASCII(wstr);
	return wstring_to_utf8str(wstr);
}

std::string mbcsstring_makeASCII(const std::string& wstr)
{
	// Replace non-ASCII chars with replace_char
	std::string out_str = wstr;
	for (S32 i = 0, len = out_str.length(); i < len; ++i)
	{
		if ((U8)out_str[i] > 0x7f)
		{
			out_str[i] = LL_UNKNOWN_CHAR;
		}
	}
	return out_str;
}

std::string utf8str_removeCRLF(const std::string& utf8str)
{
	std::string out;

	size_t len = utf8str.length();
	if (!len) return out;

	out.reserve(len);

	for (size_t i = 0; i < len; ++i)
	{
		unsigned char c = utf8str[i];
		if (c != 13)
		{
			out.push_back(c);
		}
	}

	return out;
}

std::string iso8859_to_utf8(const std::string& iso8859str)
{
	std::string out;

	size_t len = iso8859str.length();
	if (!len) return out;

	out.reserve(2 * len);

	for (size_t i = 0; i < len; ++i)
	{
		unsigned char c = iso8859str[i];
		if (c < 128)
		{
			out.push_back(c);
		}
		else
		{
			out.push_back(0xc2 + (c > 0xbf));
			out.push_back(0x80 + (c & 0x3f));
		}
	}

	return out;
}

std::string utf8_to_iso8859(const std::string& utf8str)
{
	std::string out;

	size_t len = utf8str.length();
	if (!len) return out;

	out.reserve(len);

	for (size_t i = 0; i < len; ++i)
	{
		unsigned char c = utf8str[i];
		if (c < 128)
		{
			out.push_back(c);
		}
		else if (i < len - 1)
		{
			out.push_back(((c & 0x1f) << 6) + (utf8str[++i] & 0x3f));
		}
	}

	return out;
}

#if LL_WINDOWS

std::string ll_convert_wide_to_string(const wchar_t* in,
									  unsigned int code_page)
{
	std::string out;
	if (in)
	{
		int len_in = wcslen(in);
		int len_out = WideCharToMultiByte(code_page, 0, in, len_in, NULL, 0, 0,
										  0);
		// We will need two more bytes for the double NULL ending created in
		// WideCharToMultiByte().
		char* pout = new char[len_out + 2];
		memset(pout, 0, len_out + 2);
		if (pout)
		{
			WideCharToMultiByte(code_page, 0, in, len_in, pout, len_out, 0, 0);
			out.assign(pout);
			delete[] pout;
		}
	}
	return out;
}

std::string ll_convert_wide_to_string(const wchar_t* in)
{
	return ll_convert_wide_to_string(in, CP_UTF8);
}

LLWString ll_convert_wide_to_wstring(const std::wstring& in)
{
	// This function, like its converse, is a placeholder, encapsulating a
	// guilty little hack: the only "official" way Nat has found to convert
	// between std::wstring (16 bits on Windows) and LLWString (UTF-32) is
	// by using iconv, which we have avoided so far. It sorts of works to just
	// copy individual characters...
	// The point is that if/when we DO introduce some more official way to
	// perform such conversions, we should only have to call it here.
	return { in.begin(), in.end() };
}

std::wstring ll_convert_wstring_to_wide(const LLWString& in)
{
	// See comments in ll_convert_wide_to_wstring()
	return { in.begin(), in.end() };
}

std::basic_string<wchar_t> ll_convert_string_to_wide(const std::string& in)
{
	return ll_convert_string_to_wide(in, CP_UTF8);
}

std::basic_string<wchar_t> ll_convert_string_to_wide(const std::string& in,
													 unsigned int code_page)
{
	// From review:
	// We can preallocate a wide char buffer that is the same length (in
	// wchar_t elements) as the utf8 input, plus one for a nul terminator, and
	// be guaranteed to not overflow.

	// Normally, I would call that sort of thing premature optimization, but we
	// *are* seeing string operations taking a bunch of time, especially when
	// constructing widgets.
	//int output_str_len = MultiByteToWideChar(code_page, 0, in.c_str(),
	//										 in.length(), NULL, 0);

	// Reserve an output buffer that will be destroyed on exit, with a place to
	// put a NUL terminator.
	std::vector<wchar_t> w_out(in.length() + 1);

	size_t len = w_out.size();
	memset(&w_out[0], 0, len);
	int real_output_str_len = MultiByteToWideChar(code_page, 0, in.c_str(),
												  in.length(), &w_out[0], len);

	// Looks like MultiByteToWideChar didn't add null terminator to converted
	// string, see EXT-4858.
	w_out[real_output_str_len] = 0;

	// Construct string<wchar_t> from our temporary output buffer
	return {&w_out[0]};
}

std::string ll_convert_string_to_utf8_string(const std::string& in)
{
	auto w_mesg = ll_convert_string_to_wide(in, CP_ACP);
	std::string out_utf8(ll_convert_wide_to_string(w_mesg.c_str(), CP_UTF8));
	return out_utf8;
}

#endif // LL_WINDOWS

///////////////////////////////////////////////////////////////////////////////
// Formerly in u64.cpp - Utilities for conversions between U64 and string
///////////////////////////////////////////////////////////////////////////////

U64 str_to_U64(const std::string& str)
{
	U64 result = 0;
	const char* aptr = strpbrk(str.c_str(), "0123456789");

	if (!aptr)
	{
		llwarns << "Bad string to U64 conversion attempt: format" << llendl;
	}
	else
	{
		while (*aptr >= '0' && *aptr <= '9')
		{
			result = result * 10 + (*aptr++ - '0');
		}
	}
	return result;
}

std::string U64_to_str(U64 value)
{
	std::string res;
	U32 part1, part2, part3;

	part3 = (U32)(value % (U64)10000000);

	value /= 10000000;
	part2 = (U32)(value % (U64)10000000);

	value /= 10000000;
	part1 = (U32)(value % (U64)10000000);

	// Three cases to avoid leading zeroes unless necessary
	if (part1)
	{
		res = llformat("%u%07u%07u", part1, part2, part3);
	}
	else if (part2)
	{
		res = llformat("%u%07u", part2, part3);
	}
	else
	{
		res = llformat("%u", part3);
	}
	return res;
}

char* U64_to_str(U64 value, char* result, S32 result_size)
{
	std::string res = U64_to_str(value);
	LLStringUtil::copy(result, res.c_str(), result_size);
	return result;
}

U64	llstrtou64(const char* str, char** end, S32 base)
{
#ifdef LL_WINDOWS
	return _strtoui64(str, end, base);
#else
	return strtoull(str, end, base);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// LLStringOps class
///////////////////////////////////////////////////////////////////////////////

long LLStringOps::sPacificTimeOffset = 0;
long LLStringOps::sLocalTimeOffset = 0;
bool LLStringOps::sPacificDaylightTime = 0;
strings_map_t LLStringOps::sDatetimeToCodes;

std::vector<std::string> LLStringOps::sWeekDayList;
std::vector<std::string> LLStringOps::sWeekDayShortList;
std::vector<std::string> LLStringOps::sMonthList;
std::vector<std::string> LLStringOps::sMonthShortList;

std::string LLStringOps::sDayFormat;
std::string LLStringOps::sAM;
std::string LLStringOps::sPM;

//static
bool LLStringOps::isHexString(const std::string& str)
{
	const char* buf = str.c_str();
	int len = str.size();
	while (--len >= 0)
	{
		if (!isxdigit(buf[len])) return false;
	}

	return true;
}

//static
bool LLStringOps::isEmoji(llwchar a)
{
#if 0	// Do not consider special characters that might have a corresponding
		// glyph in the monochorme fallback fonts as a "genuine" emoji. HB
	return a == 0xa9 || a == 0xae || (a >= 0x2000 && a < 0x3300) ||
		   (a >= 0x1f000 && a < 0x20000);
#else
	// These are indeed "genuine" emojis. HB
	return a >= 0x1f000 && a < 0x20000;
#endif
}

//static
S32	LLStringOps::collate(const llwchar* a, const llwchar* b)
{
#if LL_WINDOWS
	// Under Windows, wide string functions operator on 16-bit strings, not the
	// proper 32 bit wide string.
	return strcmp(wstring_to_utf8str(LLWString(a)).c_str(),
				  wstring_to_utf8str(LLWString(b)).c_str());
#else
	return wcscoll(a, b);
#endif
}

//static
void LLStringOps::setupDatetimeInfo(bool daylight)
{
	time_t nowT, localT, gmtT;
	struct tm * tmpT;

	nowT = time (NULL);

	tmpT = gmtime(&nowT);
	gmtT = mktime (tmpT);

	tmpT = localtime (&nowT);
	localT = mktime (tmpT);

	sLocalTimeOffset = (long) (gmtT - localT);
	if (tmpT->tm_isdst)
	{
		sLocalTimeOffset -= 60 * 60;	// 1 hour
	}

	sPacificDaylightTime = daylight;
	sPacificTimeOffset = (sPacificDaylightTime? 7 : 8) * 60 * 60;

	sDatetimeToCodes["wkday"]	= "%a";		// Thu
	sDatetimeToCodes["weekday"]	= "%A";		// Thursday
	sDatetimeToCodes["year4"]	= "%Y";		// 2009
	sDatetimeToCodes["year"]		= "%Y";		// 2009
	sDatetimeToCodes["year2"]	= "%y";		// 09
	sDatetimeToCodes["mth"]		= "%b";		// Aug
	sDatetimeToCodes["month"]	= "%B";		// August
	sDatetimeToCodes["mthnum"]	= "%m";		// 08
	sDatetimeToCodes["day"]		= "%d";		// 31
	sDatetimeToCodes["sday"]		= "%-d";	// 9
	sDatetimeToCodes["hour24"]	= "%H";		// 14
	sDatetimeToCodes["hour"]		= "%H";		// 14
	sDatetimeToCodes["hour12"]	= "%I";		// 02
	sDatetimeToCodes["min"]		= "%M";		// 59
	sDatetimeToCodes["ampm"]		= "%p";		// AM
	sDatetimeToCodes["second"]	= "%S";		// 59
	sDatetimeToCodes["timezone"]	= "%Z";		// PST
}

static void tokenize_str_to_aray(const std::string& data,
								 std::vector<std::string>& output)
{
	output.clear();
	size_t length = data.size();

	// Tokenize it and put it in the array
	std::string cur_word;
	for (size_t i = 0; i < length; ++i)
	{
		if (data[i] == ':')
		{
			output.push_back(cur_word);
			cur_word.clear();
		}
		else
		{
			cur_word.append(1, data[i]);
		}
	}
	output.push_back(cur_word);
}

//static
void LLStringOps::setupWeekDaysNames(const std::string& data)
{
	tokenize_str_to_aray(data, sWeekDayList);
}

//static
void LLStringOps::setupWeekDaysShortNames(const std::string& data)
{
	tokenize_str_to_aray(data, sWeekDayShortList);
}

//static
void LLStringOps::setupMonthNames(const std::string& data)
{
	tokenize_str_to_aray(data, sMonthList);
}

//static
void LLStringOps::setupMonthShortNames(const std::string& data)
{
	tokenize_str_to_aray(data, sMonthShortList);
}

//static
void LLStringOps::setupDayFormat(const std::string& data)
{
	sDayFormat = data;
}

//static
const std::string& LLStringOps::getDatetimeCode(const std::string& key)
{
	strings_map_t::iterator iter = sDatetimeToCodes.find(key);
	return iter != sDatetimeToCodes.end() ? iter->second : LLStringUtil::null;
}

namespace LLStringFn
{
	// Note: this restricts output to ASCII
	void replace_nonprintable_in_ascii(std::basic_string<char>& str,
									   char replacement)
	{
		constexpr char SPACE = 0x20;
		for (size_t i = 0, len = str.size(); i < len; ++i)
		{
			if (str[i] < SPACE)
			{
				str[i] = replacement;
			}
		}
	}


	// Note: this restricts output to ASCII
	void replace_nonprintable_and_pipe_in_ascii(std::basic_string<char>& str,
												char replacement)
	{
		constexpr char SPACE = 0x20;
		constexpr char PIPE = 0x7c;
		for (size_t i = 0, len = str.size(); i < len; ++i)
		{
			if (str[i] < SPACE || str[i] == PIPE)
			{
				str[i] = replacement;
			}
		}
	}

	// Replaces all control characters (c < 0x20) with replacement in string.
	void replace_ascii_controlchars(std::basic_string<char>& str,
									char replacement)
	{
		constexpr unsigned char SPACE = 0x20;
		for (size_t i = 0, len = str.size(); i < len; ++i)
		{
			const unsigned char c = (unsigned char)str[i];
			if (c < SPACE)
			{
				str[i] = replacement;
			}
		}
	}

	// https://wiki.lindenlab.com/wiki/Unicode_Guidelines has details on
	// allowable code points for XML. Specifically, they are:
	// 0x09, 0x0a, 0x0d, and 0x20 on up.  JC
	std::string strip_invalid_xml(const std::string& str)
	{
		constexpr unsigned char SPACE = 0x20;
		constexpr unsigned char TAB = 0x09;
		constexpr unsigned char LF = 0x0a;
		constexpr unsigned char CR = 0x0d;
		std::string output;
		output.reserve(str.size());
		std::string::const_iterator it = str.begin();
		while (it != str.end())
		{
			// Must compare as unsigned for >=
			// Test most likely match first
			const unsigned char c = (unsigned char)*it;
			if (c >= SPACE || c == TAB || c == LF || c == CR)
			{
				output.push_back(c);
			}
			++it;
		}
		return output;
	}

	typedef std::map<char, std::string> literals_map_t;
	static const literals_map_t xml_elem_literals =
	{
		{ '<', "&lt;" },
		{ '>', "&gt;" },
		{ '&', "&amp;" }
	};
	static const literals_map_t xml_attr_literals =
	{
		{ '"', "&quot;" },
		{ '\'', "&apos;" }
	};

	static void literals_encode(std::string& text,
								const literals_map_t& literals)
	{
		for (literals_map_t::const_iterator it = literals.begin(),
											end = literals.end();
			 it != end; ++it)
		{
			size_t pos = 0;
			while ((pos = text.find(it->first, pos)) != std::string::npos)
			{
				text.replace(pos, 1, it->second);
				pos += it->second.size();
			}
		}
	}

	// Replaces all characters that are not allowed in XML 1.0 with the
	// corresponding literals.
	std::string xml_encode(const std::string& input, bool for_attribute)
	{
		std::string result(input);
		literals_encode(result, xml_elem_literals);
		if (for_attribute)
		{
			literals_encode(result, xml_attr_literals);
		}
		return result;
	}

	static void literals_decode(std::string& text,
								const literals_map_t& literals)
	{
		for (literals_map_t::const_iterator it = literals.begin(),
											end = literals.end();
			 it != end; ++it)
		{
			size_t pos = 0;
			while ((pos = text.find(it->second, pos)) != std::string::npos)
			{
				text[pos++] = it->first;
				text.erase(pos, it->second.size() - 1);
			}
		}
	}

	// Replaces some of XML literals that are defined in XML 1.0 with the
	// corresponding characters.
	std::string xml_decode(const std::string& input, bool for_attribute)
	{
		std::string result(input);
		literals_decode(result, xml_elem_literals);
		if (for_attribute)
		{
			literals_decode(result, xml_attr_literals);
		}
		return result;
	}
}

////////////////////////////////////////////////////////////

// Forward specialization of LLStringUtil::format before use in
// LLStringUtil::formatDatetime.
template<>
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions);

//static
template<>
void LLStringUtil::getTokens(const std::string& instr,
							 std::vector<std::string>& tokens,
							 const std::string& delims)
{
	std::string token;
	size_t start = instr.find_first_not_of(delims);
	while (start != std::string::npos)
	{
		size_t end = instr.find_first_of(delims, start);
		if (end == std::string::npos)
		{
			end = instr.length();
		}

		token = instr.substr(start, end - start);
		LLStringUtil::trim(token);
		tokens.push_back(token);
		start = instr.find_first_not_of(delims, end);
	}
}

//static
template<>
LLStringUtil::size_type LLStringUtil::getSubstitution(const std::string& instr,
													  size_type& start,
													  std::vector<std::string>& tokens)
{
	static const std::string delims(",");

	// Find the first [
	size_type pos1 = instr.find('[', start);
	if (pos1 == std::string::npos)
		return pos1;

	// Find the first ] after the initial [
	size_type pos2 = instr.find(']', pos1);
	if (pos2 == std::string::npos)
		return pos2;

	// Find the last [ before ] in case of nested [[]]
	pos1 = instr.find_last_of('[', pos2 - 1);
	if (pos1 == std::string::npos || pos1 < start)
		return std::string::npos;

	getTokens(std::string(instr ,pos1 + 1, pos2 - pos1 - 1), tokens, delims);
	start = pos2 + 1;

	return pos1;
}

//static
template<>
bool LLStringUtil::simpleReplacement(std::string& replacement,
									 const std::string& token,
									 const format_map_t& substitutions)
{
	// See if we have a replacement for the bracketed string (without the
	// brackets) test first using has() because if we just look up with
	// operator[] we get back an empty string even if the value is missing.
	// We want to distinguish between missing replacements and deliberately
	// empty replacement strings.
	format_map_t::const_iterator iter = substitutions.find(token);
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}
	// If not, see if there's one WITH brackets
	iter = substitutions.find(std::string("[" + token + "]"));
	if (iter != substitutions.end())
	{
		replacement = iter->second;
		return true;
	}

	return false;
}

//static
template<>
bool LLStringUtil::simpleReplacement(std::string& replacement,
									 const std::string& token,
									 const LLSD& substitutions)
{
	// See if we have a replacement for the bracketed string (without the
	// brackets). Test first using has() because if we just look up with
	// operator[] we get back an empty string even if the value is missing.
	// We want to distinguish between missing replacements and deliberately
	// empty replacement strings.
	if (substitutions.has(token))
	{
		replacement = substitutions[token].asString();
		return true;
	}
	// If not, see if there's one WITH brackets
	else if (substitutions.has(std::string("[" + token + "]")))
	{
		replacement = substitutions[std::string("[" + token + "]")].asString();
		return true;
	}

	return false;
}

//static
template<>
void LLStringUtil::setLocale(std::string in_locale)
{
	sLocale = in_locale;
}

//static
template<>
std::string LLStringUtil::getLocale()
{
	return sLocale;
}

//static
template<>
void LLStringUtil::formatNumber(std::string& num_str, S32 decimals)
{
	std::stringstream str_stream;
	if (!sLocale.empty())
	{
		// std::locale() throws if the locale is unknown ! (EXT-7926)
		try
		{
			str_stream.imbue(std::locale(sLocale.c_str()));
		}
		catch (const std::exception&)
		{
			llwarns_once << "Cannot set locale to " << sLocale << llendl;
		}
	}

	if (!decimals)
	{
		S32 int_str;
		if (convertToS32(num_str, int_str))
		{
			str_stream << int_str;
			num_str = str_stream.str();
		}
	}
	else
	{
		F32 float_str;
		if (convertToF32(num_str, float_str))
		{
			str_stream << std::fixed << std::showpoint
					   << std::setprecision(decimals) << float_str;
			num_str = str_stream.str();
		}
	}
}

//static
template<>
bool LLStringUtil::formatDatetime(std::string& replacement,
								  const std::string& token,
								  const std::string& param, S32 sec_epoch)
{
	if (param == "local")   // Local time
	{
		sec_epoch -= LLStringOps::getLocalTimeOffset();
	}
	else if (param != "utc" && param != "gmt") // SL time
	{
		sec_epoch -= LLStringOps::getPacificTimeOffset();
	}

	// If never fell into those two ifs above, param must be utc
	if (sec_epoch < 0) sec_epoch = 0;

	LLDate datetime((F64)sec_epoch);
	std::string code = LLStringOps::getDatetimeCode(token);
	// Special case to handle timezone
	if (code == "%Z")
	{
		if (param == "utc" || param == "gmt")
		{
			replacement = "UTC";
		}
		else if (param == "local")
		{
			replacement.clear();	// User knows their own timezone
		}
		else
		{
			// "slt" = Second Life Time, which is deprecated.
			// If not UTC or user local time, fallback to pacific time
			replacement = LLStringOps::getPacificDaylightTime() ? "PDT"
																: "PST";
		}
		return true;
	}

	// EXT-7013: few codes are not suppotred by strtime function (example:
	// weekdays for Japanese), so use predefined ones.

	// If sWeekDayList is not empty than current locale does not support the
	// weekday name.
	time_t loc_seconds = (time_t) sec_epoch;
	if (LLStringOps::sWeekDayList.size() == 7 && code == "%A")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		replacement = LLStringOps::sWeekDayList[gmt->tm_wday];
	}
	else if (LLStringOps::sWeekDayShortList.size() == 7 && code == "%a")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		replacement = LLStringOps::sWeekDayShortList[gmt->tm_wday];
	}
	else if (LLStringOps::sMonthList.size() == 12 && code == "%B")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		replacement = LLStringOps::sMonthList[gmt->tm_mon];
	}
	else if (!LLStringOps::sDayFormat.empty() && code == "%d")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		LLStringUtil::format_map_t args;
		args["[MDAY]"] = llformat ("%d", gmt->tm_mday);
		replacement = LLStringOps::sDayFormat;
		LLStringUtil::format(replacement, args);
	}
	else if (code == "%-d")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		// Day of the month without leading zero
		replacement = llformat ("%d", gmt->tm_mday);
	}
	else if (!LLStringOps::sAM.empty() && !LLStringOps::sPM.empty() &&
			 code == "%p")
	{
		struct tm* gmt = gmtime(&loc_seconds);
		if (gmt->tm_hour<12)
		{
			replacement = LLStringOps::sAM;
		}
		else
		{
			replacement = LLStringOps::sPM;
		}
	}
	else
	{
		replacement = datetime.toHTTPDateString(code.c_str());
	}

	// *HACK: delete leading zero from hour string in case 'hour12' (code = %I)
	// time format to show time without leading zero, e.g. 08:16 -> 8:16
	// (EXT-2738). We could have used '%l' format instead, but it is not
	// supported by Windows.
	if (code == "%I" && token == "hour12" && replacement[0] == '0')
	{
		replacement = replacement[1];
	}

	return !code.empty();
}

// LLStringUtil::format recogizes the following patterns.
// All substitutions *must* be encased in []'s in the input string. The []'s
// are optional in the substitution map.
// [FOO_123]
// [FOO,number,precision]
// [FOO,datetime,format]

//static
template<>
S32 LLStringUtil::format(std::string& s, const format_map_t& substitutions)
{
	S32 res = 0;

	std::string output;
	// Reserve space to avoid reallocations. HB
	output.reserve(4 * s.size() / 3);
	std::vector<std::string> tokens;
	// Reserve space to avoid reallocations. HB
	tokens.reserve(substitutions.size());

	size_t start = 0;
	size_t prev_start = 0;
	size_t key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start - prev_start);
		prev_start = start;

		bool found_replacement = false;
		std::string replacement;

		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement(replacement, tokens[0],
												  substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";
			if (tokens.size() > 2)
			{
				param = tokens[2];
			}
			found_replacement = simpleReplacement(replacement, tokens[0],
												  substitutions);
			if (found_replacement)
			{
				formatNumber(replacement, atoi(param.c_str()));
			}
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2)
			{
				param = tokens[2];
			}

			format_map_t::const_iterator iter = substitutions.find("datetime");
			if (iter != substitutions.end())
			{
				S32 sec_epoch = 0;
				bool r = LLStringUtil::convertToS32(iter->second,
													sec_epoch);
				if (r)
				{
					found_replacement = formatDatetime(replacement, tokens[0],
													   param, sec_epoch);
				}
			}
		}

		if (found_replacement)
		{
			output += replacement;
			++res;
		}
		else
		{
			// We had no replacement, use the string as is. E.g.
			// "hello [MISSING_REPLACEMENT]" or "-=[Stylized Name]=-"
			output += std::string(s, key_start, start - key_start);
		}
		tokens.clear();
	}
	// Send the remainder of the string (with no further matches for bracketed
	// names)
	output += std::string(s, start);
	s = output;
	return res;
}

//static
template<>
S32 LLStringUtil::format(std::string& s, const LLSD& substitutions)
{
	S32 res = 0;

	if (!substitutions.isMap())
	{
		return res;
	}

	std::string output;
	// Reserve space to avoid reallocations. HB
	output.reserve(4 * s.size() / 3);
	std::vector<std::string> tokens;
	// Reserve space to avoid reallocations. HB
	tokens.reserve(substitutions.size());

	size_t start = 0;
	size_t prev_start = 0;
	size_t key_start = 0;
	while ((key_start = getSubstitution(s, start, tokens)) != std::string::npos)
	{
		output += std::string(s, prev_start, key_start - prev_start);
		prev_start = start;

		bool found_replacement = false;
		std::string replacement;

		if (tokens.size() == 0)
		{
			found_replacement = false;
		}
		else if (tokens.size() == 1)
		{
			found_replacement = simpleReplacement(replacement, tokens[0],
												  substitutions);
		}
		else if (tokens[1] == "number")
		{
			std::string param = "0";
			if (tokens.size() > 2)
			{
				param = tokens[2];
			}
			found_replacement = simpleReplacement(replacement, tokens[0],
												  substitutions);
			if (found_replacement)
			{
				formatNumber(replacement, atoi(param.c_str()));
			}
		}
		else if (tokens[1] == "datetime")
		{
			std::string param;
			if (tokens.size() > 2)
			{
				param = tokens[2];
			}

			S32 sec_epoch = (S32)substitutions["datetime"].asInteger();
			found_replacement = formatDatetime(replacement, tokens[0],
											   param, sec_epoch);
		}

		if (found_replacement)
		{
			output += replacement;
			++res;
		}
		else
		{
			// We had no replacement, use the string as is. E.g.
			// "hello [MISSING_REPLACEMENT]" or "-=[Stylized Name]=-"
			output += std::string(s, key_start, start-key_start);
		}
		tokens.clear();
	}
	// Send the remainder of the string (with no further matches for bracketed
	// names)
	output += std::string(s, start);
	s = output;
	return res;
}

// This used to be in separate llformat.cpp file. Moved here for coherency. HB
// Note: uses an internal buffer limited to 1024 characters (but vsnprintf
// prevents any overrun).
std::string llformat(const char* fmt, ...)
{
	// Avoid allocating 1024 bytes on the stack (or worst, depending on the
	// compiler: on the heap) at *each* call; instead use a static buffer in
	// the thread local storage (so that we stay thread-safe). HB
	thread_local char buffer[1024];

	if (LL_UNLIKELY(!fmt))
	{
		return std::string();
	}

	va_list va;
	va_start(va, fmt);
#if LL_WINDOWS
	_vsnprintf(buffer, 1024, fmt, va);
#else
	vsnprintf(buffer, 1024, fmt, va);
#endif
	va_end(va);

	return std::string(buffer);
}
