#include "Meta.h"

#include <algorithm>
#include <deque>
#include <iomanip>
#include <iostream>
#include <limits>
#include <queue>
#include <unordered_map>
#include "MetaConfig.h"

namespace Meta
{
	static Index type_counter = 0;

	template<typename Reservable>
	static Reservable Prealloc()
	{
		Reservable result;
		result.reserve(kPreallocationAmount);
		return result;
	}

	static auto infos = Prealloc<std::vector<Information>>();
	static auto name_to_index = Prealloc<std::unordered_map<std::string_view, Index>>();

	// static auto constructors = Prealloc<std::vector<Constructor>>();
	// static auto destructors = Prealloc<std::vector<Destructor>>();
}

namespace Meta
{
	const Information& Register(const std::string_view name, const size_t size)
	{
		Index index = kInvalidType;

		if (name_to_index.empty() || !name_to_index.contains(name))
		{
			index = type_counter++;

			infos.push_back(Information
				{
					  .index = index
					, .name = name
					, .size = size
				}
			);

			name_to_index.insert_or_assign(name, index);
		}
		else
			index = name_to_index.find(name)->second;

		return infos[index];
	}

	Index Find(const std::string_view name)
	{
		const auto iterator = name_to_index.find(name);

		if (iterator == name_to_index.end())
			return kInvalidType;

		return iterator->second;
	}

	const Information* Get(Index type)
	{
		assert(type > kInvalidType && type <= type_counter && "Type index out of bounds!");
		return &infos[type];
	}

	bool Valid(Index type_index)
	{
		return type_index > kInvalidType && type_index <= type_counter;
	}

	bool AddInheritance(Information& derived_info, const std::vector<Index>& directly_inherited)
	{
		if (directly_inherited.empty())
			return true;

		for (const Index parent : directly_inherited)
		{
			if (parent == derived_info.index || !Valid(parent))
				continue;

			if (size_t(parent) >= derived_info.bases.size())
				derived_info.bases.resize(parent + 1);

			derived_info.bases[parent] = true;
			++derived_info.num_bases;

			const Information& parent_info = *Get(parent);
			std::vector<Index> parent_inherited;

			parent_inherited.reserve(parent_info.num_bases);

			for (size_t parent_base_index = 0; parent_info.num_bases > 0 && parent_base_index < parent_info.bases.size(); ++parent_base_index)
			{
				if (parent_info.bases[parent_base_index])
					parent_inherited.push_back(Index(parent_base_index));
			}

			AddInheritance(derived_info, parent_inherited);
		}

		return true;
	}

	void DumpInfo()
	{
		static constexpr auto kLabel = "[Meta] ";

		std::cout << kLabel << "-------------------- Meta --------------------" << std::endl;
		std::cout << kLabel << "~~~~~ Type List ~~~~" << std::endl;

		Index digits = 0;
		Index type_counter_copy = type_counter;

		while (type_counter_copy > 0)
		{
			type_counter_copy /= 10;
			++digits;
		}

		for (const Information& info : infos)
		{
			std::cout << kLabel << "Type ID: " << std::setfill('0') << std::setw(int(digits)) << info.index <<
					" | Name: " << info.name;

			if (info.num_bases > 0)
			{
				std::cout << " | Bases: ";

				Index base_count = 0;

				for (size_t base_index = 0; base_index < info.bases.size(); ++base_index)
				{
					if (info.bases[base_index])
					{
						std::cout << infos[base_index].name << " ";

						if (base_count < info.num_bases - 1)
							std::cout << ", ";

						++base_count;
					}
				}
			}

			std::cout << std::endl;
		}

		std::cout << kLabel << "~~~~~ Type Stats ~~~~~" << std::endl;
		std::cout << kLabel << "Number of types: " << type_counter << std::endl;

		if (size_t(type_counter) > kPreallocationAmount)
			std::cout << kLabel << "Recommendation: Set kPreallocationAmount to " << type_counter << std::endl;
	}

	View::operator bool() const
	{
		return valid();
	}

	bool View::valid() const
	{
		return data && Valid(type);
	}

	bool View::is(const Information& info) const
	{
		if (!(Valid(info.index) && valid()))
			return false;

		if (type == info.index)
			return true;

		const Information* my_info = Get(type);
		return size_t(info.index) < my_info->bases.size() && my_info->bases[info.index];
	}
}

META(u8)
META(u16)
META(u32)
META(u64)
META(i8)
META(i16)
META(i32)
META(i64)
META(f32)
META(f64)
META(bool)
META_COMPLEX(std::vector<Meta::Handle>, stdvectormetahandle)

META_AS(Meta::Handle, Handle)
META_AS(Meta::View, View)

namespace Pools
{
	static constexpr size_t kMaxSize = size_t(std::numeric_limits<Index>::max()) + 1;

	class Pool
	{
	public:
		Pool(Meta::Index type_index = kInvalidIndex)
			: type(type_index)
		{}

		Index alloc()
		{
			Index index = kInvalidIndex;

			if (last_deleted != kInvalidIndex)
			{
				index = last_deleted;
				const Index next_to_recycle = deleted_jump_table[last_deleted];

				// Indicate that this element is in use again
				deleted_jump_table[last_deleted] = kInvalidIndex;
				// Make the element recycled before this one the next up to be reused
				last_deleted = next_to_recycle;

				// If we've reached the first index deleted, mark it as no longer being deleted
				if (first_deleted == index)
					first_deleted = kInvalidIndex;

				const Meta::Information* info = Meta::Get(type);

				// Clear previous data
				for (size_t i = 0; i < info->size; ++i)
					data[index * info->size + i] = 0;
			}
			else if (deleted_jump_table.size() < kMaxSize)
			{
				const Meta::Information* info = Meta::Get(type);

				data.resize((num_allocated + 1) * info->size, u8(0));
				deleted_jump_table.push_back(kInvalidIndex);
				references.push_back(0);

				index = Index(deleted_jump_table.size() - 1);
			}
			else
#ifdef _DEBUG
				assert(!"Ran out of memory!");
#else
				std::abort();
#endif

			if (index != kInvalidIndex)
			{
				++num_allocated;
				ref(index);
			}

			return index;
		}

		void ref(Index index)
		{
			if (is_deleted(index))
				return;

			++references[index];
		}

		void deref(Index index)
		{
			if (is_deleted(index))
				return;

			--references[index];

			if (references[index] == 0)
			{
				deleted_jump_table[index] = last_deleted;
				last_deleted = index;

				if (first_deleted == kInvalidIndex)
					first_deleted = index;

				--num_allocated;
			}
		}

		[[nodiscard]] bool is_valid(Index index) const
		{
			return index > kInvalidIndex && size_t(index) < deleted_jump_table.size();
		}

		void* get(Index index)
		{
			return is_valid(index) ? &data[size_t(index) * Meta::Get(type)->size] : nullptr;
		}

		[[nodiscard]] bool is_deleted(Index index) const
		{
			const bool valid = is_valid(index);

			// The first deleted element will have the invalid index as its next jump index
			// so we need to check for that by seeing if this index is that one
			const bool deleted = deleted_jump_table[index] != kInvalidIndex || index == first_deleted;

			return valid && deleted;
		}

	private:
		std::vector<u8> data;
		std::vector<size_t> references;
		std::vector<Index> deleted_jump_table;

		Index last_deleted = kInvalidIndex;
		Index first_deleted = kInvalidIndex;
		size_t num_allocated = 0;

		Meta::Index type;
	};

	Pool* get_pool(Meta::Index type)
	{
		static std::vector<Pool> pools;
		static std::vector<bool> used;

		if (!Meta::Valid(type))
			return nullptr;

		if (size_t(type) >= used.size())
		{
			used.resize(type + 1);
			pools.resize(type + 1);
		}

		if (!used[type])
		{
			pools[type] = Pool(type);
			used[type]  = true;
		}

		return &pools[type];
	}

	class Heap
	{
	public:
		struct Range
		{
			Index start;
			Index end;

			[[nodiscard]] size_t size() const { return size_t(end - start); }
		};

		struct RangeComparator
		{
			bool operator()(const Range& lhs, const Range& rhs) const
			{
				return lhs.size() < rhs.size();
			}
		};

		Heap(Meta::Index type_index = kInvalidIndex)
			: type(type_index)
		{}

		Range alloc(size_t size)
		{
			Range result(kInvalidIndex, kInvalidIndex);

			if (queue.size() < kMaxSize)
			{
				if (queue.empty() || queue.top().size() < size)
				{
					const Meta::Information* info = Meta::Get(type);

					data.resize((num_allocated + size) * info->size, u8(0));

					result.start = Index(num_allocated);
					result.end = result.start + Index(size);
				}
				else
				{
					result = queue.top();
					queue.pop();
				}
			}
			else
#ifdef _DEBUG
				assert(!"Ran out of memory!");
#else
				std::abort();
#endif

			if (result.start != kInvalidIndex && result.end != kInvalidIndex)
				num_allocated += size;

			return result;
		}

	private:
		std::vector<u8> data;
		std::priority_queue<Range, std::deque<Range>, RangeComparator> queue;

		size_t num_allocated = 0;

		Meta::Index type;
	};
}

namespace Meta
{
	Handle::Handle(Handle& other)
		: view(other.view)
		, index(other.index)
	{
		Pools::get_pool(view.type)->ref(index);
	}

	inline Handle::Handle(const Handle& other)
		: view(other.view)
		, index(other.index)
	{
		Pools::get_pool(view.type)->ref(index);
	}

	Handle::Handle(Handle&& other) noexcept
		: view(std::move(other.view))
		, index(std::move(other.index))
	{
		other.invalidate();
	}

	Handle::Handle(const Information& info)
		: view(Meta::View())
		, index(Pools::kInvalidIndex)
	{
		index = Pools::get_pool(info.index)->alloc();
		view = Meta::View(Pools::get_pool(info.index)->get(index), info);
	}

	Handle::Handle(View v)
		: view(v)
		, index(Pools::kInvalidIndex)
	{}

	Handle::~Handle()
	{
		destroy();
	}

	inline Handle& Handle::operator=(const Handle& other)
	{
		if (this != &other)
		{
			if (*this)
				destroy();

			view = other.view;
			index = other.index;

			Pools::get_pool(view.type)->ref(index);
		}

		return *this;
	}

	Handle& Handle::operator=(Handle&& other) noexcept
	{
		if (this != &other)
		{
			if (*this)
				destroy();

			view = other.view;
			index = other.index;

			other.invalidate();
		}

		return *this;
	}

	Handle::operator bool() const
	{
		return valid();
	}

	bool Handle::valid() const
	{
		return view.valid() && index != Pools::kInvalidIndex;
	}

	bool Handle::is(const Information& info) const
	{
		return view.is(info);
	}

	View Handle::peek() const
	{
		return view;
	}

	void Handle::invalidate()
	{
		view = Meta::View();
		index = Pools::kInvalidIndex;
	}

	void Handle::destroy()
	{
		if (!valid())
			return;

		Pools::get_pool(view.type)->deref(index);
		invalidate();
	}

	Spandle::Spandle(size_t num_handles)
		: pimpl()
	{
		if (num_handles > 0)
		{
			pimpl = std::vector<Meta::Handle>();
			pimpl.as<std::vector<Meta::Handle>>().resize(num_handles);
		}
	}

	Handle& Spandle::operator[](size_t index)
	{
		return pimpl.as<std::vector<Meta::Handle>>()[index];
	}

	Handle Spandle::operator[](size_t index) const
	{
		return pimpl.as<std::vector<Meta::Handle>>()[index];
	}

	size_t Spandle::size() const
	{
		if (!pimpl.valid())
			return 0;
		return pimpl.as<std::vector<Meta::Handle>>().size();
	}

	bool Spandle::empty() const
	{
		if (!pimpl.valid())
			return true;
		return pimpl.as<std::vector<Meta::Handle>>().empty();
	}
}
