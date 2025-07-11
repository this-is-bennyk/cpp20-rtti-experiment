#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

template<typename T>
extern const std::string& nameof();

namespace Meta
{
	using Index = long long;
	static constexpr Index kInvalidType = -1;

	struct Information
	{
		Index index = kInvalidType;
		const std::string& name;
		size_t size;

		std::vector<bool> bases;
		size_t num_bases = 0;
	};

	extern const Information& Register(const std::string& name, size_t size);

	template<typename T>
	static const Information& Info()
	{
		using Type = std::remove_cvref_t<T>;
		static const Information& info = Register(nameof<Type>(), sizeof(Type));
		return info;
	}

	static Index Find(const std::string& name);

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

	extern bool AddInheritance(Index derived, const std::vector<Index>& directly_inherited);

	template<typename T, typename... DirectlyInherited>
	static bool AddInheritance()
	{
		const std::vector<Index> directly_inherited{ ((Info<DirectlyInherited>().index), ...) };
		return AddInheritance(Info<T>().index, directly_inherited);
	}

	void Dump();

	class View
	{
	public:
		View() = default;

		View(void* ptr, Index type_index)
			: data(ptr)
			, type(type_index)
		{}

		template<typename T>
		View(T* ptr)
			: data(ptr)
			, type(Info<T>().index)
		{}

		template<typename T>
		View(T& ref)
			: data(&ref)
			, type(Info<T>().index)
		{}

		View(const View&) = default;
		View(View&&) = default;
		~View() = default;

		View& operator=(const View&) = default;
		View& operator=(View&&) = default;

		operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(Meta::Index type_index) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Info<T>().index);
		}

		template<typename T>
		T& as() const
		{
			assert(is<T>());
			return *static_cast<T*>(data);
		}

	private:
		void* data = nullptr;
		Index type = kInvalidType;

		friend class Handle;
	};
};

namespace Pools
{
	using Index = long long;
	static constexpr Index kInvalidIndex = -1;
}

namespace Meta
{
	class Handle
	{
	public:
		Handle() = default;
		Handle(const Handle& other);
		Handle(Handle&& other) noexcept;

		explicit Handle(Meta::Index type);
		explicit Handle(View v);

		template<typename T>
		Handle(const T& value)
			: Handle(Meta::Info<T>().index)
		{
			view.as<std::remove_cvref_t<T>>() = value;
		}

		template<typename T>
		Handle(T&& value)
			: Handle(Meta::Info<T>().index)
		{
			view.as<std::remove_cvref_t<T>>() = std::forward<T>(value);
		}

		~Handle();

		Handle& operator=(const Handle& other);
		Handle& operator=(Handle&& other) noexcept;

		operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(Meta::Index type) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Meta::Info<T>().index);
		}

		template<typename T>
		[[nodiscard]] T& as() const
		{
			assert(is<T>());
			return view.as<T>();
		}

		[[nodiscard]] View peek() const;

	private:
		Meta::View view = Meta::View();
		Pools::Index index = Pools::kInvalidIndex;

		void invalidate();
		void destroy();
	};

	class Spandle
	{
	public:
		Spandle(size_t num_handles = 0);
		Spandle(const Spandle&) = default;
		Spandle(Spandle&&) = default;
		~Spandle() = default;

		Spandle& operator=(const Spandle&) = default;
		Spandle& operator=(Spandle&&) = default;

		Handle& operator[](size_t index);
		Handle operator[](size_t index) const;

		[[nodiscard]] size_t size() const;
		[[nodiscard]] bool empty() const;

		template<typename... Args>
		std::tuple<Args...> expand() const
		{
			std::tuple<Args...> result;
			
			expand_impl(result, std::index_sequence_for<Args...>{});

			return result;
		}

	private:
		Handle pimpl;

		template<typename Tuple, std::size_t... Index>
		void expand_impl(Tuple& tuple, std::index_sequence<Index...>) const
		{
			([&]
			{
				std::get<Index>(tuple) = (*this)[Index].template as<Handle>().template as<std::tuple_element_t<Index, Tuple>>();

			}(), ...);
		}
	};
	
	using Method = std::function<Handle (View, Spandle)>;

	template<typename T, typename Return, typename... Args>
	static Method From(Return (T::* method)(Args...))
	{
		return [method](const View view, Spandle parameters) -> Handle
		{
			Handle result;
				
			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				std::tuple<Args...> arguments = parameters.expand<Args...>();

				if constexpr (std::is_void_v<Return>)
					std::apply(view.as<T>().*method, arguments);
				else
					result = Handle(std::apply(view.as<T>().*method, arguments));
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
	static Method From(Return (T::*method)(Args...) const)
	{
		return [method](const View view, Spandle parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				std::tuple<Args...> arguments = parameters.expand<Args...>();

				if constexpr (std::is_void_v<Return>)
					std::apply(view.as<T>().*method, arguments);
				else
					result = Handle(std::apply(view.as<T>().*method, arguments));
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

	using Function = std::function<Handle (Spandle)>;

	template<typename T, typename Return, typename... Args>
	static Function From(Return (*function)(Args...))
	{
		return [function](Spandle parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				std::tuple<Args...> arguments = parameters.expand<Args...>();

				if constexpr (std::is_void_v<Return>)
					std::apply(function, arguments);
				else
					result = Handle(std::apply(function, arguments));
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

	// using Member = std::function<Handle (View)>;
}

#define NAMEOF_DEF(type) \
template<> \
const std::string& nameof<type>() \
{ \
	static const std::string name = #type; \
	return name; \
}

#define META_COMPLEX(type, simplified_name, ...) \
NAMEOF_DEF(type) \
namespace \
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
