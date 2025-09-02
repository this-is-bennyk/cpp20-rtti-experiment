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

#include "OS.hpp"

#include <cstdlib>
#include <format>
#include "Program.hpp"

namespace
{
	OS::Memory::Stats stats;

	std::uintptr_t GetAlignmentPadding(const std::size_t alignment) noexcept
	{
		// ReSharper is wrong both times here
		// ReSharper disable once CppDFAConstantConditions
		Program::Assert(alignment == 0 || std::has_single_bit(alignment), "Alignment must be power of 2!");
		// ReSharper thinks a ternary conditional creates unreachable code
		// ReSharper disable once CppDFAConstantConditions
		return std::uintptr_t(alignment > 0) * std::uintptr_t(alignment) - 1;
	}

	std::size_t GetTotalAllocationSize(const std::size_t alignment, const std::size_t size, const std::size_t count) noexcept
	{
		Program::Assert(alignment == 0 || std::has_single_bit(alignment), "Alignment must be power of 2!");
		return alignment > 0 ? std::size_t(GetAlignmentPadding(alignment)) + (size * count) + sizeof(std::uintptr_t) : size * count;
	}

	void* Align(void* ptr, const std::size_t alignment, const std::size_t size, const std::size_t count, const bool count_waste) noexcept
	{
		Program::Assert(alignment == 0 || std::has_single_bit(alignment), "Alignment must be power of 2!");

		if (alignment > 0)
		{
			// Aligned storage model:
			//
			// (alignment - 1) + (size * count) + (sizeof(std::uintptr_t)) = GetTotalAllocationSize
			//
			// Original Aligned          Unaligned bits storage
			// \/  ...  \/ ...           \/ (std::uintptr_t) ...
			//  ------------------------------------------------------...
			//  | Alignment space | ...  | Unaligned bits storage space |
			//
			// Notes:
			// - Some storage is wasted since we don't know how far back the aligned pointer was moved in the alignment space.
			// - There's also a slight amount of overhead so the unaligned bits can be stored, but that allows us to undo the alignment
			//   for reallocation and deallocation.

			const std::uintptr_t alignment_padding = GetAlignmentPadding(alignment);
			const std::uintptr_t unaligned_bits = (reinterpret_cast<std::uintptr_t>(ptr) + alignment_padding) & alignment_padding;

			const auto aligned_ptr = reinterpret_cast<void*>((reinterpret_cast<std::uintptr_t>(ptr) + alignment_padding) & ~alignment_padding);
			const auto unaligned_bits_storage = static_cast<std::uintptr_t*>(static_cast<void*>(static_cast<char*>(aligned_ptr) + (size * count)));

			*unaligned_bits_storage = unaligned_bits;

			if (count_waste)
			{
				const std::size_t total_size = GetTotalAllocationSize(alignment, size, count);

				stats.cur_alignment_waste += total_size - (size * count);
				stats.max_alignment_waste += total_size - (size * count);
			}

			return aligned_ptr;
		}

		return ptr;
	}

	void* Dealign(void* aligned_ptr, const std::size_t alignment, const std::size_t size, const std::size_t count, const bool count_waste) noexcept
	{
		Program::Assert(aligned_ptr, "Aligned pointer is null!");
		Program::Assert(alignment == 0 || std::has_single_bit(alignment), "Alignment must be power of 2!");

		if (alignment > 0)
		{
			const auto unaligned_bits_storage = static_cast<std::uintptr_t*>(static_cast<void*>(static_cast<char*>(aligned_ptr) + (size * count)));
			const std::uintptr_t unaligned_bits = *unaligned_bits_storage;
			const auto original_ptr = reinterpret_cast<void*>((reinterpret_cast<std::uintptr_t>(aligned_ptr) | unaligned_bits) - GetAlignmentPadding(alignment));

			if (count_waste)
				stats.cur_alignment_waste -= GetTotalAllocationSize(alignment, size, count) - (size * count);

			return original_ptr;
		}

		return aligned_ptr;
	}

	// ReSharper is wrong
	// ReSharper disable once CppDFAConstantParameter
	void PrintMemoryStat(const std::size_t bytes, const Program::Name tag)
	{
		Program::Log::Std(L"Memory") << tag
			<< L": "  <<  bytes                << L" B"
			<< L" (~" << (bytes / 1000)       << L" KB)"
			<< L" (~" << (bytes / 1000000)    << L" MB)"
			<< L" (~" << (bytes / 1000000000) << L" GB)"
			<< std::endl;
	}
}

void* OS::Memory::Allocate(const std::size_t alignment, const std::size_t size, const std::size_t count) noexcept
{
	Program::Assert(count <= std::numeric_limits<std::size_t>::max() / size, "Too large of an allocation!");

	const size_t total_size = GetTotalAllocationSize(alignment, size, count);

	void* ptr = std::malloc(total_size);
	Program::Assert(ptr, "Failed to allocate memory!");

	stats.cur_memory_used += total_size;
	stats.max_memory_used += total_size;

	return Align(ptr, alignment, size, count, true);
}

void* OS::Memory::Reallocate(void* ptr, const std::size_t alignment, const std::size_t size, std::size_t& old_count, const std::size_t new_count, const bool tracked) noexcept
{
	if (!ptr)
		return Allocate(alignment, size, new_count);

	// No point in physically shrinking memory, we might use it later
	if (new_count <= old_count)
		return ptr;

	Program::Assert(new_count <= std::numeric_limits<std::size_t>::max() / size, "Too large of an allocation!");

	const size_t old_total_size = GetTotalAllocationSize(alignment, size, old_count);
	// ReSharper is wrong
	// ReSharper disable once CppDFAConstantConditions
	void* old_ptr = ptr ? Dealign(ptr, alignment, size, old_count, false) : nullptr;

	const size_t new_total_size = GetTotalAllocationSize(alignment, size, new_count);
	void* new_ptr = std::realloc(old_ptr, new_total_size);

	Program::Assert(new_ptr, "Failed to reallocate memory!");

	if (old_ptr != new_ptr)
		Deallocate(old_ptr, alignment, size, old_count);

	if (tracked) [[likely]]
	{
		stats.cur_memory_used += new_total_size - old_total_size;
		stats.max_memory_used += new_total_size - old_total_size;
	}
	else
		++stats.untracked_reallocations;

	old_count = new_count;

	return Align(new_ptr, alignment, size, new_count, false);
}

void OS::Memory::Deallocate(void* ptr, const std::size_t alignment, const std::size_t size, const std::size_t count) noexcept
{
	if (ptr)
	{
		std::free(Dealign(ptr, alignment, size, count, true));
		stats.cur_memory_used -= GetTotalAllocationSize(alignment, size, count);
	}
}

void OS::Memory::Report()
{
	PrintMemoryStat(stats.cur_memory_used, L"Cur. Used Memory");
	PrintMemoryStat(stats.max_memory_used, L"Max. Used Memory");

	PrintMemoryStat(stats.cur_alignment_waste, L"Cur. Alignment Waste");
	PrintMemoryStat(stats.max_alignment_waste, L"Max. Alignment Waste");

	Program::Log::Std(L"Memory") << L"Untracked Reallocations: " << stats.untracked_reallocations << std::endl;
}

OS::Memory::Stats OS::Memory::GetStats() noexcept { return stats; }

void* OS::Memory::Simple::DirtyAllocate(const std::size_t size) noexcept { return Allocate(0, size, 1); }

void* OS::Memory::Simple::CleanAllocate(const std::size_t num, const std::size_t size) noexcept
{
	void* ptr = DirtyAllocate(num * size);
	std::memset(ptr, 0, num * size);
	return ptr;
}

void* OS::Memory::Simple::Reallocate(void* ptr, const std::size_t size) noexcept
{
	std::size_t unused = 0;
	return Memory::Reallocate(ptr, 0, unused, unused, size, false);
}

void  OS::Memory::Simple::Deallocate(void* ptr, const std::size_t size) noexcept { return Memory::Deallocate(ptr, 0, size, 1); }
