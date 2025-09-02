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

#ifndef PROGRAM_HPP
#define PROGRAM_HPP

#include <iomanip>
#include <iostream>
#include <source_location>

// Included for convenience
#include "ProgramConfig.hpp"
#include "Name.hpp"

namespace Program
{
	namespace Log
	{
		inline std::wostream& Std(const Name tag = L"") { return std::wcout << (tag.empty() ? tag : L"[") << tag << (tag.empty() ? tag : L"] "); }
		inline std::wostream& Err(const Name tag = L"") { return std::wcerr << (tag.empty() ? tag : L"[") << tag << (tag.empty() ? tag : L"] "); }
	}

	void Assert(bool statement, const wchar_t* message, const std::source_location location = std::source_location::current());
	void Assert(bool statement, const char* message, const std::source_location location = std::source_location::current());
}

#endif