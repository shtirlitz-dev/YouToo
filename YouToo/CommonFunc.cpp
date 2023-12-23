#include "pch.h"
#include "CommonFunc.h"

CStringW fromUtf8(std::string_view utf8)
{
	int utf16Length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
	CStringW ret;
	if (utf16Length <= 0)
		return ret;
	// Convert UTF-8 to UTF-16
	MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), ret.GetBufferSetLength(utf16Length), utf16Length);
	return ret;
}

