#ifndef META_H
#define META_H

#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

namespace Meta
{
	template<typename T>
	extern std::string_view nameof();

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
	static constexpr u8 kQualifier_Temporary = 0b000;
	static constexpr u8 kQualifier_Constant  = 0b001;
	static constexpr u8 kQualifier_Volatile  = 0b010;
	static constexpr u8 kQualifier_Reference = 0b100;

	template<typename T>
	static constexpr auto QualifiersOf = Qualifier(std::is_rvalue_reference_v<T>
		? kQualifier_Temporary
		: (u8(std::is_const_v<std::remove_reference_t<T>>) << u8(0)) | (u8(std::is_volatile_v<std::remove_reference_t<T>>) << u8(1)) | (u8(std::is_lvalue_reference_v<T>) << u8(2)));

	struct Information
	{
		Index index = kInvalidType;
		std::string_view name;
		size_t size;

		std::vector<bool> bases;
		size_t num_bases = 0;
	};

	extern const Information& Register(std::string_view name, size_t size);

	template<typename T>
	static const Information& Info()
	{
		using Type = std::remove_cvref_t<T>;
		static const Information& info = Register(nameof<Type>(), sizeof(Type));
		return info;
	}

	static Index Find(std::string_view name);

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
			return (... && args);
		return true;
	}

	static bool Valid(Index type_index);

	void DumpInfo();

	class View
	{
	public:
		View() = default;

		View(void* ptr, const Information& info, const Qualifier qualifier_flags);

		View(const View&) = default;
		View(View&&) = default;

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(T* ptr)
			: View(ptr, Info<T>(), QualifiersOf<T>)
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		explicit View(T& ref)
			: View(&ref)
		{
			qualifiers = QualifiersOf<T>;
		}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, View>)
		View(T&& value)
			: View(&value)
		{
			if constexpr (kIsPrimitive<T>)
				*this = value;
			else
				qualifiers = QualifiersOf<T>;
		}

		~View() = default;

		View& operator=(const View&) = default;
		View& operator=(View&&) = default;

		template<typename T> requires (kIsPrimitive<T>)
		View& operator=(T&& value)
		{
			type = -Info<T>().index + kByValue_u8;
			qualifiers = QualifiersOf<T>;
			*static_cast<std::remove_cvref_t<T>*>(static_cast<void*>(&data[0])) = value;
			return *this;
		}

		explicit operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(const Information& info, const Qualifier qualifier_flags) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Info<T>(), QualifiersOf<T>);
		}

		template<typename T>
		std::remove_cvref_t<T>* raw() const
		{
			assert(valid());
			assert(is<T>());

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
			assert(is_in_place_primitive());
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
		explicit Handle(const T& value)
			: Handle(Meta::Info<T>(), Spandle(&value))
		{}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		Handle(T&& value)
			: Handle()
		{
			if constexpr (kIsPrimitive<T>)
				view = value;
			else
				*this = Handle(Info<T>(), Spandle(&value));
		}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		explicit Handle(T* value)
			: view(View(value))
		{}

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

		[[nodiscard]] bool is(const Information& info, const Qualifier qualifier_flags) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Meta::Info<T>(), QualifiersOf<T>);
		}

		template<typename T>
		[[nodiscard]] T as() const
		{
			assert(is<T>());
			return view.as<T>();
		}

		template<typename T> requires (kIsPrimitive<T>)
		T primitive() const { return view.primitive<T>(); }

		[[nodiscard]] View peek() const;

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
		explicit Spandle(size_t num_handles = 0);

		template<typename... Args>
		static Spandle Create(Args... args)
		{
			auto result = Spandle(sizeof...(Args));
			Memory::Index index = 0;

			([&]()
			{
				if constexpr (kIsPrimitive<Args>)
					result[index] = args;
				else
					result[index] = Handle(args);

				++index;

			}(), ...);

			return result;
		}

		Spandle(const Spandle&) = default;
		Spandle(Spandle&&) = default;
		~Spandle();

		Spandle& operator=(const Spandle&) = default;
		Spandle& operator=(Spandle&&) = default;

		Handle& operator[](Memory::Index index);
		Handle  operator[](Memory::Index index) const;

		[[nodiscard]] size_t size() const;
		[[nodiscard]] bool empty() const;

		[[nodiscard]] FunctionSignature get_function_signature(ParameterArray& memory) const;

	private:
		Memory::Range list;
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

	using Method = std::function<Handle (const View, const Spandle&)>;

	template<typename T, typename Return, typename MethodPtr, typename Tuple, std::size_t... Index>
	static Method FromMethodImpl(MethodPtr method, std::index_sequence<Index...>)
	{
		return [method](const View view, const Spandle& parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Index) > 0)
			{
				assert(parameters.size() == sizeof...(Index) && "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					std::invoke(method, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
				else
					result = Handle(std::invoke(method, view.as<T>(), parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
			}
			else
			{
				assert(parameters.empty() && "Method takes no parameters!");

				if constexpr (std::is_void_v<Return>)
					(view.as<T>().*method)();
				else
					result = Handle((view.as<T>().*method)());
			}

			return result;
		};
	}

	template<typename T, typename Return, typename... Args>
	static Method FromMethod(Return (T::*method)(Args...))
	{
		return FromMethodImpl<T, Return, Return (T::*)(Args...), std::tuple<Args...>>(method, std::index_sequence_for<Args...>{});
	}

	template<typename T, typename Return, typename... Args>
	static Method FromMethod(Return (T::*method)(Args...) const)
	{
		return FromMethodImpl<T, Return, Return (T::*)(Args...) const, std::tuple<Args...>>(method, std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Functions
	// -----------------------------------------------------------------------------------------------------------------

	using Function = std::function<Handle (const Spandle&)>;

	template<typename Return, typename FunctionPtr, typename Tuple, std::size_t... Index>
	static Function FromFunctionImpl(FunctionPtr function, std::index_sequence<Index...>)
	{
		return [function](const Spandle& parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Index) > 0)
			{
				assert(parameters.size() == sizeof...(Index) && "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					std::invoke(function, parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
				else
					result = Handle(std::invoke(function, parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...));
			}
			else
			{
				assert(parameters.empty() && "Method takes no parameters!");

				if constexpr (std::is_void_v<Return>)
					(function)();
				else
					result = Handle((function)());
			}

			return result;
		};
	}

	template<typename Return, typename... Args>
	static Function FromFunction(Return (*function)(Args...))
	{
		return FromFunctionImpl<Return, Return (*)(Args...), std::tuple<Args...>>(function, std::index_sequence_for<Args...>{});
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Members
	// -----------------------------------------------------------------------------------------------------------------

	using Member = std::function<Handle (const View)>;

	template<typename T, typename Return>
	static Member FromMember(Return T::*member)
	{
		return [member](const View view) -> Handle
		{
			return Handle(view.as<T>().*member);
		};
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Constructors
	// -----------------------------------------------------------------------------------------------------------------

	using Constructor = void (*)(const View, const Spandle&);

	template<typename T, typename Tuple, std::size_t... Index>
	static Constructor FromCtorImpl(std::index_sequence<Index...>)
	{
		return [](const View view, const Spandle& parameters) -> void
		{
			if constexpr (sizeof...(Index) > 0)
			{
				assert(parameters.size() == sizeof...(Index) && "Mismatched parameter number!");
				new (view.raw<T&>()) T(parameters[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
			}
			else
			{
				assert(parameters.empty() && "Constructor takes no parameters!");
				new (view.raw<T&>()) T;
			}
		};
	}

	template<typename T, typename... Args>
	static Constructor FromCtor()
	{
		return FromCtorImpl<T, std::tuple<Args...>>(std::index_sequence_for<Args...>{});
	}

	extern bool AddConstructor(const Information& info, Constructor constructor, FunctionSignature signature);

	template<typename T, typename... Args> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static bool AddConstructor()
	{
		return AddConstructor(Info<T>(), FromCtor<T, Args...>(), FromParameterList<Args...>());
	}

	extern Constructor GetConstructor(Index type, FunctionSignature signature);

	template<typename T, typename... Args>
	static Constructor GetConstructor()
	{
		return GetDefaultConstructor(Info<T>().index, FromParameterList<Args...>());
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

	extern bool AddDestructor(const Information& info, Destructor destructor);

	template<typename T> requires (std::is_same_v<T, std::remove_pointer_t<std::remove_cvref_t<T>>>)
	static bool AddDestructor()
	{
		return AddDestructor(Info<T>(), FromDtor<T>());
	}

	extern Destructor GetDestructor(Index type);

	template<typename T>
	static Destructor GetDestructor()
	{
		return GetDestructor(Info<T>().index);
	}

	// -----------------------------------------------------------------------------------------------------------------
	// Assigners
	// -----------------------------------------------------------------------------------------------------------------

	// using Assigner = View (*)(View, Spandle);

	// template<typename T, typename... Args>
	// static Assigner FromAssignOp()
	// {
	// 	return [](View view, Spandle parameters) -> void
	// 	{
	// 		if constexpr (sizeof...(Args) > 0)
	//
	// 	}
	// }

	template<typename T>
	static bool AddPOD()
	{
		return AddConstructor<T>()
			&& AddConstructor<T, const T&>()
			&& AddConstructor<T, T&&>()
			&& AddDestructor<T>();
	}
}

#define NAMEOF_DEF(type) \
namespace Meta \
{ \
	template<> \
	std::string_view nameof<type>() \
	{ \
		static constexpr auto name = #type; \
		static constexpr auto size = sizeof(#type) - 1; \
		return std::string_view(name, size); \
	} \
}

#define META_COMPLEX(type, simplified_name, ...) \
NAMEOF_DEF(type) \
namespace Meta \
{ \
	static const bool k##simplified_name##Success = []() -> bool \
		{ \
			Meta::Index MetaIndex = Meta::Info<type>().index; \
			using Type = std::remove_cvref_t<type>; \
			bool result = Meta::RegistrationSuccessful(__VA_ARGS__); \
			assert(result && "Error in meta registration process!"); \
			return result; \
		}(); \
}

#define META(type, ...) META_COMPLEX(type, type, __VA_ARGS__)
#define META_AS(type, alt_name, ...) using alt_name = type; META(alt_name, __VA_ARGS__)

#endif
