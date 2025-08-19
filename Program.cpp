/*
MIT License

Copyright (c) 2025 Ben Kurtin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "Program.h"

#include <cstdlib>
#include <cwchar>

#if MK_IS_PLATFORM_WINDOWS
#include <intrin.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#endif

namespace
{
	constexpr size_t kBufferSize = 4096;
	wchar_t ErrorBuffer[kBufferSize] = { 0 };
}

namespace Program
{
	[[noreturn]]
	void Panic()
	{
		#if MK_IS_PLATFORM_WINDOWS
		MessageBoxW(nullptr, ErrorBuffer, MK_PROGRAM_NAME_WSTR L": Error", MB_ICONERROR | MB_OK | MB_DEFBUTTON1);
		#endif

		if constexpr (kIsDebug)
		{
			#if MK_IS_PLATFORM_WINDOWS
			__debugbreak();
			#endif
			std::abort();
		}
		else
			std::abort();
	}

	void Assert(const bool statement, const wchar_t* func, const wchar_t* file, const wchar_t* line, const wchar_t* message)
	{
		if (!statement)
		{
			swprintf_s(ErrorBuffer, L"In %s, at line %s of %s: %s", func, line, file, message);
			Panic();
		}
	}

	void Assert(const bool statement, const char* func, const char* file, const char* line, const char* message)
	{
		if (!statement)
		{
			swprintf_s(ErrorBuffer, L"In %S, at line %S of %S: %S", func, line, file, message);
			Panic();
		}
	}


}
