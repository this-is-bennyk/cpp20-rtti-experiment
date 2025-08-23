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
	extern Program::Name nameof();

	template<typename T>
	T& GetGlobal()
	{
		static T global;
		return &global;
	}

	template<typename T>
	static constexpr bool kIsPrimitive =
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
	static constexpr Index kInvalidType = -1;

	using Qualifier = u8;
	static constexpr u8 kQualifier_Temporary = 0b0001;
	static constexpr u8 kQualifier_Constant  = 0b0010;
	static constexpr u8 kQualifier_Volatile  = 0b0100;
	static constexpr u8 kQualifier_Reference = 0b1000;

	template<typename T> requires (!(std::is_pointer_v<T>))
	static constexpr auto QualifiersOf = Qualifier(
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

	extern const Information& Register(Program::Name name, size_t size);

	template<typename T>
	static const Information& Info()
	{
		using Type = std::remove_cvref_t<T>;
		static const Information& info = Register(nameof<Type>(), sizeof(Type));
		return info;
	}

	static Index Find(Program::Name name);

	template<typename T>
	static Index Find()
	{
		return Find(nameof<T>());
	}

	static const Information* Get(Index type);

	template<typename... Args>
	static bool RegistrationSuccessful(Args... args)
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

	static bool Valid(Index type_index);

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
			if constexpr (std::is_rvalue_reference_v<T>)
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

	private:
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

		friend class Handle;
		friend class Spandle;
	};
}

namespace Memory
{
	using Index = i64;
	static constexpr Index kInvalidIndex = -1;

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
	static constexpr u64 kMaximumParameters = 256;

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

	private:
		View view = View();
		Memory::Index index = Memory::kInvalidIndex;

		void invalidate();
		void destroy();

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
	// Inheritance
	// -----------------------------------------------------------------------------------------------------------------

	extern bool AddInheritance(Information& derived_info, const std::vector<Index>& directly_inherited);

	template<typename T, typename... DirectlyInherited>
	static bool AddInheritance()
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

	template<typename T, auto MethodPtr, typename Return, bool IsConst, typename Tuple, std::size_t... Index> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Method FromMethodImpl(std::index_sequence<Index...>)
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
							std::invoke(MethodPtr, view.as<const T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
						else if (view.is<const T&>())
							std::invoke(MethodPtr, view.as<const T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
						else if (view.is<T>())
							std::invoke(MethodPtr, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
						else
							std::invoke(MethodPtr, view.as<T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
					}
					else
					{
						if (view.is<T>())
							std::invoke(MethodPtr, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
						else
							std::invoke(MethodPtr, view.as<T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
					}
				}
				else
				{
					if constexpr (IsConst)
					{
						if (view.is<const T>())
							result = Handle(std::invoke(MethodPtr, view.as<const T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
						else if (view.is<const T&>())
							result = Handle(std::invoke(MethodPtr, view.as<const T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
						else if (view.is<T>())
							result = Handle(std::invoke(MethodPtr, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
						else
							result = Handle(std::invoke(MethodPtr, view.as<T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
					}
					else
					{
						if (view.is<T>())
							result = Handle(std::invoke(MethodPtr, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
						else
							result = Handle(std::invoke(MethodPtr, view.as<T&>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
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
	static Method FromMethod()
	{
		static_assert(std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...)> || std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...) const>, "MethodPtr is not a valid method pointer!");
		return FromMethodImpl<T, MethodPtr, Return, std::is_same_v<decltype(MethodPtr), Return (T::*)(Args...) const>, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Functions
	// -----------------------------------------------------------------------------------------------------------------

	using Function = Handle (*)(const Spandle&);

	template<auto FunctionPtr, typename Return, typename Tuple, std::size_t... Index>
	static Function FromFunctionImpl(std::index_sequence<Index...>)
	{
		return [](const Spandle& parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Index) > 0)
			{
				Program::Assert(parameters.size() == sizeof...(Index), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					(FunctionPtr)(parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
				else
					result = Handle((FunctionPtr)(parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
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
	static Function FromFunction()
	{
		static_assert(std::is_same_v<decltype(FunctionPtr), Return (*)(Args...)>, "FunctionPtr is not a valid function pointer!");
		return FromFunctionImpl<FunctionPtr, Return, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Members
	// -----------------------------------------------------------------------------------------------------------------

	using Member = Handle (*)(const View);

	template<auto MemberPtr, typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Member FromMemberImpl()
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
	static Member FromMember()
	{
		static_assert(std::is_same_v<decltype(MemberPtr), Return (T::*)>, "MemberPtr is not a valid member pointer!");
		return FromMemberImpl<MemberPtr, T>();
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------------------------------------------------------

	using Constructor = void (*)(const View, const Spandle&);

	template<typename T, typename Tuple, std::size_t... Index> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Constructor FromCtorImpl(std::index_sequence<Index...>)
	{
		return [](const View view, const Spandle& parameters) -> void
		{
			if constexpr (sizeof...(Index) > 0)
			{
				Program::Assert(parameters.size() == sizeof...(Index), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");
				new (view.raw<T&>()) T(parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
			}
			else
			{
				Program::Assert(parameters.empty(), MK_ASSERT_STATS_CSTR, "Constructor takes no parameters!");
				new (view.raw<T&>()) T;
			}
		};
	}

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Constructor FromCtor()
	{
		return FromCtorImpl<T, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	extern bool AddConstructor(const Information& info, const Constructor constructor, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static bool AddConstructor()
	{
		return AddConstructor(Info<T>(), FromCtor<T, Args...>(), FromParameterList<Args...>());
	}

	extern Constructor GetConstructor(const Index type, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Constructor GetConstructor()
	{
		return GetConstructor(Info<T>().index, FromParameterList<Args...>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Destructors
	// -----------------------------------------------------------------------------------------------------------------

	using Destructor = void (*)(const View);

	template<typename T>
	static Destructor FromDtor()
	{
		return [](const View view) -> void
		{
			view.as<T&>().~T();
		};
	}

	extern bool AddDestructor(const Information& info, const Destructor destructor); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static bool AddDestructor()
	{
		return AddDestructor(Info<T>(), FromDtor<T>());
	}

	extern Destructor GetDestructor(const Index type); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T>
	static Destructor GetDestructor()
	{
		return GetDestructor(Info<T>().index);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Assigners
	// -----------------------------------------------------------------------------------------------------------------

	using Assigner = View (*)(const View, const Spandle&);

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static Assigner FromAssignOp()
	{
		return [](const View view, const Spandle& parameters) -> View
		{
			Program::Assert(parameters.size() == size_t(1), MK_ASSERT_STATS_CSTR, "Mismatched parameter number!");
			view.as<T&>() = parameters[0].as<U>();
			return view;
		};
	}

	extern bool AddAssigner(const Information& info, const Assigner assigner, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename U> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static bool AddAssigner()
	{
		return AddAssigner(Info<T>(), FromAssignOp<T, U>(), FromParameterList<U>());
	}

	extern Assigner GetAssigner(const Index type, const FunctionSignature signature); // NOLINT(*-avoid-const-params-in-decls)

	template<typename T, typename... Args>
	static Assigner GetAssigner()
	{
		return GetAssigner(Info<T>().index, FromParameterList<Args...>());
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Logic
	// -----------------------------------------------------------------------------------------------------------------

	using UnaryOperator = Handle (*)(const View);
	using BinaryOperator = Handle (*)(const View, const View);

	// Math

	template<typename T>
	static UnaryOperator FromPositive()
	{
		return [](const View view) -> Handle
		{
			return Handle(+view.as<T>());
		};
	}

	template<typename T>
	static UnaryOperator FromNegative()
	{
		return [](const View view) -> Handle
		{
			return Handle(-view.as<T>());
		};
	}

	template<typename T>
	static UnaryOperator FromPrefixIncrement()
	{
		return [](const View view) -> Handle
		{
			++view.as<T&>();
			return Handle(view);
		};
	}

	template<typename T>
	static UnaryOperator FromPostfixIncrement()
	{
		return [](const View view) -> Handle
		{
			return Handle(view.as<T&>()++);
		};
	}

	template<typename T>
	static UnaryOperator FromPrefixDecrement()
	{
		return [](const View view) -> Handle
		{
			--view.as<T&>();
			return Handle(view);
		};
	}

	template<typename T>
	static UnaryOperator FromPostfixDecrement()
	{
		return [](const View view) -> Handle
		{
			return Handle(view.as<T&>()--);
		};
	}

	template<typename T>
	static BinaryOperator FromAddEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() += b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromAdd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() + b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromSubEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() -= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromSub()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() - b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromMulEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() *= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromMul()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() * b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromDivEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() /= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromDiv()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() / b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromModEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() %= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromMod()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() % b.as<T>());
		};
	}

	// Bit Manipulation

	template<typename T>
	static UnaryOperator FromBitwiseNot()
	{
		return [](const View view) -> Handle
		{
			return Handle(~view.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseAndEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() &= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseAnd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() & b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseOrEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() |= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseOr()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() | b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseXorEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() ^= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseXor()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() ^ b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseLeftShiftEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() <<= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseLeftShift()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() << b.as<T>());
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseRightShiftEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			a.as<T&>() >>= b.as<T>();
			return Handle(a);
		};
	}

	template<typename T>
	static BinaryOperator FromBitwiseRightShift()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<T>() >> b.as<T>());
		};
	}

	// Logic

	template<typename T>
	static UnaryOperator FromLogicalNot()
	{
		return [](const View a) -> Handle
		{
			return Handle(!a.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromLogicalAnd()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() && b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromLogicalOr()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() || b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() == b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromNotEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() != b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromLessThan()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() < b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromLessThanOrEquals()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() <= b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromGreaterThan()
	{
		return [](const View a, const View b) -> Handle
		{
			return Handle(a.as<const T&>() > b.as<const T&>());
		};
	}

	template<typename T>
	static BinaryOperator FromGreaterThanOrEquals()
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

		kUnaryOperation_Count
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

		kBinaryOperation_Equals,
		kBinaryOperation_NotEquals,
		kBinaryOperation_LessThan,
		kBinaryOperation_LessThanOrEquals,
		kBinaryOperation_GreaterThan,
		kBinaryOperation_GreaterThanOrEquals,

		kBinaryOperation_Count
	};

	// -----------------------------------------------------------------------------------------------------------------
	// Registration Helpers
	// -----------------------------------------------------------------------------------------------------------------

	template<typename T>
	static bool AddPOD()
	{
		return AddConstructor<T>()
			&& AddConstructor<T, const T&>()
			&& AddConstructor<T, T&&>()
			&&  AddDestructor<T>()
			&&    AddAssigner<T, const T&>()
			&&    AddAssigner<T, T&&>();
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

#define META(type, ...) \
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

#define META_AS(type, alt_name, ...) using alt_name = type; META(alt_name, __VA_ARGS__)

#endif
