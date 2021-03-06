/*
 * Copyright (C) 2013-2015 Max Kellermann <max.kellermann@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SYSTEM_ERROR_HXX
#define SYSTEM_ERROR_HXX

#include "util/StringUtil.hxx"
#include "Compiler.h"

#include <system_error>
#include <utility>

#include <stdio.h>

template<typename... Args>
static inline std::system_error
FormatSystemError(std::error_code code, const char *fmt, Args&&... args)
{
	char buffer[1024];
	snprintf(buffer, sizeof(buffer), fmt, std::forward<Args>(args)...);
	return std::system_error(code, buffer);
}

#ifdef WIN32

#include <windows.h>

static inline std::system_error
MakeLastError(DWORD code, const char *msg)
{
	return std::system_error(std::error_code(code, std::system_category()),
				 msg);
}

static inline std::system_error
MakeLastError(const char *msg)
{
	return MakeLastError(GetLastError(), msg);
}

template<typename... Args>
static inline std::system_error
FormatLastError(DWORD code, const char *fmt, Args&&... args)
{
	char buffer[512];
	const auto end = buffer + sizeof(buffer);
	size_t length = snprintf(buffer, sizeof(buffer) - 128,
				 fmt, std::forward<Args>(args)...);
	char *p = buffer + length;
	*p++ = ':';
	*p++ = ' ';

	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
		       FORMAT_MESSAGE_IGNORE_INSERTS,
		       nullptr, code, 0, p, end - p, nullptr);
	return MakeLastError(code, buffer);
}

template<typename... Args>
static inline std::system_error
FormatLastError(const char *fmt, Args&&... args)
{
	return FormatLastError(GetLastError(), fmt,
			       std::forward<Args>(args)...);
}

#endif /* WIN32 */

#include <errno.h>
#include <string.h>

/**
 * Returns the error_category to be used to wrap errno values.  The
 * C++ standard does not define this well, so this code is based on
 * observations what C++ standard library implementations actually
 * use.
 *
 * @see https://stackoverflow.com/questions/28746372/system-error-categories-and-standard-system-error-codes
 */
static inline const std::error_category &
ErrnoCategory()
{
#ifdef WIN32
	/* on Windows, the generic_category() is used for errno
	   values */
	return std::generic_category();
#else
	/* on POSIX, system_category() appears to be the best
	   choice */
	return std::system_category();
#endif
}

static inline std::system_error
MakeErrno(int code, const char *msg)
{
	return std::system_error(std::error_code(code, ErrnoCategory()),
				 msg);
}

static inline std::system_error
MakeErrno(const char *msg)
{
	return MakeErrno(errno, msg);
}

template<typename... Args>
static inline std::system_error
FormatErrno(int code, const char *fmt, Args&&... args)
{
	char buffer[512];
	snprintf(buffer, sizeof(buffer),
		 fmt, std::forward<Args>(args)...);
	return MakeErrno(code, buffer);
}

template<typename... Args>
static inline std::system_error
FormatErrno(const char *fmt, Args&&... args)
{
	return FormatErrno(errno, fmt, std::forward<Args>(args)...);
}

gcc_pure
static inline bool
IsFileNotFound(const std::system_error &e) noexcept
{
#ifdef WIN32
	return e.code().category() == std::system_category() &&
		e.code().value() == ERROR_FILE_NOT_FOUND;
#else
	return e.code().category() == ErrnoCategory() &&
		e.code().value() == ENOENT;
#endif
}

gcc_pure
static inline bool
IsPathNotFound(const std::system_error &e) noexcept
{
#ifdef WIN32
	return e.code().category() == std::system_category() &&
		e.code().value() == ERROR_PATH_NOT_FOUND;
#else
	return e.code().category() == ErrnoCategory() &&
		e.code().value() == ENOTDIR;
#endif
}

gcc_pure
static inline bool
IsAccessDenied(const std::system_error &e) noexcept
{
#ifdef WIN32
	return e.code().category() == std::system_category() &&
		e.code().value() == ERROR_ACCESS_DENIED;
#else
	return e.code().category() == ErrnoCategory() &&
		e.code().value() == EACCES;
#endif
}

#endif
