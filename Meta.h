#ifndef META_H
#define META_H

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

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

namespace Meta
{
	template<typename T>
	extern std::string_view nameof();

	using Index = long long;
	static constexpr Index kInvalidType = -1;

	struct Information
	{
		Index index = kInvalidType;
		std::string_view name;
		size_t size;

		std::vector<bool> bases;
		size_t num_bases = 0;
	};

	extern const Information& Register(const std::string_view name, const size_t size);

	template<typename T>
	static const Information& Info()
	{
		using Type = std::remove_cvref_t<T>;
		static const Information& info = Register(nameof<Type>(), sizeof(Type));
		return info;
	}

	static Index Find(const std::string_view name);

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

	extern bool AddInheritance(Information& derived_info, const std::vector<Index>& directly_inherited);

	template<typename T, typename... DirectlyInherited>
	static bool AddInheritance()
	{
		const std::vector<Index> directly_inherited{ ((Info<DirectlyInherited>().index), ...) };
		return AddInheritance(const_cast<Information&>(Info<T>()), directly_inherited);
	}

	void DumpInfo();

	class View
	{
	public:
		View() = default;

		View(void* ptr, const Information& info)
			: data(ptr)
			, type(info.index)
		{}

		View(const View&) = default;
		View(View&&) = default;

		template<typename T> requires (!std::is_same_v<T, View>)
		View(T* ptr)
			: data(ptr)
			, type(Info<T>().index)
		{}

		template<typename T> requires (!std::is_same_v<T, View>)
		View(T& ref)
			: data(&ref)
			, type(Info<T>().index)
		{}

		~View() = default;

		View& operator=(const View&) = default;
		View& operator=(View&&) = default;

		explicit operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(const Information& info) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Info<T>());
		}

		template<typename T>
		T* raw() const
		{
			assert(is<T>());
			return static_cast<T*>(data);
		}


		template<typename T>
		T& as() const
		{
			return *raw<std::remove_reference_t<T>>();
		}

	private:
		void* data = nullptr;
		Index type = kInvalidType;

		friend class Handle;
	};
}

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
		Handle(Handle& other);
		Handle(const Handle& other);
		Handle(Handle&& other) noexcept;

		explicit Handle(const Information& info);
		explicit Handle(View v);

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		Handle(const T& value)
			: Handle(Meta::Info<T>())
		{
			view.as<std::remove_cvref_t<T>>() = value;
		}

		template<typename T> requires (!std::is_same_v<std::remove_cvref_t<T>, Handle>)
		Handle(T&& value)
			: Handle(Meta::Info<T>())
		{
			view.as<std::remove_cvref_t<T>>() = std::forward<T>(value);
		}

		~Handle();

		Handle& operator=(const Handle& other);
		Handle& operator=(Handle&& other) noexcept;

		explicit operator bool() const;
		[[nodiscard]] bool valid() const;

		[[nodiscard]] bool is(const Information& info) const;

		template<typename T>
		[[nodiscard]] bool is() const
		{
			return is(Meta::Info<T>());
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
			return expand_impl<std::tuple<Args...>>(std::index_sequence_for<Args...>{});
		}

	private:
		Handle pimpl;

		template<typename Tuple, std::size_t... Index>
		decltype(auto) expand_impl(std::index_sequence<Index...>) const
		{
			return Tuple((*this)[Index].template as<std::tuple_element_t<Index, Tuple>>()...);
		}
	};
	
	using Method = std::function<Handle (View, Spandle)>;

	template<typename T, typename Return, typename... Args>
	static Method FromMethod(Return (T::*method)(Args...))
	{
		return [method](const View view, Spandle parameters) -> Handle
		{
			Handle result;
				
			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					std::apply(view.as<T>().*method, parameters.expand<Args...>());
				else
					result = Handle(std::apply(view.as<T>().*method, parameters.expand<Args...>()));
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
	static Method FromMethod(Return (T::*method)(Args...) const)
	{
		return [method](const View view, Spandle parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					std::apply(view.as<T>().*method, parameters.expand<Args...>());
				else
					result = Handle(std::apply(view.as<T>().*method, parameters.expand<Args...>()));
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
	static Function FromMethod(Return (*function)(Args...))
	{
		return [function](Spandle parameters) -> Handle
		{
			Handle result;

			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				if constexpr (std::is_void_v<Return>)
					std::apply(function, parameters.expand<Args...>());
				else
					result = Handle(std::apply(function, parameters.expand<Args...>()));
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

	using Member = std::function<Handle (View)>;

	template<typename T, typename Return>
	static Member FromMember(Return T::*member)
	{
		return [member](const View view) -> Handle
		{
			return Handle(view.as<T>().*member);
		};
	}

	using Constructor = void (*)(View, Spandle);

	template<typename T, typename... Args>
	static Constructor FromCtor()
	{
		return [](const View view, Spandle parameters) -> void
		{
			if constexpr (sizeof...(Args) > 0)
			{
				assert(parameters.size() == sizeof...(Args) && "Mismatched parameter number!");

				std::apply([view](Args... args) -> void
				{
					new (view.raw<T>()) T(args...);
				}
				, parameters.expand<Args...>());
			}
			else
			{
				assert(parameters.empty() && "Constructor takes no parameters!");
				new (view.raw<T>()) T;
			}
		};
	}

	using Destructor = void (*)(View);

	template<typename T>
	static Destructor FromDtor()
	{
		return [](const View view) -> void
		{
			view.as<T>().~T();
		};
	}

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
