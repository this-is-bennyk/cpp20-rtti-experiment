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

#include "Meta.h"

#include <algorithm>
#include <bit>
#include <cstdlib>
#include <deque>
#include <iterator>
#include <limits>
#include <memory>
#include <queue>
#include <unordered_map>
#include "MetaConfig.h"

namespace
{
	template <typename T>
	T& Require(T* ptr)
	{
		Program::Assert(ptr, MK_ASSERT_STATS_CSTR, "Pointer must not be null!");
		return *ptr;
	}
}

namespace Meta
{
	static Index type_counter = 0;

	template<typename Reservable>
	static Reservable PreallocateContainer()
	{
		Reservable result;
		result.reserve(kPreallocationAmount);
		return result;
	}

	using  InfosContainer = std::vector<Information>;
	static InfosContainer* infos_ptr = nullptr;

	using  NameToIndexContainer = std::unordered_map<Program::Name, Index>;
	static NameToIndexContainer* name_to_index_ptr = nullptr;

	using  ConstructorsContainer = std::vector<std::unordered_map<FunctionSignature, Constructor>>;
	static ConstructorsContainer* constructors_ptr = nullptr;

	using  DestructorsContainer = std::vector<Destructor>;
	static DestructorsContainer* destructors_ptr = nullptr;

	using  AssignersContainer = std::vector<std::unordered_map<FunctionSignature, Assigner>>;
	static AssignersContainer* assigners_ptr = nullptr;

	using  UnaryOpsContainer = std::vector<std::vector<std::unordered_map<FunctionSignature, UnaryOperator>>>;
	static UnaryOpsContainer* unary_ops_ptr = nullptr;

	using  BinaryOpsContainer = std::vector<std::vector<std::unordered_map<FunctionSignature, BinaryOperator>>>;
	static BinaryOpsContainer* binary_ops_ptr = nullptr;

	using  CastersContainer = std::vector<std::vector<Caster>>;
	static CastersContainer* casters_ptr = nullptr;

	using ConvertersContainer = std::vector<std::vector<Converter>>;
	static ConvertersContainer* converters_ptr = nullptr;
}

NAMEOF_DEF(u8);
NAMEOF_DEF(u16);
NAMEOF_DEF(u32);
NAMEOF_DEF(u64);
NAMEOF_DEF(i8);
NAMEOF_DEF(i16);
NAMEOF_DEF(i32);
NAMEOF_DEF(i64);
NAMEOF_DEF(f32);
NAMEOF_DEF(f64);
NAMEOF_DEF(bool);

using View = Meta::View;
using Handle = Meta::Handle;

NAMEOF_DEF(View);
NAMEOF_DEF(Handle);

namespace Memory
{
	static constexpr size_t kMaxSize = size_t(std::numeric_limits<Index>::max()) + 1;

	template<typename T>
	T* get_allocator(const Meta::Index type)
	{
		static std::vector<T> spaces;
		static std::vector<bool> used;

		if (!Meta::Valid(type))
			return nullptr;

		if (size_t(type) >= used.size())
		{
			used.resize(type + 1);
			spaces.resize(type + 1);
		}

		if (!used[type])
		{
			spaces[type] = T(type);
			used[type]  = true;
		}

		return &spaces[type];
	}

	class Pool
	{
	public:
		explicit Pool(const Meta::Index type_index = Meta::kInvalidType)
			: type(type_index)
		{}

		~Pool() = default;

		Index alloc(const Meta::Spandle& arguments)
		{
			const Meta::Information* info = Meta::Get(type);
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
			}
			else if (deleted_jump_table.size() < kMaxSize)
			{
				data.resize((num_allocated + 1) * info->size, u8(0));
				deleted_jump_table.push_back(kInvalidIndex);
				references.push_back(0);

				index = Index(deleted_jump_table.size() - 1);
			}
			else
				Program::Assert(false, MK_ASSERT_STATS_CSTR, "Ran out of memory!");

			if (index != kInvalidIndex)
			{
				++num_allocated;
				ref(index);

				Meta::ParameterArray memory = { std::make_pair(Meta::kInvalidType, Meta::kQualifier_Temporary) };
				const Meta::FunctionSignature signature = arguments.get_function_signature(memory);
				const Meta::Constructor constructor = Meta::GetConstructor(type, signature);
				constructor(Meta::View(get(index), *info, Meta::kQualifier_Reference), arguments);
			}

			return index;
		}

		void ref(const Index index)
		{
			if (is_deleted(index))
				return;

			++references[index];
		}

		void deref(const Index index)
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

				const Meta::Information* info = Meta::Get(type);

				Meta::GetDestructor(type)(View(get(index), *info, Meta::kQualifier_Reference));

				std::fill(&data[size_t(index) * info->size], &data[size_t(index + 1) * info->size], u8());
			}
		}

		[[nodiscard]] bool is_valid(const Index index) const
		{
			return index > kInvalidIndex && size_t(index) < deleted_jump_table.size();
		}

		void* get(const Index index)
		{
			return is_valid(index) ? &data[size_t(index) * Meta::Get(type)->size] : nullptr;
		}

		[[nodiscard]] bool is_deleted(const Index index) const
		{
			// The first deleted element will have the invalid index as its next jump index
			// so we need to check for that by seeing if this index is that one
			return !is_valid(index) || deleted_jump_table[index] != kInvalidIndex || index == first_deleted;
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

	class Heap
	{
	public:
		explicit Heap(const Meta::Index type_index = Meta::kInvalidType)
			: type(type_index)
		{}

		~Heap()
		{
			std::free(data);
		}

		Range alloc(const size_t size, const Meta::Spandle& arguments)
		{
			const Meta::Information* info = Meta::Get(type);
			Range result(kInvalidIndex, kInvalidIndex);

			if (queue.size() < kMaxSize)
			{
				if (queue.empty() || queue.top().size() < size)
				{
					if (capacity < num_allocated + size)
					{
						const size_t new_capacity = std::bit_ceil(num_allocated + size);
						void* allocation = std::realloc(data, new_capacity * info->size);

						Program::Assert(allocation, MK_ASSERT_STATS_CSTR, "Ran out of memory!");

						if (allocation != data)
						{
							std::free(data);
							data = allocation;
						}

						capacity = new_capacity;
					}

					used.resize(num_allocated + size, true);

					result.start = Index(num_allocated);
					result.end = result.start + Index(size);
				}
				else
				{
					result = queue.top();
					queue.pop();
					std::fill(std::next(used.begin(), result.start), std::next(used.begin(), result.end), true);
				}
			}
			else
				Program::Assert(false, MK_ASSERT_STATS_CSTR, "Ran out of memory!");

			if (result.start != kInvalidIndex && result.end != kInvalidIndex)
			{
				num_allocated += size;

				Meta::ParameterArray memory = { std::make_pair(Meta::kInvalidType, Meta::kQualifier_Temporary) };
				const Meta::FunctionSignature signature = arguments.get_function_signature(memory);
				const Meta::Constructor constructor = Meta::GetConstructor(type, signature);

				for (Index index = result.start; index < result.end; ++index)
					constructor(Meta::View(get(index), *info, Meta::kQualifier_Reference), arguments);
			}

			return result;
		}

		void free(const Range range)
		{
			if (range.empty())
				return;

			const Meta::Information* info = Meta::Get(type);

			for (Index index = range.start; index < range.end; ++index)
				Meta::GetDestructor(info->index)(View(get(index), *info, Meta::kQualifier_Reference));

			std::fill(std::next(used.begin(), range.start), std::next(used.begin(), range.end), false);
			queue.push(range);

			num_allocated -= range.size();
		}

		void* get(const Index index)
		{
			return used[index] ? static_cast<u8*>(data) + (size_t(index) * Meta::Get(type)->size) : nullptr;
		}

	private:
		void* data = nullptr;
		size_t capacity = 0;

		std::vector<bool> used;
		std::priority_queue<Range, std::deque<Range>, RangeComparator> queue;

		size_t num_allocated = 0;

		Meta::Index type;
	};
}

namespace Meta
{
	std::pair<std::vector<Information>*, Index> Register(const Program::Name name, const size_t size)
	{
		static auto infos         = PreallocateContainer<std::remove_pointer_t<decltype(infos_ptr)>>();
		static auto name_to_index = PreallocateContainer<std::remove_pointer_t<decltype(name_to_index_ptr)>>();
		static auto casters       = PreallocateContainer<std::remove_pointer_t<decltype(casters_ptr)>>();
		static auto converters    = PreallocateContainer<std::remove_pointer_t<decltype(converters_ptr)>>();

		static auto constructors = PreallocateContainer<std::remove_pointer_t<decltype(constructors_ptr)>>();
		static auto destructors  = PreallocateContainer<std::remove_pointer_t<decltype(destructors_ptr)>>();
		static auto assigners    = PreallocateContainer<std::remove_pointer_t<decltype(assigners_ptr)>>();
		static auto unary_ops    = PreallocateContainer<std::remove_pointer_t<decltype(unary_ops_ptr)>>();
		static auto binary_ops   = PreallocateContainer<std::remove_pointer_t<decltype(binary_ops_ptr)>>();

		static bool initialized = false;

		if (!initialized)
		{
			initialized = true;

			infos_ptr = &infos;
			name_to_index_ptr = &name_to_index;
			casters_ptr = &casters;
			converters_ptr = &converters;

			constructors_ptr = &constructors;
			destructors_ptr = &destructors;
			assigners_ptr = &assigners;
			unary_ops_ptr = &unary_ops;
			binary_ops_ptr = &binary_ops;

			Program::Assert(AddPrimitiveIntegralType<u8>(),  MK_ASSERT_STATS_CSTR, "Error in initializing u8 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<u16>(), MK_ASSERT_STATS_CSTR, "Error in initializing u16 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<u32>(), MK_ASSERT_STATS_CSTR, "Error in initializing u32 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<u64>(), MK_ASSERT_STATS_CSTR, "Error in initializing u64 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<i8>(),  MK_ASSERT_STATS_CSTR, "Error in initializing i8 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<i16>(), MK_ASSERT_STATS_CSTR, "Error in initializing i16 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<i32>(), MK_ASSERT_STATS_CSTR, "Error in initializing i32 into the Meta system!");
			Program::Assert(AddPrimitiveIntegralType<i64>(), MK_ASSERT_STATS_CSTR, "Error in initializing i64 into the Meta system!");
			Program::Assert(AddPrimitiveFloatType<f32>(),    MK_ASSERT_STATS_CSTR, "Error in initializing f32 into the Meta system!");
			Program::Assert(AddPrimitiveFloatType<f64>(),    MK_ASSERT_STATS_CSTR, "Error in initializing f64 into the Meta system!");
			Program::Assert(AddPOD<bool>(),                  MK_ASSERT_STATS_CSTR, "Error in initializing bool into the Meta system!");
			Program::Assert(AddPOD<View>(),                  MK_ASSERT_STATS_CSTR, "Error in initializing View into the Meta system!");
			Program::Assert(AddPOD<Handle>(),                MK_ASSERT_STATS_CSTR, "Error in initializing Handle into the Meta system!");
		}

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

			constructors.emplace_back();
			destructors.push_back(nullptr);
			assigners.emplace_back();
			unary_ops.emplace_back();
			binary_ops.emplace_back();
		}
		else
			index = name_to_index.find(name)->second;

		return { &infos, index };
	}

	Index Find(const Program::Name name)
	{
		const auto iterator = Require(name_to_index_ptr).find(name);

		if (iterator == Require(name_to_index_ptr).end())
			return kInvalidType;

		return iterator->second;
	}

	const Information* Get(const Index type)
	{
		Program::Assert(type > kInvalidType && type <= type_counter, MK_ASSERT_STATS_CSTR, "Type index out of bounds!");
		return &Require(infos_ptr)[type];
	}

	bool Valid(const Index type_index)
	{
		return type_index > kInvalidType && type_index <= type_counter;
	}

	bool AddCasters(const Information& info_a, const Information& info_b, const Caster caster_ab, const Caster caster_ba)
	{
		auto& casters = Require(casters_ptr);

		if (casters.size() < info_a.index + 1)
			casters.resize(info_a.index + 1);

		if (casters.size() < info_b.index + 1)
			casters.resize(info_b.index + 1);

		if (casters[info_a.index].size() < info_b.index + 1)
			casters[info_a.index].resize(info_b.index + 1);

		if (casters[info_b.index].size() < info_a.index + 1)
			casters[info_b.index].resize(info_a.index + 1);

		casters[info_a.index][info_b.index] = caster_ab;
		casters[info_b.index][info_a.index] = caster_ba;

		return true;
	}

	bool IsCastableTo(const Information& info_a, const Information& info_b)
	{
		const auto& casters = Require(casters_ptr);
		return Valid(info_a.index) && Valid(info_b.index) && info_a.index < casters.size() && info_b.index < casters[info_a.index].size();
	}

	Caster GetCaster(const Information& info_a, const Information& info_b)
	{
		Program::Assert(IsCastableTo(info_a, info_b), MK_ASSERT_STATS_CSTR, "Cannot cast from A to B!");
		return Require(casters_ptr)[info_a.index][info_b.index];
	}

	bool AddConverters(const Information& info_a, const Information& info_b, const Converter converter_ab, const Converter converter_ba)
	{
		auto& converters = Require(converters_ptr);

		if (converters.size() < info_a.index + 1)
			converters.resize(info_a.index + 1);

		if (converters.size() < info_b.index + 1)
			converters.resize(info_b.index + 1);

		if (converters[info_a.index].size() < info_b.index + 1)
			converters[info_a.index].resize(info_b.index + 1);

		if (converters[info_b.index].size() < info_a.index + 1)
			converters[info_b.index].resize(info_a.index + 1);

		converters[info_a.index][info_b.index] = converter_ab;
		converters[info_b.index][info_a.index] = converter_ba;

		return true;
	}

	bool IsConvertibleTo(const Information& info_a, const Information& info_b)
	{
		const auto& converters = Require(converters_ptr);
		return Valid(info_a.index) && Valid(info_b.index) && info_a.index < converters.size() && info_b.index < converters[info_a.index].size();
	}

	Converter GetConverter(const Information& info_a, const Information& info_b)
	{
		Program::Assert(IsConvertibleTo(info_a, info_b), MK_ASSERT_STATS_CSTR, "Cannot convert from A to B!");
		return Require(converters_ptr)[info_a.index][info_b.index];
	}

	bool AddInheritance(Information& derived_info, const std::vector<Index>& directly_inherited)
	{
		if (directly_inherited.empty())
			return true;

		for (const Index parent : directly_inherited)
		{
			if (!Valid(parent))
				return false;

			if (parent == derived_info.index)
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

			if (!AddInheritance(derived_info, parent_inherited))
				return false;
		}

		return true;
	}

	bool AddConstructor(const Information& info, const Constructor constructor, const FunctionSignature signature)
	{
		auto [iterator, success] = Require(constructors_ptr)[info.index].try_emplace(signature, constructor);
		return success;
	}

	Constructor GetConstructor(const Index type, const FunctionSignature signature)
	{
		auto& constructors = Require(constructors_ptr);

		Program::Assert(Valid(type), MK_ASSERT_STATS_CSTR, "Type index out of bounds!");
		Program::Assert(constructors[type].contains(signature), MK_ASSERT_STATS_CSTR, "No constructor with the specified signature!");
		return constructors[type][signature];
	}

	bool AddDestructor(const Information& info, const Destructor destructor)
	{
		Require(destructors_ptr)[info.index] = destructor;
		return true;
	}

	Destructor GetDestructor(const Index type)
	{
		const auto& destructors = Require(destructors_ptr);

		Program::Assert(Valid(type), MK_ASSERT_STATS_CSTR, "Type index out of bounds!");
		Program::Assert(destructors[type], MK_ASSERT_STATS_CSTR, "No destructor specified!");
		return destructors[type];
	}

	bool AddAssigner(const Information& info, const Assigner assigner, const FunctionSignature signature)
	{
		auto [iterator, success] = Require(assigners_ptr)[info.index].try_emplace(signature, assigner);
		return success;
	}

	Assigner GetAssigner(const Index type, const FunctionSignature signature)
	{
		auto& assigners = Require(assigners_ptr);

		Program::Assert(Valid(type), MK_ASSERT_STATS_CSTR, "Type index out of bounds!");
		Program::Assert(assigners[type].contains(signature), MK_ASSERT_STATS_CSTR, "No assigner with the specified signature!");
		return assigners[type][signature];
	}

	bool AddUnaryOp(const Information& info, const UnaryOperator unary_operator, const UnaryOperation type, const FunctionSignature signature)
	{
		auto& unary_ops = Require(unary_ops_ptr);

		if (unary_ops[info.index].size() < size_t(type) + 1)
			unary_ops[info.index].resize(size_t(type) + 1);

		auto [iterator, success] = unary_ops[info.index][size_t(type)].try_emplace(signature, unary_operator);
		return success;
	}

	bool AddBinaryOp(const Information& info, const BinaryOperator binary_operator, const BinaryOperation type, const FunctionSignature signature)
	{
		auto& binary_ops = Require(binary_ops_ptr);

		if (binary_ops[info.index].size() < size_t(type) + 1)
			binary_ops[info.index].resize(size_t(type) + 1);

		auto [iterator, success] = binary_ops[info.index][size_t(type)].try_emplace(signature, binary_operator);
		return success;
	}

	void DumpInfo()
	{
		static constexpr auto kLabel = L"Meta";

		Program::Log::Std(kLabel) << L"-------------------- Meta --------------------" << std::endl;
		Program::Log::Std(kLabel) << L"~~~~~ Type List ~~~~" << std::endl;

		Index digits = 0;
		Index type_counter_copy = type_counter;

		while (type_counter_copy > 0)
		{
			type_counter_copy /= 10;
			++digits;
		}

		for (const auto& infos = Require(infos_ptr); const Information& info : infos)
		{
			Program::Log::Std(kLabel)
				<< L"Type ID: " << std::setfill(L'0') << std::setw(int(digits)) << info.index
				<< L" | Name: " << info.name;

			if (info.num_bases > 0)
			{
				Program::Log::Std() << L" | Bases: ";

				Index base_count = 0;

				for (size_t base_index = 0; base_index < info.bases.size(); ++base_index)
				{
					if (info.bases[base_index])
					{
						Program::Log::Std() << infos[base_index].name << L" ";

						if (base_count < info.num_bases - 1)
							Program::Log::Std() << L", ";

						++base_count;
					}
				}
			}

			std::cout << std::endl;
		}

		Program::Log::Std(kLabel) << L"~~~~~ Type Stats ~~~~~" << std::endl;
		Program::Log::Std(kLabel) << L"Number of types: " << type_counter << std::endl;

		if (size_t(type_counter) > kPreallocationAmount)
			Program::Log::Std(kLabel) << L"Recommendation: Set kPreallocationAmount to " << type_counter << std::endl;
	}

	View::View(void* ptr, const Information& info, const Qualifier qualifier_flags)
		: data()
		, type(info.index)
		, qualifiers(qualifier_flags)
	{
		*reinterpret_cast<void**>(&data[0]) = ptr;
	}

	View::operator bool() const
	{
		return valid();
	}

	bool View::valid() const
	{
		return is_in_place_primitive() || (Valid(type) && reinterpret_cast<const void*>(data));
	}

	bool View::is(const Information& info, const Qualifier qualifier_flags) const
	{
		if (qualifier_flags != qualifiers && !is_in_place_primitive())
		{
			const bool can_allow_const = ((qualifiers & qualifier_flags) & kQualifier_Constant)  || !(qualifiers & kQualifier_Constant);
			const bool can_allow_ref   = ((qualifiers & qualifier_flags) & kQualifier_Reference) || ((qualifier_flags & kQualifier_Temporary) && (qualifiers & kQualifier_Reference));

			if (!(can_allow_const && can_allow_ref))
				return false;
		}

		if (is_in_place_primitive())
			return get_type() == info.index;

		if (!(Valid(info.index) && valid()))
			return false;

		if (type == info.index)
			return true;

		const Information* my_info = Get(type);
		return size_t(info.index) < my_info->bases.size() && my_info->bases[info.index];
	}

	bool View::is_castable_to(const Information& info) const
	{
		if (!valid())
			return false;
		return IsCastableTo(*Get(get_type()), info);
	}

	bool  View::is_in_place_primitive() const { return type < kInvalidType && type >= kByValue_bool; }

	Index View::get_type() const { return is_in_place_primitive() ? std::abs(type) + kByValue_u8 : type; }

	void* View::internal() const
	{
		if (is_in_place_primitive())
			return const_cast<u8*>(&data[0]);
		return *reinterpret_cast<void**>(const_cast<u8*>(&data[0]));
	}

	Caster View::get_caster(const Information& info_b) const
	{
		Program::Assert(valid(), MK_ASSERT_STATS_CSTR, "No memory to cast!");
		return GetCaster(*Get(get_type()), info_b);
	}

	inline Handle::Handle(const Handle& other)
		: view(other.view)
		, index(other.index)
	{
		if (!view.is_in_place_primitive())
			Memory::get_allocator<Memory::Pool>(view.type)->ref(index);
	}

	Handle::Handle(Handle&& other) noexcept
		: view(other.view)
		, index(other.index)
	{
		other.invalidate();
	}

	Handle::Handle(const Information& info)
	{
		index = Memory::get_allocator<Memory::Pool>(info.index)->alloc(Spandle());
		view = Meta::View(Memory::get_allocator<Memory::Pool>(info.index)->get(index), info, kQualifier_Reference);
	}

	Handle::Handle(const Information& info, const Spandle& arguments)
	{
		index = Memory::get_allocator<Memory::Pool>(info.index)->alloc(arguments);
		view = Meta::View(Memory::get_allocator<Memory::Pool>(info.index)->get(index), info, kQualifier_Reference);
	}

	Handle::Handle(const View v)
		: view(v)
		, index(Memory::kInvalidIndex)
	{}

	Handle::~Handle()
	{
		destroy();
	}

	inline Handle& Handle::operator=(const Handle& other)
	{
		if (this != &other)
		{
			destroy();

			view = other.view;
			index = other.index;

			Memory::get_allocator<Memory::Pool>(view.type)->ref(index);
		}

		return *this;
	}

	Handle& Handle::operator=(Handle&& other) noexcept
	{
		if (this != &other)
		{
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
		return view.valid() && index != Memory::kInvalidIndex;
	}

	bool Handle::is(const Information& info, const Qualifier qualifier_flags) const
	{
		return view.is(info, qualifier_flags);
	}

	View Handle::peek() const
	{
		return view;
	}

	Handle::operator View() const
	{
		return peek();
	}

	bool Handle::is_convertible_to(const Information& info) const
	{
		if (!valid())
			return false;
		return IsConvertibleTo(*Get(view.get_type()), info);
	}

	void Handle::invalidate()
	{
		view = Meta::View();
		index = Memory::kInvalidIndex;
	}

	void Handle::destroy()
	{
		if (view.is_in_place_primitive() || !valid())
			return;

		Memory::get_allocator<Memory::Pool>(view.type)->deref(index);
		invalidate();
	}

	Converter Handle::get_converter(const Information& info_b) const
	{
		Program::Assert(valid(), MK_ASSERT_STATS_CSTR, "No memory to convert!");
		return GetConverter(*Get(view.get_type()), info_b);
	}

	Spandle::~Spandle()
	{
		Memory::get_allocator<Memory::Heap>(Info<Handle>().index)->free(list);
	}

	Spandle Spandle::reserve(const size_t size)
	{
		Spandle result;
		result.allocate(size);
		return result;
	}

	Handle& Spandle::operator[](const Memory::Index index)
	{
		Program::Assert(list.is_valid(index), MK_ASSERT_STATS_CSTR, "Out-of-bounds!");
		auto* const result = static_cast<Handle* const>(Memory::get_allocator<Memory::Heap>(Info<Handle>().index)->get(index));
		Program::Assert(result, MK_ASSERT_STATS_CSTR, "Could not create Handle!");
		return *result;
	}

	Handle Spandle::operator[](const Memory::Index index) const
	{
		Program::Assert(list.is_valid(index), MK_ASSERT_STATS_CSTR, "Out-of-bounds!");
		const auto* const result = static_cast<const Handle* const>(Memory::get_allocator<Memory::Heap>(Info<Handle>().index)->get(index));
		Program::Assert(result, MK_ASSERT_STATS_CSTR, "Could not create Handle!");
		return *result;
	}

	size_t Spandle::size() const
	{
		return list.size();
	}

	bool Spandle::empty() const
	{
		return list.empty();
	}

	void Spandle::allocate(const size_t num_handles)
	{
		if (num_handles > 0)
			list = Memory::get_allocator<Memory::Heap>(Info<Handle>().index)->alloc(num_handles, Spandle());
	}

	FunctionSignature Spandle::get_function_signature(ParameterArray& memory) const
	{
		for (Memory::Index i = 0; i < size(); ++i)
		{
			View view = (*this)[i].view;

			memory[i].first = view.get_type();
			memory[i].second = view.qualifiers;
		}

		return { reinterpret_cast<const char*>(&memory), size() * sizeof(Parameter) };
	}
}
