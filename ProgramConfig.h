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

#ifndef PROGRAMCONFIG_H
#define PROGRAMCONFIG_H

// Search for CONFIGME to find where options can be configured.

// ---------------------------------------------------------------------------------------------------------------------
// Helper Macros
// ---------------------------------------------------------------------------------------------------------------------

#define CSTR_MACRO(m) CSTR(m)
#define CSTR(s) #s

#define WSTR_MACRO(m) WSTR(m)
#define WSTR(s) L###s

// ---------------------------------------------------------------------------------------------------------------------
// CONFIGME: Program Info Macros
// ---------------------------------------------------------------------------------------------------------------------

#define MK_PROGRAM_NAME VariantV4
#define MK_PROGRAM_NAME_CSTR CSTR_MACRO(MK_PROGRAM_NAME)
#define MK_PROGRAM_NAME_WSTR WSTR_MACRO(MK_PROGRAM_NAME)

#define MK_IS_DEBUG   CMAKE_BUILD_TYPE == Debug
#define MK_IS_RELEASE CMAKE_BUILD_TYPE == Release

#define MK_IS_PLATFORM_WINDOWS (CMAKE_BUILD_TYPE == Win32 || CMAKE_BUILD_TYPE == Win64)
#define MK_IS_PLATFORM_MAC CMAKE_BUILD_TYPE == mac
#define MK_IS_PLATFORM_LINUX CMAKE_BUILD_TYPE == linux

#define MK_IS_PLATFORM_ANDROID CMAKE_BUILD_TYPE == Android
#define MK_IS_PLATFORM_APPLE CMAKE_BUILD_TYPE == apple
#define MK_IS_PLATFORM_IOS CMAKE_BUILD_TYPE == ios
#define MK_IS_PLATFORM_WATCHOS CMAKE_BUILD_TYPE == watchos

#define MK_IS_PLATFORM_PS CMAKE_BUILD_TYPE == ps
#define MK_IS_PLATFORM_NT CMAKE_BUILD_TYPE == nt
#define MK_IS_PLATFORM_EMSCRIPTEN CMAKE_BUILD_TYPE == em

#if MK_IS_PLATFORM_WINDOWS
#define MK_PLATFORM_NAME Windows
#else
#define MK_PLATFORM_NAME Unknown
#endif

#define MK_PLATFORM_NAME_CSTR CSTR_MACRO(MK_PLATFORM_NAME)
#define MK_PLATFORM_NAME_WSTR WSTR_MACRO(MK_PLATFORM_NAME)

#define MK_TRIM_DEBUG_INFO 0

// ---------------------------------------------------------------------------------------------------------------------
// Assert Helper Macros
// ---------------------------------------------------------------------------------------------------------------------

#define MK_LINE_CSTR CSTR_MACRO(__LINE__)
#define MK_LINE_WSTR WSTR_MACRO(__LINE__)

#define MK_FILE_CSTR __FILE__
#define MK_FILE_WSTR WSTR_MACRO(__FILE__)

#define MK_FILENAME_CSTR __FILE_NAME__
#define MK_FILENAME_WSTR WSTR_MACRO(__FILE_NAME__)

#define MK_FUNCTION_CSTR __FUNCTION__
#define MK_FUNCTION_WSTR WSTR_MACRO(__FUNCTION__)

#define MK_PRETTY_FUNCTION_CSTR __PRETTY_FUNCTION__
#define MK_PRETTY_FUNCTION_WSTR WSTR_MACRO(__PRETTY_FUNCTION__)

// ---------------------------------------------------------------------------------------------------------------------
// Assert Info Macros
// ---------------------------------------------------------------------------------------------------------------------

#define MK_ASSERT_INFO_LVL_OPAQUE  0
#define MK_ASSERT_INFO_LVL_REGULAR 1
#define MK_ASSERT_INFO_LVL_VERBOSE 2
#define MK_ASSERT_INFO_LVL_EXACT   3

// CONFIGME
#if MK_IS_DEBUG
#define MK_ASSERT_INFO_LVL MK_ASSERT_INFO_LVL_VERBOSE
#else
#define MK_ASSERT_INFO_LVL MK_ASSERT_INFO_LVL_REGULAR
#endif

#if MK_ASSERT_INFO_LVL == MK_ASSERT_INFO_LVL_REGULAR

#define MK_ASSERT_STATS_CSTR MK_FUNCTION_CSTR, MK_FILENAME_CSTR, MK_LINE_CSTR
#define MK_ASSERT_STATS_WSTR MK_FUNCTION_CSTR, MK_FILENAME_WSTR, MK_LINE_WSTR

#elif MK_ASSERT_INFO_LVL == MK_ASSERT_INFO_LVL_VERBOSE

#define MK_ASSERT_STATS_CSTR MK_PRETTY_FUNCTION_CSTR, MK_FILENAME_CSTR, MK_LINE_CSTR
#define MK_ASSERT_STATS_WSTR MK_PRETTY_FUNCTION_WSTR, MK_FILENAME_WSTR, MK_LINE_WSTR

#elif MK_ASSERT_INFO_LVL == MK_ASSERT_INFO_LVL_EXACT

#define MK_ASSERT_STATS_CSTR MK_PRETTY_FUNCTION_CSTR, MK_FILE_CSTR, MK_LINE_CSTR
#define MK_ASSERT_STATS_WSTR MK_PRETTY_FUNCTION_WSTR, MK_FILE_WSTR, MK_LINE_WSTR

#else // MK_ASSERT_INFO_LVL == MK_ASSERT_INFO_LVL_OPAQUE || MK_ASSERT_INFO_LVL == (anything else)

#define MK_ASSERT_STATS_CSTR "[unknown]", "[unknown]", "[unknown]"
#define MK_ASSERT_STATS_WSTR L"[unknown]", L"[unknown]", L"[unknown]"

#endif

// ---------------------------------------------------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------------------------------------------------

namespace Program
{
	static constexpr bool kIsDebug =
		#if MK_IS_DEBUG
		true
		#else
		false
		#endif
		;

	static constexpr auto kNameCStr = MK_PROGRAM_NAME_CSTR;
	static constexpr auto kNameWStr = MK_PROGRAM_NAME_WSTR;

	static constexpr auto kPlatformCStr = MK_PLATFORM_NAME_CSTR;
	static constexpr auto kPlatformWStr = MK_PLATFORM_NAME_WSTR;

	static constexpr bool kTrimDebugInfo = MK_TRIM_DEBUG_INFO;
}

#endif