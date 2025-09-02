// MIT License
//
// Copyright (c) 2025 Entropy Embracers LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "Program.hpp"

#include <cstdlib>
#include <cwchar>

#if MK_IS_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <intrin.h>
#include <Windows.h>
#else
#endif

namespace
{
	constexpr size_t kBufferSize = 4096;
	wchar_t ErrorBuffer[kBufferSize] = { 0 };

	const char* FilenameFromPath(const char* path)
	{
		const char* file_name = path + strlen(path);

		while (file_name != path && *file_name != '/')
			--file_name;
		++file_name;

		return file_name;
	}
}

namespace Program
{
	[[noreturn]]
	void Panic()
	{
		if constexpr (kIsDebug || !kTrimDebugInfo)
		{
			Log::Err(L"Panic") << ErrorBuffer << std::endl;

			#if MK_IS_PLATFORM_WINDOWS
			MessageBoxW(nullptr, ErrorBuffer, MK_PROGRAM_NAME_WSTR L": Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
			#endif
		}

		if constexpr (kIsDebug)
		{
			#if MK_IS_PLATFORM_WINDOWS
			__debugbreak();
			#endif
		}

		std::exit(-1);
	}

	void Assert(const bool statement, const wchar_t* message, const std::source_location location)
	{
		if constexpr (kIsDebug || !kTrimDebugInfo)
		{
			if (!statement)
				swprintf_s(ErrorBuffer, L"In %S, at line %u, column %u, of %S:\n%s", location.function_name(), location.line(), location.column(), FilenameFromPath(location.file_name()), message);
		}

		if (!statement)
			Panic();
	}

	void Assert(const bool statement, const char* message, const std::source_location location)
	{
		if constexpr (kIsDebug || !kTrimDebugInfo)
		{
			if (!statement)
				swprintf_s(ErrorBuffer, L"In %S, at line %u, column %u, of %S:\n%S", location.function_name(), location.line(), location.column(), FilenameFromPath(location.file_name()), message);
		}

		if (!statement)
			Panic();
	}
}
