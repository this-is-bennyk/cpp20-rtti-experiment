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

#ifndef PROGRAMCONFIG_HPP
#define PROGRAMCONFIG_HPP

// Search for CONFIGME to find where options can be configured.

// ---------------------------------------------------------------------------------------------------------------------
// Helper Macros
// ---------------------------------------------------------------------------------------------------------------------

#define CSTR_MACRO(m) CSTR(m)
#define CSTR(s) #s

#define WSTR_MACRO(m) WSTR(m)
#define WSTR(s) L###s

// ---------------------------------------------------------------------------------------------------------------------
// Program Info Macros
// ---------------------------------------------------------------------------------------------------------------------

// CONFIGME
#define MK_PROGRAM_NAME Extropy

#define MK_PROGRAM_NAME_CSTR CSTR_MACRO(MK_PROGRAM_NAME)
#define MK_PROGRAM_NAME_WSTR WSTR_MACRO(MK_PROGRAM_NAME)

// CONFIGME
#define MK_IS_DEBUG   CMAKE_BUILD_TYPE == Debug
#define MK_IS_RELEASE CMAKE_BUILD_TYPE == Release
#include <cstdint>

#if defined(_WIN32) || defined(_WIN64)
	#define MK_IS_PLATFORM_WINDOWS   1
#else
	#define MK_IS_PLATFORM_WINDOWS   0
#endif

#if defined(__linux__)
	#define MK_IS_PLATFORM_LINUX     1
#else
	#define MK_IS_PLATFORM_LINUX     0
#endif

#if defined(__FreeBSD__)
	#define MK_IS_PLATFORM_FREEBSD   1
#else
	#define MK_IS_PLATFORM_FREEBSD   0
#endif

#if defined(__OpenBSD__)
	#define MK_IS_PLATFORM_OPENBSD   1
#else
	#define MK_IS_PLATFORM_OPENBSD   0
#endif

#if defined(__NetBSD__)
	#define MK_IS_PLATFORM_NETBSD    1
#else
	#define MK_IS_PLATFORM_NETBSD    0
#endif

#if defined(__DragonFly__)
	#define MK_IS_PLATFORM_DRAGONFLY 1
#else
	#define MK_IS_PLATFORM_DRAGONFLY 0
#endif

#define MK_IS_PLATFORM_BSD MK_IS_PLATFORM_FREEBSD || MK_IS_PLATFORM_OPENBSD || MK_IS_PLATFORM_NETBSD || MK_IS_PLATFORM_DRAGONFLY

#if defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	
	#define MK_IS_PLATFORM_IOS       TARGET_IPHONE_SIMULATOR == 1 || TARGET_OS_IPHONE == 1
	#define MK_IS_PLATFORM_MAC       TARGET_OS_OSX == 1
	#define MK_IS_PLATFORM_TVOS      TARGET_OS_TV == 1
	#define MK_IS_PLATFORM_WATCHOS   TARGET_OS_WATCH == 1
	#define MK_IS_PLATFORM_VISIONOS  TARGET_OS_VISION == 1
	#define MK_IS_PLATFORM_BRIDGE    TARGET_OS_BRIDGE == 1

	#define MK_IS_APPLE_EMBEDDED     TARGET_OS_IPHONE == 1 || TARGET_OS_SIMULATOR == 1
	#define MK_IS_APPLE_SIMULATOR    TARGET_OS_SIMULATOR == 1
	#define MK_IS_APPLE_DRIVERKIT    TARGET_OS_DRIVERKIT == 1

#else
	#define MK_IS_PLATFORM_IOS       0
	#define MK_IS_PLATFORM_MAC       0
	#define MK_IS_PLATFORM_TVOS      0
	#define MK_IS_PLATFORM_WATCHOS   0
	#define MK_IS_PLATFORM_VISIONOS  0
	#define MK_IS_PLATFORM_BRIDGE    0

	#define MK_IS_APPLE_EMBEDDED     0
	#define MK_IS_APPLE_SIMULATOR    0
	#define MK_IS_APPLE_DRIVERKIT    0
#endif

#if defined(__ANDROID__)
	#define MK_IS_PLATFORM_ANDROID   1
#else
	#define MK_IS_PLATFORM_ANDROID   0
#endif

#if defined(__EMSCRIPTEN__)
#define MK_IS_PLATFORM_EMSCRIPTEN    1
#else
#define MK_IS_PLATFORM_EMSCRIPTEN    0
#endif

#define MK_IS_PLATFORM_WEB MK_IS_PLATFORM_EMSCRIPTEN

// No actual implementation for NDA SDKs
#define MK_IS_PLATFORM_XB            0
#define MK_IS_PLATFORM_PS            0
#define MK_IS_PLATFORM_NT            0

#if MK_IS_PLATFORM_WINDOWS
	#define MK_PLATFORM_NAME Windows
#elif MK_IS_PLATFORM_LINUX
	#define MK_PLATFORM_NAME Linux
#elif MK_IS_PLATFORM_FREEBSD
	#define MK_PLATFORM_NAME FreeBSD
#elif MK_IS_PLATFORM_OPENBSD
	#define MK_PLATFORM_NAME OpenBSD
#elif MK_IS_PLATFORM_NETBSD
	#define MK_PLATFORM_NAME NetBSD
#elif MK_IS_PLATFORM_DRAGONFLY
	#define MK_PLATFORM_NAME DragonFly
#elif MK_IS_PLATFORM_IOS
	#define MK_PLATFORM_NAME iOS
#elif MK_IS_PLATFORM_MAC
	#define MK_PLATFORM_NAME macOS
#elif MK_IS_PLATFORM_TVOS
	#define MK_PLATFORM_NAME tvOS
#elif MK_IS_PLATFORM_WATCHOS
	#define MK_PLATFORM_NAME watchOS
#elif MK_IS_PLATFORM_VISIONOS
	#define MK_PLATFORM_NAME visionOS
#elif MK_IS_PLATFORM_BRIDGE
	#define MK_PLATFORM_NAME Bridge
#elif MK_IS_PLATFORM_ANDROID
	#define MK_PLATFORM_NAME Android
#elif MK_IS_PLATFORM_EMSCRIPTEN
	#define MK_PLATFORM_NAME Emscripten
#elif MK_IS_PLATFORM_XB
	#define MK_PLATFORM_NAME XB
#elif MK_IS_PLATFORM_PS
	#define MK_PLATFORM_NAME PS
#elif MK_IS_PLATFORM_NT
	#define MK_PLATFORM_NAME NT
#else
	#define MK_PLATFORM_NAME Unknown
#endif

#define MK_PLATFORM_NAME_CSTR CSTR_MACRO(MK_PLATFORM_NAME)
#define MK_PLATFORM_NAME_WSTR WSTR_MACRO(MK_PLATFORM_NAME)

#define MK_TRIM_DEBUG_INFO 0

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

	enum Platform : uint8_t
	{
		  kPlatform_Windows
		, kPlatform_Linux
		, kPlatform_FreeBSD
		, kPlatform_OpenBSD
		, kPlatform_NetBSD
		, kPlatform_DragonFly
		, kPlatform_iOS
		, kPlatform_macOS
		, kPlatform_tvOS
		, kPlatform_watchOS
		, kPlatform_visionOS
		, kPlatform_Bridge
		, kPlatform_Android
		, kPlatform_Emscripten
		, kPlatform_XB
		, kPlatform_PS
		, kPlatform_NT

		, kPlatform_Count
		, kPlatform_Unknown = uint8_t(255)
	};
	static_assert(kPlatform_Count < kPlatform_Unknown, "So many platforms that you need a bigger integer!");

	#define PLATFORM_ENUM_MACRO(m) PLATFORM_ENUM(m)
	#define PLATFORM_ENUM(name) kPlatform_##name
	static constexpr auto kPlatform = PLATFORM_ENUM_MACRO(MK_PLATFORM_NAME);
	#undef PLATFORM_ENUM
	#undef PLATFORM_ENUM_MACRO
}

#endif