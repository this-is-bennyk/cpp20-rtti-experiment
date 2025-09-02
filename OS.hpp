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

#ifndef EXTROPY_OS_HPP
#define EXTROPY_OS_HPP

#include <cstddef>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "patricia.hpp"

namespace OS::Memory
{
	struct Stats
	{
		std::size_t cur_memory_used = 0;
		std::size_t max_memory_used = 0;

		std::size_t cur_alignment_waste = 0;
		std::size_t max_alignment_waste = 0;

		std::size_t untracked_reallocations = 0;
	};

	[[nodiscard]] void* Allocate(const std::size_t alignment, const std::size_t size, const std::size_t count) noexcept;
	void* Reallocate(void* ptr, const std::size_t alignment, const std::size_t size, std::size_t& old_count, const std::size_t new_count, const bool tracked = true) noexcept;
	void Deallocate(void* ptr, const std::size_t alignment, const std::size_t size, const std::size_t count) noexcept;

	void Report();
	Stats GetStats() noexcept;

	template<typename T>
	struct UnalignedAllocator
	{
		using value_type                             = T;
		using size_type                              = std::size_t;
		using difference_type                        = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal                        = std::true_type;

		template <typename U>
		struct rebind
		{
			using other = UnalignedAllocator<U>;
		};

		static constexpr std::size_t kTypeSize = sizeof(T);
		static constexpr std::size_t kMaxSize  = std::numeric_limits<std::size_t>::max() / kTypeSize;

		constexpr UnalignedAllocator() noexcept = default;
		constexpr UnalignedAllocator(const UnalignedAllocator&) noexcept : UnalignedAllocator() {}
		constexpr UnalignedAllocator(UnalignedAllocator&&) noexcept : UnalignedAllocator() {}

		template<typename U>
		constexpr explicit UnalignedAllocator(const UnalignedAllocator<U>&) noexcept : UnalignedAllocator() {}
		template<typename U>
		constexpr explicit UnalignedAllocator(UnalignedAllocator<U>&) noexcept : UnalignedAllocator() {}
		template<typename U>
		constexpr explicit UnalignedAllocator(UnalignedAllocator<U>&&) noexcept : UnalignedAllocator() {}

		~UnalignedAllocator() noexcept = default;

		constexpr UnalignedAllocator& operator=(const UnalignedAllocator&) noexcept = default;
		constexpr UnalignedAllocator& operator=(UnalignedAllocator&&) noexcept      = default;

		[[nodiscard]] static constexpr T* allocate(const std::size_t count) noexcept
		{
			return static_cast<T*>(Allocate(0, kTypeSize, count));
		}

		static constexpr void deallocate(T* ptr, const std::size_t count) noexcept
		{
			Memory::Deallocate(ptr, 0, kTypeSize, count);
		}

		friend constexpr bool operator==(const UnalignedAllocator&, const UnalignedAllocator&) noexcept { return true; }
	};

	template<typename T>
	struct Allocator
	{
		using value_type                             = T;
		using size_type                              = std::size_t;
		using difference_type                        = std::ptrdiff_t;
		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal                        = std::true_type;

		template <typename U>
		struct rebind
		{
			using other = Allocator<U>;
		};

		static constexpr std::size_t kTypeAlignment = alignof(T);
		static constexpr std::size_t kTypeSize      = sizeof(T);

		constexpr Allocator() noexcept = default;
		constexpr Allocator(const Allocator&) noexcept : Allocator() {}
		constexpr Allocator(Allocator&&) noexcept : Allocator() {}

		template<typename U>
		constexpr explicit Allocator(const Allocator<U>&) noexcept : Allocator() {}
		template<typename U>
		constexpr explicit Allocator(Allocator<U>&) noexcept : Allocator() {}
		template<typename U>
		constexpr explicit Allocator(Allocator<U>&&) noexcept : Allocator() {}

		constexpr ~Allocator() noexcept = default;

		constexpr Allocator& operator=(const Allocator&) noexcept = default;
		constexpr Allocator& operator=(Allocator&&) noexcept      = default;

		[[nodiscard]] static constexpr T* allocate(const std::size_t count) noexcept
		{
			return static_cast<T*>(Allocate(kTypeAlignment, kTypeSize, count));
		}

		static constexpr void deallocate(T* ptr, const std::size_t count) noexcept
		{
			Memory::Deallocate(ptr, kTypeAlignment, kTypeSize, count);
		}

		friend constexpr bool operator==(const Allocator&, const Allocator&) noexcept { return true; }
	};

	namespace Simple
	{
		void* DirtyAllocate(const std::size_t size) noexcept;
		void* CleanAllocate(const std::size_t num, const std::size_t size) noexcept;
		void* Reallocate(void* ptr, const std::size_t size) noexcept;
		void  Deallocate(void* ptr, const std::size_t size) noexcept;
	}
}

template<typename T, typename U>
constexpr bool operator==(const OS::Memory::UnalignedAllocator<T>&, const OS::Memory::UnalignedAllocator<U>&) noexcept { return true; }
template<typename T, typename U>
constexpr bool operator!=(const OS::Memory::UnalignedAllocator<T>&, const OS::Memory::UnalignedAllocator<U>&) noexcept { return false; }

template<typename T, typename U>
constexpr bool operator==(const OS::Memory::Allocator<T>&, const OS::Memory::Allocator<U>&) noexcept { return true; }
template<typename T, typename U>
constexpr bool operator!=(const OS::Memory::Allocator<T>&, const OS::Memory::Allocator<U>&) noexcept { return false; }

namespace OS
{
	template<typename T>
	using Vector = std::vector<T, Memory::Allocator<T>>;

	// TODO: Is there a portable way to get the allocation type for a std::vector<bool>?
	using BitType = unsigned long;
	using BitVector = std::vector<bool, Memory::Allocator<BitType>>;

	template<typename T>
	using SortSet = std::set<T, Memory::Allocator<T>>;

	template<typename K, typename V>
	using SortMap = std::map<K, V, Memory::Allocator<K>>;

	template<typename T>
	using HashSet = std::unordered_set<T, Memory::Allocator<T>>;

	template<typename K, typename V>
	using HashMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, Memory::Allocator<std::pair<const K, V>>>;

	template<typename K, typename KM = sk::patricia_key_maker<K>>
	using TrieSet = sk::patricia_set<K, KM, Memory::Allocator<K>>;

	template<typename K, typename V, typename KM = sk::patricia_key_maker<K>>
	using TrieMap = sk::patricia_map<K, V, KM, Memory::Allocator<K>>;
}

#endif //EXTROPY_OS_HPP