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

#ifndef META_H
#define META_H

#include <array>
#include <cassert>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include "Math.h"
#include "Program.h"

namespace Meta
{
	// Intentionally not implemented by default to force user to define nameof() function for a type via NAMEOF_DEF
	template<typename T>
	// ReSharper disable once CppFunctionIsNotImplemented
	Program::Name nameof();

	template<typename T>
	T& GetGlobal()
	{
		static T global;
		return &global;
	}

	template<typename T>
	constexpr bool kIsPrimitive =
	   std::is_same_v<std::remove_cvref_t<T>, u8>
	|| std::is_same_v<std::remove_cvref_t<T>, u16>
	|| std::is_same_v<std::remove_cvref_t<T>, u32>
	|| std::is_same_v<std::remove_cvref_t<T>, u64>
	|| std::is_same_v<std::remove_cvref_t<T>, i8>
	|| std::is_same_v<std::remove_cvref_t<T>, i16>
	|| std::is_same_v<std::remove_cvref_t<T>, i32>
	|| std::is_same_v<std::remove_cvref_t<T>, i64>
	|| std::is_same_v<std::remove_cvref_t<T>, f32>
	|| std::is_same_v<std::remove_cvref_t<T>, f64>
	|| std::is_same_v<std::remove_cvref_t<T>, bool>;

	using Index = i32;
	constexpr Index kInvalidType = -1;

	using Qualifier = u8;
	constexpr u8 kQualifier_Temporary = 0b0001;
	constexpr u8 kQualifier_Constant  = 0b0010;
	constexpr u8 kQualifier_Volatile  = 0b0100;
	constexpr u8 kQualifier_Reference = 0b1000;

	template<typename T> requires (!(std::is_pointer_v<T>))
	constexpr auto QualifiersOf = Qualifier(
		  (u8(std::is_rvalue_reference_v<T> || std::is_same_v<T, std::remove_cvref_t<T>>) << u8(0))
		| (u8(std::is_const_v<std::remove_reference_t<T>>) << u8(1))
		| (u8(std::is_volatile_v<std::remove_reference_t<T>>) << u8(2))
		| (u8(std::is_lvalue_reference_v<T>) << u8(3))
		);

	struct Information
	{
		Index index = kInvalidType;
		Program::Name name;
		size_t size;

		std::vector<bool> bases;
		size_t num_bases = 0;
	};

	std::pair<std::vector<Information>*, Index> Register(Program::Name name, size_t size);

	template<typename T>
	const Information& Info()
	{
		using Type = std::remove_pointer_t<std::remove_cvref_t<T>>;
		static const auto infos_pair = Register(nameof<Type>(), sizeof(Type));
		return (*infos_pair.first)[infos_pair.second];
	}

	Index Find(Program::Name name);

	template<typename T>
	Index Find()
	{
		return Find(nameof<T>());
	}

	const Information* GetType(Index type);

	template<typename... Args>
	bool RegistrationSuccessful(Args... args)
	{
		if constexpr (sizeof...(args) > 0)
		{
			([&]()
			{
				Program::Assert(args, MK_ASSERT_STATS_CSTR, "Error in registration process!");

			}(), ...);

			return (... && args);
		}
		return true;
	}

	bool Valid(Index type_index);

	void DumpInfo();

	class View
	{
	public:
		View() = default;

		View(void* ptr, const Information& info, const Qualifier qualifier_flags); // NOLINT(*-avoid-const-params-in-decls)

		View(const View&) = default;
		View(View&&) = default;

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(const T* ptr)
			: View(static_cast<void*>(const_cast<T*>(ptr)), Info<T>(), QualifiersOf<const T&>)
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(T* ptr)
			: View(static_cast<void*>(ptr), Info<T>(), QualifiersOf<T&>)
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(const T& ref)
			: View(&ref)
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(T& ref)
			: View(&ref)
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		View(T&& value) // NOLINT(*-explicit-constructor)
			: View(&value)
		{
			if constexpr (kIsPrimitive<T>)
				*this = value;
			else
				qualifiers = QualifiersOf<T&&>;
		}

		~View() = default;

		View& operator=(const View&) = default;
		View& operator=(View&&) = default;

		template<typename T> requires (kIsPrimitive<T>)
		View& operator=(T&& value)
		{
			type = -Info<T>().index + kByValue_u8;
			qualifiers = QualifiersOf<T&&>;
			*static_cast<std::remove_cvref_t<T>*>(static_cast<void*>(&data[0])) = value;
			return *this;
		}

		explicit operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(const Information& info, const Qualifier qualifier_flags) const; // NOLINT(*-avoid-const-params-in-decls)

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Info<T>(), QualifiersOf<T>);
		}

		template<typename T>
		std::remove_cvref_t<T>* raw() const
		{
			Program::Assert(valid(), MK_ASSERT_STATS_CSTR, "Not a valid View!");
			Program::Assert(is<T>(), MK_ASSERT_STATS_CSTR, "Not the correct type!");

			if constexpr (kIsPrimitive<T>)
			{
				if (is_in_place_primitive())
					return static_cast<std::remove_cvref_t<T>*>(static_cast<void*>(const_cast<u8*>(&data[0])));
			}
			return static_cast<std::remove_cvref_t<T>*>(*reinterpret_cast<void**>(const_cast<u8*>(&data[0])));
		}

		template<typename T>
		T as() const
		{
			if constexpr (std::is_pointer_v<T>)
				return raw<std::remove_pointer_t<T>>();
			else if constexpr (std::is_rvalue_reference_v<T>)
				return std::forward<T>(*raw<T>());
			else
				return *raw<T>();
		}

		template<typename T> requires (kIsPrimitive<T>)
		T primitive() const
		{
			Program::Assert(is_in_place_primitive(), MK_ASSERT_STATS_CSTR, "Not an in-place primitive View!");
			return as<T>();
		}

		[[nodiscard]] bool is_castable_to(const Information& info) const;

		template<typename U>
		[[nodiscard]] bool is_castable_to() const
		{
			return is_castable_to(Info<U>());
		}

		template<typename U>
		[[nodiscard]] View cast_to() const
		{
			Program::Assert(is_castable_to<U>(), MK_ASSERT_STATS_CSTR, "Cannot cast to this type!");
			return get_caster(Info<U>())(*this);
		}

	private:
		using Caster = View (*)(const View);

		static constexpr Index kByValue_u8 = kInvalidType - 1;
		static constexpr Index kByValue_u16 = kByValue_u8 - 1;
		static constexpr Index kByValue_u32 = kByValue_u16 - 1;
		static constexpr Index kByValue_u64 = kByValue_u32 - 1;
		static constexpr Index kByValue_i8 = kByValue_u64 - 1;
		static constexpr Index kByValue_i16 = kByValue_i8 - 1;
		static constexpr Index kByValue_i32 = kByValue_i16 - 1;
		static constexpr Index kByValue_i64 = kByValue_i32 - 1;
		static constexpr Index kByValue_f32 = kByValue_i64 - 1;
		static constexpr Index kByValue_f64 = kByValue_f32 - 1;
		static constexpr Index kByValue_bool = kByValue_f64 - 1;

		u8 data[(std::max)({
			  sizeof(u8)
			, sizeof(u16)
			, sizeof(u32)
			, sizeof(u64)
			, sizeof(i8)
			, sizeof(i16)
			, sizeof(i32)
			, sizeof(i64)
			, sizeof(f32)
			, sizeof(f64)
			, sizeof(bool)
			, sizeof(void*)
		})] = { 0 };
		Index type = kInvalidType;
		Qualifier qualifiers = kQualifier_Temporary;

		[[nodiscard]] bool is_in_place_primitive() const;
		[[nodiscard]] Index get_type() const;
		[[nodiscard]] void* internal() const;
		[[nodiscard]] Caster get_caster(const Information& info_b) const;

		friend class Handle;
		friend class Spandle;

		template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
		friend Caster FromCaster();
	};

	bool AddSingleton(const Information& info, const View view); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<T>>)
	bool AddSingleton()
	{
		return AddSingleton(Info<T>(), View(GetGlobal<T>()));
	}

	View GetSingleton(const Information& info);

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<T>>)
	View GetSingleton()
	{
		return GetSingleton(Info<T>());
	}
}

namespace Memory
{
	using Index = i64;
	constexpr Index kInvalidIndex = -1;

	struct Range
	{
		Index start = kInvalidIndex;
		Index end = kInvalidIndex;

		[[nodiscard]] size_t size() const { return size_t(end - start); }
		[[nodiscard]] bool empty() const { return start == end; }
		[[nodiscard]] bool is_valid(const Index index) const { return start + index >= start && start + index < end; }
	};

	struct RangeComparator
	{
		bool operator()(const Range& lhs, const Range& rhs) const
		{
			return lhs.size() < rhs.size();
		}
	};
}

namespace Meta
{
	using Parameter = std::pair<Index, Qualifier>;
	constexpr u64 kMaximumParameters = 256;

	using ParameterArray = std::array<Parameter, kMaximumParameters>;
	using FunctionSignature = std::string_view;

	class Handle
	{
	public:
		Handle() = default;
		Handle(const Handle& other);
		Handle(Handle&& other) noexcept;

		explicit Handle(const Information& info);
		explicit Handle(const Information& info, const class Spandle& arguments);
		explicit Handle(View v);

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		explicit Handle(const T* value)
			: view(View(value))
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		explicit Handle(T* value)
			: view(View(value))
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		explicit Handle(const T& value)
			: Handle(Meta::Info<T>(), Spandle(&value))
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		Handle(T&& value) // NOLINT(*-explicit-constructor)
			: Handle()
		{
			if constexpr (kIsPrimitive<T>)
				view = value;
			else
				*this = Handle(Info<T>(), Spandle(&value));
		}

		template<typename T>
		static Handle Create(const Spandle& arguments)
		{
			return Handle(Meta::Info<T>(), arguments);
		}

		~Handle();

		Handle& operator=(const Handle& other);
		Handle& operator=(Handle&& other) noexcept;

		template<typename T> requires (kIsPrimitive<T>)
		Handle& operator=(T&& value)
		{
			destroy();
			view = value;
			return *this;
		}

		explicit operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(const Information& info, const Qualifier qualifier_flags) const; // NOLINT(*-avoid-const-params-in-decls)

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Meta::Info<T>(), QualifiersOf<T>);
		}

		template<typename T>
		[[nodiscard]] T as() const
		{
			Program::Assert(is<T>(), MK_ASSERT_STATS_CSTR, "Not the correct type!");
			return view.as<T>();
		}

		template<typename T> requires (kIsPrimitive<T>)
		T primitive() const { return view.primitive<T>(); }

		[[nodiscard]] View peek() const;

		explicit operator View() const;

		[[nodiscard]] bool is_convertible_to(const Information& info) const;

		template<typename U>
		[[nodiscard]] bool is_convertible_to() const
		{
			return is_convertible_to(Info<U>());
		}

		template<typename U>
		[[nodiscard]] Handle convert_to() const
		{
			return get_converter(Info<U>())(view);
		}

	private:
		using Converter = Handle (*)(const View);

		View view = View();
		Memory::Index index = Memory::kInvalidIndex;

		void invalidate();
		void destroy();
		[[nodiscard]] Converter get_converter(const Information& info_b) const;

		friend Spandle;
	};

	class Spandle
	{
	public:
		Spandle() = default;

		template<typename... Args>
		explicit Spandle(Args... args)
		{
			// ReSharper disable once CppDFAUnreadVariable
			Memory::Index index = 0;

			allocate(sizeof...(Args));

			([&]()
			{
				if constexpr (kIsPrimitive<Args>)
					(*this)[index] = args;
				else
					(*this)[index] = Handle(args);

				++index;

			}(), ...);
		}

		Spandle(const Spandle&) = default;
		Spandle(Spandle&&) = default;
		~Spandle();

		static Spandle reserve(const size_t size); // NOLINT(*-avoid-const-params-in-decls)

		Spandle& operator=(const Spandle&) = default;
		Spandle& operator=(Spandle&&) = default;

		Handle& operator[](Memory::Index index);
		Handle  operator[](Memory::Index index) const;

		[[nodiscard]] size_t size() const;
		[[nodiscard]] bool empty() const;

		[[nodiscard]] FunctionSignature get_function_signature(ParameterArray& memory) const;

	private:
		Memory::Range list;
		void allocate(const size_t num_handles); // NOLINT(*-avoid-const-params-in-decls)
	};

	// -----------------------------------------------------------------------------------------------------------------
	// Conversion
	// -----------------------------------------------------------------------------------------------------------------

	using Caster = View (*)(const View);
	using Converter = Handle (*)(const View);

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Caster FromCaster()
	{
		return [](const View view) -> View
		{
			return View(view.internal(), Info<T>(), view.qualifiers);
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Caster kCasterOf = FromCaster<T>();

	bool AddCaster(const Information& info_a, const Information& info_b, const Caster caster_ab); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddCaster()
	{
		return AddCaster(Info<T>(), Info<U>(), kCasterOf<U>);
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddTwoWayCast()
	{
		return AddCaster(Info<T>(), Info<U>(), kCasterOf<U>) && AddCaster(Info<U>(), Info<T>(), kCasterOf<T>);
	}

	bool IsCastableTo(const Information& info_a, const Information& info_b);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool IsCastableTo()
	{
		return IsCastableTo(Info<T>(), Info<U>());
	}

	Caster GetCaster(const Information& info_a, const Information& info_b);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	Caster GetCaster()
	{
		return GetCaster(Info<T>(), Info<U>());
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	Converter FromConverter()
	{
		return [](const View view) -> Handle
		{
			return Handle(T(view.as<const U&>()));
		};
	}

	bool AddConverter(const Information& info_a, const Information& info_b, const Converter converter_ab); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddConverter()
	{
		return AddConverter(Info<T>(), Info<U>(), FromConverter<T, U>());
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddTwoWayConversion()
	{
		return AddConverter(Info<T>(), Info<U>(), FromConverter<T, U>()) && AddConverter(Info<U>(), Info<T>(), FromConverter<U, T>());
	}

	bool IsConvertibleTo(const Information& info_a, const Information& info_b);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool IsConvertibleTo()
	{
		return IsConvertibleTo(Info<T>(), Info<U>());
	}

	Converter GetConverter(const Information& info_a, const Information& info_b);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	Converter GetConverter()
	{
		return GetConverter(Info<T>(), Info<U>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Inheritance
	// -----------------------------------------------------------------------------------------------------------------

	bool AddInheritance(Information& derived_info, const std::vector<Index>& directly_inherited);

	template<typename T, typename... DirectlyInherited>
	bool AddInheritance()
	{
		const std::vector<Index> directly_inherited{ ((Info<DirectlyInherited>().index), ...) };
		return AddInheritance(const_cast<Information&>(Info<T>()), directly_inherited);
	}

	template<typename... Args>
	FunctionSignature FromParameterList()
	{
		static_assert(sizeof...(Args) <= kMaximumParameters, "Too many parameters passed!");

		static ParameterArray params       = { Parameter() };
		static const size_t signature_size = [&]() -> size_t
		{
			size_t index = 0;

			([&]()
			{
				params[index].first = Info<Args>().index;
				params[index].second = QualifiersOf<Args>;
				++index;

			}(), ...);

			return index;
		}();
		static const auto laundered = static_cast<const char*>(static_cast<const void*>(params.data()));
		static const auto signature = FunctionSignature(laundered, signature_size * sizeof(Parameter));

		return signature;
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Methods
	// -----------------------------------------------------------------------------------------------------------------

	using Method = Handle (*)(const View, const Spandle&);

	template<typename U>
	Handle MapTo(const Handle& handle)
	{
		// First, we prefer that this handle is the given type.
		if (handle.is<U>())
			return handle;

		// Then we prefer to safely convert to the given type by a conversion construction.
		if (handle.is_convertible_to<U>())
			return handle.convert_to<U>();

		// Then we prefer a direct cast as a last resort.
		if (handle.peek().is_castable_to<U>())
			return Handle(handle.peek().cast_to<U>());

		// Otherwise this mapping is ill-formed.
		Program::Assert(false, MK_ASSERT_STATS_CSTR, "Cannot map this handle to the given type!");
		return {};
	}

	template<typename T, auto MethodPtr, typename Return, bool IsConst, typename Tuple, std::size_t... Index> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Method FromMethodImpl(std::index_sequence<Index...>)
	{
		return [](const View view, const Spandle& parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Index) > 0)
			{
				Program::Assert(parameters.size() == sizeof...(Index), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
				{
					if constexpr (IsConst)
					{
						if (view.is<const T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<const T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
						else if (view.is<const T&>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<const T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
						else if (view.is<T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
						else
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
					}
					else
					{
						if (view.is<T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
						else
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							std::invoke(MethodPtr, view.as<T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
					}
				}
				else
				{
					if constexpr (IsConst)
					{
						if (view.is<const T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<const T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
						else if (view.is<const T&>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<const T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
						else if (view.is<T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
						else
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
					}
					else
					{
						if (view.is<T>())
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<T>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
						else
							// ReSharper is wrong, template dependent name keyword is required
							// ReSharper disable once CppRedundantTemplateKeyword
							result = Handle(std::invoke(MethodPtr, view.as<T&>(), MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
					}
				}
			}
			else
			{
				Program::Assert(parameters.empty(), MK_ASSERT_STATS_CSTR, "Method takes no parameters!");

				if constexpr (std::is_void_v<Return>)
				{
					if constexpr (IsConst)
					{
						if (view.is<const T>())
							(view.as<const T>().*MethodPtr)();
						else if (view.is<const T&>())
							(view.as<const T&>().*MethodPtr)();
						else if (view.is<T>())
							(view.as<T>().*MethodPtr)();
						else
							(view.as<T&>().*MethodPtr)();
					}
					else
					{
						if (view.is<T>())
							(view.as<T>().*MethodPtr)();
						else
							(view.as<T&>().*MethodPtr)();
					}
				}
				else
				{
					if constexpr (IsConst)
					{
						if (view.is<const T>())
							result = Handle((view.as<const T>().*MethodPtr)());
						else if (view.is<const T&>())
							result = Handle((view.as<const T&>().*MethodPtr)());
						else if (view.is<T>())
							result = Handle((view.as<T>().*MethodPtr)());
						else
							result = Handle((view.as<T&>().*MethodPtr)());
					}
					else
					{
						if (view.is<T>())
							result = Handle((view.as<T>().*MethodPtr)());
						else
							result = Handle((view.as<T&>().*MethodPtr)());
					}
				}
			}

			return result;
		};
	}

	template<typename T, auto MethodPtr, typename Return, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Method FromMethod()
	{
		static_assert(std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...)> || std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...) const>, "MethodPtr is not a valid method pointer!");
		return FromMethodImpl<T, MethodPtr, Return, std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...) const>, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Functions
	// -----------------------------------------------------------------------------------------------------------------

	using Function = Handle (*)(const Spandle&);

	template<auto FunctionPtr, typename Return, typename Tuple, std::size_t... Index>
	Function FromFunctionImpl(std::index_sequence<Index...>)
	{
		return [](const Spandle& parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Index) > 0)
			{
				Program::Assert(parameters.size() == sizeof...(Index), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					// ReSharper is wrong, template dependent name keyword is required
					// ReSharper disable once CppRedundantTemplateKeyword
					(FunctionPtr)(MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
				else
					// ReSharper is wrong, template dependent name keyword is required
					// ReSharper disable once CppRedundantTemplateKeyword
					result = Handle((FunctionPtr)(MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...));
			}
			else
			{
				Program::Assert(parameters.empty(), MK_ASSERT_STATS_CSTR, "Method takes no parameters!");

				if constexpr (std::is_void_v<Return>)
					(FunctionPtr)();
				else
					result = Handle((FunctionPtr)());
			}

			return result;
		};
	}

	template<auto FunctionPtr, typename Return, typename... Args>
	Function FromFunction()
	{
		static_assert(std::is_same_v<decltype(FunctionPtr), Return (*)(Args...)>, "FunctionPtr is not a valid function pointer!");
		return FromFunctionImpl<FunctionPtr, Return, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Members
	// -----------------------------------------------------------------------------------------------------------------

	using Member = Handle (*)(const View);

	template<auto MemberPtr, typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Member FromMemberImpl()
	{
		return [](const View view) -> Handle
		{
			if (view.is<T&>())
				return Handle(view.as<T&>().*MemberPtr);
			if (view.is<const T>())
				return Handle(view.as<const T>().*MemberPtr);
			if (view.is<const T&>())
				return Handle(view.as<const T&>().*MemberPtr);
			return Handle(view.as<T>().*MemberPtr);
		};
	}

	template<typename T, auto MemberPtr, typename Return> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Member FromMember()
	{
		static_assert(std::is_same_v<decltype(MemberPtr), Return (T::*)>, "MemberPtr is not a valid member pointer!");
		return FromMemberImpl<MemberPtr, T>();
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------------------------------------------------------

	using Constructor = void (*)(const View, const Spandle&);

	template<typename T, typename Tuple, std::size_t... Index> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Constructor FromCtorImpl(std::index_sequence<Index...>)
	{
		return [](const View view, const Spandle& parameters) -> void
		{
			if constexpr (sizeof...(Index) > 0)
			{
				Program::Assert(parameters.size() == sizeof...(Index), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");
				// ReSharper is wrong, template dependent name keyword is required
				// ReSharper disable once CppRedundantTemplateKeyword
				new (view.raw<T&>()) T(MapTo<std::tuple_element_t<Index, Tuple>>(parameters[Index]).template as<std::tuple_element_t<Index, Tuple>>()...);
			}
			else
			{
				Program::Assert(parameters.empty(), MK_ASSERT_STATS_CSTR, "Constructor takes no parameters!");
				new (view.raw<T&>()) T;
			}
		};
	}

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Constructor FromCtor()
	{
		return FromCtorImpl<T, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	bool AddConstructor(const Information& info, const Constructor constructor, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddConstructor()
	{
		return AddConstructor(Info<T>(), FromCtor<T, Args...>(), FromParameterList<Args...>());
	}

	Constructor GetConstructor(const Information& info, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Constructor GetConstructor()
	{
		return GetConstructor(Info<T>(), FromParameterList<Args...>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Destructors
	// -----------------------------------------------------------------------------------------------------------------

	using Destructor = void (*)(const View);

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Destructor FromDtor()
	{
		return [](const View view) -> void
		{
			view.as<T&>().~T();
		};
	}

	bool AddDestructor(const Information& info, const Destructor destructor); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddDestructor()
	{
		return AddDestructor(Info<T>(), FromDtor<T>());
	}

	Destructor GetDestructor(const Information& info); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Destructor GetDestructor()
	{
		return GetDestructor(Info<T>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Assigners
	// -----------------------------------------------------------------------------------------------------------------

	using Assigner = View (*)(const View, const Spandle&);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	Assigner FromAssignOp()
	{
		return [](const View view, const Spandle& parameters) -> View
		{
			Program::Assert(parameters.size() == size_t(1), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");
			view.as<T&>() = parameters[0].as<U>();
			return view;
		};
	}

	bool AddAssigner(const Information& info, const Assigner assigner, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddAssigner()
	{
		return AddAssigner(Info<T>(), FromAssignOp<T, U>(), FromParameterList<U>());
	}

	Assigner GetAssigner(const Information& info, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args>
	Assigner GetAssigner()
	{
		return GetAssigner(Info<T>(), FromParameterList<Args...>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Logic
	// -----------------------------------------------------------------------------------------------------------------

	using UnaryOperator  = Handle (*)(const View);
	using BinaryOperator = Handle (*)(const View, const View);

	// Math

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromPositive()
	{
		return [](const View view) -> Handle
		{
			return Handle(+view.as<T>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromNegative()
	{
		return [](const View view) -> Handle
		{
			return Handle(-view.as<T>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromPrefixIncrement()
	{
		return [](const View view) -> Handle
		{
			++view.as<T&>();
			return Handle(view);
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromPostfixIncrement()
	{
		return [](const View view) -> Handle
		{
			return Handle(view.as<T&>()++);
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromPrefixDecrement()
	{
		return [](const View view) -> Handle
		{
			--view.as<T&>();
			return Handle(view);
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromPostfixDecrement()
	{
		return [](const View view) -> Handle
		{
			return Handle(view.as<T&>()--);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromAddEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() += T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromAdd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() + T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromSubEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() -= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromSub()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() - T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromMulEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() *= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromMul()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() * T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromDivEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() /= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromDiv()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() / T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromModEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() %= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromMod()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() % T(b.as<U>()));
		};
	}

	// Bit Manipulation

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromBitwiseNot()
	{
		return [](const View view) -> Handle
		{
			return Handle(~view.as<T>());
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseAndEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() &= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseAnd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() & T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseOrEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() |= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseOr()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() | T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseXorEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() ^= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseXor()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() ^ T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseLeftShiftEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() <<= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseLeftShift()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() << T(b.as<U>()));
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseRightShiftEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() >>= T(b.as<U>());
			return Handle(a);
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromBitwiseRightShift()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() >> T(b.as<U>()));
		};
	}

	// Unary Conditions

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	UnaryOperator FromLogicalNot()
	{
		return [](const View a) -> Handle
		{
			return Handle(!a.as<const T&>());
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromLogicalAnd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() && b.as<const U&>());
		};
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	BinaryOperator FromLogicalOr()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() || b.as<const U&>());
		};
	}

	// Comparison Conditions

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() == b.as<const T&>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromNotEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() != b.as<const T&>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromLessThan()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() < b.as<const T&>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromLessThanOrEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() <= b.as<const T&>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromGreaterThan()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() > b.as<const T&>());
		};
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	BinaryOperator FromGreaterThanOrEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() >= b.as<const T&>());
		};
	}

	enum UnaryOperation : u8
	{
		kUnaryOperation_PrefixIncrement,
		kUnaryOperation_PrefixDecrement,
		kUnaryOperation_PostfixIncrement,
		kUnaryOperation_PostfixDecrement,

		kUnaryOperation_Positive,
		kUnaryOperation_Negative,
		kUnaryOperation_BitwiseNot,

		kUnaryOperation_LogicalNot,

		kUnaryOperation_Count,
		kUnaryOperation_Initial = kUnaryOperation_PrefixIncrement
	};

	enum BinaryOperation : u8
	{
		kBinaryOperation_AddEquals,
		kBinaryOperation_SubEquals,
		kBinaryOperation_MulEquals,
		kBinaryOperation_DivEquals,
		kBinaryOperation_ModEquals,
		kBinaryOperation_BitwiseAndEquals,
		kBinaryOperation_BitwiseOrEquals,
		kBinaryOperation_BitwiseXorEquals,
		kBinaryOperation_BitwiseLeftShiftEquals,
		kBinaryOperation_BitwiseRightShiftEquals,

		kBinaryOperation_Add,
		kBinaryOperation_Sub,
		kBinaryOperation_Mul,
		kBinaryOperation_Div,
		kBinaryOperation_Mod,
		kBinaryOperation_BitwiseAnd,
		kBinaryOperation_BitwiseOr,
		kBinaryOperation_BitwiseXor,
		kBinaryOperation_BitwiseLeftShift,
		kBinaryOperation_BitwiseRightShift,

		kBinaryOperation_LogicalAnd,
		kBinaryOperation_LogicalOr,

		kComparisonOperation_Equals,
		kComparisonOperation_NotEquals,
		kComparisonOperation_LessThan,
		kComparisonOperation_LessThanOrEquals,
		kComparisonOperation_GreaterThan,
		kComparisonOperation_GreaterThanOrEquals,

		kBinaryOperation_Count,
		kBinaryOperation_Initial = kBinaryOperation_AddEquals,

		kComparisonOperation_Initial = kComparisonOperation_Equals,
		kComparisonOperation_Final = kComparisonOperation_GreaterThanOrEquals
	};

	bool AddUnaryOp(const Information& info, const UnaryOperator unary_operator, const UnaryOperation type, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)
	bool AddBinaryOp(const Information& info, const BinaryOperator binary_operator, const BinaryOperation type, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, UnaryOperation Op> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddUnaryOp()
	{
		static_assert(Op >= kUnaryOperation_Initial && Op < kUnaryOperation_Count, "Not a valid unary operator!");

		if constexpr (Op == kUnaryOperation_PrefixIncrement)
			return AddUnaryOp(Info<T>(), FromPrefixIncrement<T>(), kUnaryOperation_PrefixIncrement, FromParameterList<T&>());
		else if constexpr (Op == kUnaryOperation_PrefixDecrement)
			return AddUnaryOp(Info<T>(), FromPrefixDecrement<T>(), kUnaryOperation_PrefixDecrement, FromParameterList<T&>());
		else if constexpr (Op == kUnaryOperation_PostfixIncrement)
			return AddUnaryOp(Info<T>(), FromPostfixIncrement<T>(), kUnaryOperation_PostfixIncrement, FromParameterList<T&>());
		else if constexpr (Op == kUnaryOperation_PostfixDecrement)
			return AddUnaryOp(Info<T>(), FromPostfixDecrement<T>(), kUnaryOperation_PostfixDecrement, FromParameterList<T&>());
		else if constexpr (Op == kUnaryOperation_Positive)
			return AddUnaryOp(Info<T>(), FromPositive<T>(), kUnaryOperation_Positive, FromParameterList<T>());
		else if constexpr (Op == kUnaryOperation_Negative)
			return AddUnaryOp(Info<T>(), FromNegative<T>(), kUnaryOperation_Negative, FromParameterList<T>());
		else if constexpr (Op == kUnaryOperation_BitwiseNot)
			return AddUnaryOp(Info<T>(), FromBitwiseNot<T>(), kUnaryOperation_BitwiseNot, FromParameterList<T>());
		else if constexpr (Op == kUnaryOperation_LogicalNot)
			return AddUnaryOp(Info<T>(), FromLogicalNot<T>(), kUnaryOperation_LogicalNot, FromParameterList<const T&>());
		else
			return false;
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddAllUnaryOps()
	{
		return AddUnaryOp<T, kUnaryOperation_PrefixIncrement>()
			&& AddUnaryOp<T, kUnaryOperation_PrefixDecrement>()
			&& AddUnaryOp<T, kUnaryOperation_PostfixIncrement>()
			&& AddUnaryOp<T, kUnaryOperation_PostfixDecrement>()
			&& AddUnaryOp<T, kUnaryOperation_Positive>()
			&& AddUnaryOp<T, kUnaryOperation_Negative>()
			&& AddUnaryOp<T, kUnaryOperation_BitwiseNot>()
			&& AddUnaryOp<T, kUnaryOperation_LogicalNot>();
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddAllFloatUnaryOps()
	{
		return AddUnaryOp<T, kUnaryOperation_PrefixIncrement>()
			&& AddUnaryOp<T, kUnaryOperation_PrefixDecrement>()
			&& AddUnaryOp<T, kUnaryOperation_PostfixIncrement>()
			&& AddUnaryOp<T, kUnaryOperation_PostfixDecrement>()
			&& AddUnaryOp<T, kUnaryOperation_Positive>()
			&& AddUnaryOp<T, kUnaryOperation_Negative>()
			&& AddUnaryOp<T, kUnaryOperation_LogicalNot>();
	}

	template<typename T, typename U, BinaryOperation Op> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddBinaryOp()
	{
		static_assert(Op >= kBinaryOperation_Initial && Op < kComparisonOperation_Initial, "Not a valid binary operator!");

		if constexpr (Op == kBinaryOperation_AddEquals)
			return AddBinaryOp(Info<T>(), FromAddEquals<T, U>(), kBinaryOperation_AddEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_SubEquals)
			return AddBinaryOp(Info<T>(), FromSubEquals<T, U>(), kBinaryOperation_SubEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_MulEquals)
			return AddBinaryOp(Info<T>(), FromMulEquals<T, U>(), kBinaryOperation_MulEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_DivEquals)
			return AddBinaryOp(Info<T>(), FromDivEquals<T, U>(), kBinaryOperation_DivEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_ModEquals)
			return AddBinaryOp(Info<T>(), FromModEquals<T, U>(), kBinaryOperation_ModEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseAndEquals)
			return AddBinaryOp(Info<T>(), FromBitwiseAndEquals<T, U>(), kBinaryOperation_BitwiseAndEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseOrEquals)
			return AddBinaryOp(Info<T>(), FromBitwiseOrEquals<T, U>(), kBinaryOperation_BitwiseOrEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseXorEquals)
			return AddBinaryOp(Info<T>(), FromBitwiseXorEquals<T, U>(), kBinaryOperation_BitwiseXorEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseLeftShiftEquals)
			return AddBinaryOp(Info<T>(), FromBitwiseLeftShiftEquals<T, U>(), kBinaryOperation_BitwiseLeftShiftEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseRightShiftEquals)
			return AddBinaryOp(Info<T>(), FromBitwiseRightShiftEquals<T, U>(), kBinaryOperation_BitwiseRightShiftEquals, FromParameterList<T&, U>());
		else if constexpr (Op == kBinaryOperation_Add)
			return AddBinaryOp(Info<T>(), FromAdd<T, U>(), kBinaryOperation_Add, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_Sub)
			return AddBinaryOp(Info<T>(), FromSub<T, U>(), kBinaryOperation_Sub, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_Mul)
			return AddBinaryOp(Info<T>(), FromMul<T, U>(), kBinaryOperation_Mul, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_Div)
			return AddBinaryOp(Info<T>(), FromDiv<T, U>(), kBinaryOperation_Div, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_Mod)
			return AddBinaryOp(Info<T>(), FromMod<T, U>(), kBinaryOperation_Mod, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseAnd)
			return AddBinaryOp(Info<T>(), FromBitwiseAnd<T, U>(), kBinaryOperation_BitwiseAnd, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseOr)
			return AddBinaryOp(Info<T>(), FromBitwiseOr<T, U>(), kBinaryOperation_BitwiseOr, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseXor)
			return AddBinaryOp(Info<T>(), FromBitwiseXor<T, U>(), kBinaryOperation_BitwiseXor, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseLeftShift)
			return AddBinaryOp(Info<T>(), FromBitwiseLeftShift<T, U>(), kBinaryOperation_BitwiseLeftShift, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_BitwiseRightShift)
			return AddBinaryOp(Info<T>(), FromBitwiseRightShift<T, U>(), kBinaryOperation_BitwiseRightShift, FromParameterList<T, U>());
		else if constexpr (Op == kBinaryOperation_LogicalAnd)
			return AddBinaryOp(Info<T>(), FromLogicalAnd<T, U>(), kBinaryOperation_LogicalAnd, FromParameterList<const T&, const U&>());
		else if constexpr (Op == kBinaryOperation_LogicalOr)
			return AddBinaryOp(Info<T>(), FromLogicalOr<T, U>(), kBinaryOperation_LogicalOr, FromParameterList<const T&, const U&>());
		else
			return false;
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddAllFloatMathOps()
	{
		return AddBinaryOp<T, U, kBinaryOperation_AddEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_SubEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_MulEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_DivEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_Add>()
			&& AddBinaryOp<T, U, kBinaryOperation_Sub>()
			&& AddBinaryOp<T, U, kBinaryOperation_Mul>()
			&& AddBinaryOp<T, U, kBinaryOperation_Div>();
	}

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>> && std::is_same_v<U, std::remove_pointer_t<std::remove_cvref_t<U>>>)
	bool AddAllBinaryOps()
	{
		return AddBinaryOp<T, U, kBinaryOperation_AddEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_SubEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_MulEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_DivEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_ModEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseAndEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseOrEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseXorEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseLeftShiftEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseRightShiftEquals>()
			&& AddBinaryOp<T, U, kBinaryOperation_Add>()
			&& AddBinaryOp<T, U, kBinaryOperation_Sub>()
			&& AddBinaryOp<T, U, kBinaryOperation_Mul>()
			&& AddBinaryOp<T, U, kBinaryOperation_Div>()
			&& AddBinaryOp<T, U, kBinaryOperation_Mod>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseAnd>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseOr>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseXor>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseLeftShift>()
			&& AddBinaryOp<T, U, kBinaryOperation_BitwiseRightShift>();
	}

	template<typename T, BinaryOperation Op> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddComparisonOp()
	{
		static_assert(Op >= kComparisonOperation_Initial && Op <= kComparisonOperation_Final, "Not a valid comparison operator!");

		if constexpr (Op == kComparisonOperation_Equals)
			return AddBinaryOp(Info<T>(), FromEquals<T>(), kComparisonOperation_Equals, FromParameterList<const T&, const T&>());
		else if constexpr (Op == kComparisonOperation_NotEquals)
			return AddBinaryOp(Info<T>(), FromNotEquals<T>(), kComparisonOperation_NotEquals, FromParameterList<const T&, const T&>());
		else if constexpr (Op == kComparisonOperation_LessThan)
			return AddBinaryOp(Info<T>(), FromLessThan<T>(), kComparisonOperation_LessThan, FromParameterList<const T&, const T&>());
		else if constexpr (Op == kComparisonOperation_LessThanOrEquals)
			return AddBinaryOp(Info<T>(), FromLessThanOrEquals<T>(), kComparisonOperation_LessThanOrEquals, FromParameterList<const T&, const T&>());
		else if constexpr (Op == kComparisonOperation_GreaterThan)
			return AddBinaryOp(Info<T>(), FromGreaterThan<T>(), kComparisonOperation_GreaterThan, FromParameterList<const T&, const T&>());
		else if constexpr (Op == kComparisonOperation_GreaterThanOrEquals)
			return AddBinaryOp(Info<T>(), FromGreaterThanOrEquals<T>(), kComparisonOperation_GreaterThanOrEquals, FromParameterList<const T&, const T&>());
		else
			return false;
	}

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	bool AddAllComparisonOps()
	{
		return AddComparisonOp<T, kComparisonOperation_Equals>()
			&& AddComparisonOp<T, kComparisonOperation_NotEquals>()
			&& AddComparisonOp<T, kComparisonOperation_LessThan>()
			&& AddComparisonOp<T, kComparisonOperation_LessThanOrEquals>()
			&& AddComparisonOp<T, kComparisonOperation_GreaterThan>()
			&& AddComparisonOp<T, kComparisonOperation_GreaterThanOrEquals>();
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Registration Helpers
	// -----------------------------------------------------------------------------------------------------------------

	template<typename T>
	bool AddPOD()
	{
		return AddConstructor<T>()
			&& AddConstructor<T, const T&>()
			&& AddConstructor<T, T&&>()
			&&  AddDestructor<T>()
			&&    AddAssigner<T, const T&>()
			&&    AddAssigner<T, T&&>();
	}

	template<typename T>
	bool AddPrimitiveIntegralType()
	{
		return AddPOD<T>()
			&& AddAllUnaryOps<T>()
			&& AddAllBinaryOps<T, T>()
			&& AddAllComparisonOps<T>();
	}

	template<typename T>
	bool AddPrimitiveFloatType()
	{
		return AddPOD<T>()
			&& AddAllFloatUnaryOps<T>()
			&& AddAllFloatMathOps<T, T>()
			&& AddAllComparisonOps<T>();
	}
}

#define NAMEOF_DEF(type) \
namespace Meta \
{ \
	template<> \
	Program::Name nameof<type>() \
	{ \
		static constexpr auto name = L###type; \
		static constexpr auto size = sizeof(#type) - 1; \
		return Program::Name(name, size); \
	} \
}

#define META_TYPE(type, ...) \
NAMEOF_DEF(type) \
namespace Meta \
{ \
	static const bool k##type##Success = []() -> bool \
		{ \
			Meta::Index MetaIndex = Meta::Info<type>().index; \
			using Type = std::remove_cvref_t<type>; \
			bool result = Meta::RegistrationSuccessful(__VA_ARGS__); \
			Program::Assert(result, MK_ASSERT_STATS_CSTR, "Error in meta registration process!"); \
			return result; \
		}(); \
}

#define META_TYPE_AS(type, alt_name, ...) using alt_name = type; META_TYPE(alt_name, __VA_ARGS__)

#endif
