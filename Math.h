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

#ifndef MATH_H
#define MATH_H

#include <algorithm>
#include <cstdint>
#include <type_traits>

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

namespace Math
{
	template<typename T>
	static constexpr bool kIsNumber = !std::is_same_v<T, bool> && std::is_arithmetic_v<T>;

	template<typename T>
	static constexpr bool kIsIntegral = !std::is_same_v<T, bool> && std::is_integral_v<T>;

	template<typename T>
	static constexpr bool kIsFloat = std::is_floating_point_v<T>;

	template<typename T, size_t Dimension> requires (kIsNumber<T> && Dimension > size_t(0))
	struct Vector
	{
		T vec[Dimension] = { T() };

		constexpr Vector() = default;
		constexpr Vector(const Vector&) = default;
		constexpr Vector(Vector&&) = default;

		template<typename U, size_t OtherDimension> requires (kIsNumber<U> && OtherDimension > size_t(0))
		constexpr Vector(const Vector<U, OtherDimension>& other)
		{
			for (size_t i = 0; i < (std::min)(Dimension, OtherDimension); i++)
				vec[i] = T(other.vec[i]);

			if constexpr (Dimension > OtherDimension)
			{
				for (size_t i = OtherDimension; i < Dimension; ++i)
					vec[i] = T();
			}
		}

		template<typename U, size_t OtherDimension> requires (kIsNumber<U> && OtherDimension > size_t(0))
		constexpr Vector(const U (&other)[OtherDimension])
		{
			for (size_t i = 0; i < (std::min)(Dimension, OtherDimension); i++)
				vec[i] = T(other[i]);

			if constexpr (Dimension > OtherDimension)
			{
				for (size_t i = OtherDimension; i < Dimension; ++i)
					vec[i] = T();
			}
		}

		template<typename U> requires (kIsNumber<U>)
		constexpr Vector(U value)
		{
			for (size_t i = 0; i < Dimension; ++i)
				vec[i] = T(value);
		}

		constexpr ~Vector() = default;

		constexpr Vector& operator=(const Vector&) = default;
		constexpr Vector& operator=(Vector&&) = default;

		constexpr Vector operator+() const
		{
			Vector v;

			for (size_t i = 0; i < Dimension; ++i)
				v.vec[i] = +vec[i];

			return v;
		}

		constexpr Vector operator-() const
		{
			Vector v;

			for (size_t i = 0; i < Dimension; ++i)
				v.vec[i] = -vec[i];

			return v;
		}

		constexpr Vector& operator+=(const Vector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				vec[i] += other.vec[i];
			return *this;
		}

		constexpr Vector& operator-=(const Vector& other) { return *this += -other; }

		constexpr Vector& operator*=(const Vector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				vec[i] *= other.vec[i];
			return *this;
		}

		constexpr Vector& operator/=(const Vector& other) { return *this *= (T(1) / other); }

		static constexpr Vector Zero = Vector();
		static constexpr Vector One =  Vector(T(1));
	};
}

template<typename T, size_t Dimension>
constexpr Math::Vector<T, Dimension> operator+(const Math::Vector<T, Dimension>& a, const Math::Vector<T, Dimension>& b)
{
	Math::Vector<T, Dimension> v = a;
	v += b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::Vector<T, Dimension> operator-(const Math::Vector<T, Dimension>& a, const Math::Vector<T, Dimension>& b)
{
	Math::Vector<T, Dimension> v = a;
	v -= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::Vector<T, Dimension> operator*(const Math::Vector<T, Dimension>& a, const Math::Vector<T, Dimension>& b)
{
	Math::Vector<T, Dimension> v = a;
	v *= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::Vector<T, Dimension> operator/(const Math::Vector<T, Dimension>& a, const Math::Vector<T, Dimension>& b)
{
	Math::Vector<T, Dimension> v = a;
	v /= b;
	return v;
}

namespace Math
{
	template<typename T, size_t Dimension> requires (kIsIntegral<T> && Dimension > size_t(0))
	struct IntegralVector : public Vector<T, Dimension>
	{
		constexpr IntegralVector() = default;
		constexpr IntegralVector(const IntegralVector&) = default;
		constexpr IntegralVector(IntegralVector&&) = default;

		template<typename U, size_t OtherDimension> requires (kIsNumber<U> && OtherDimension > size_t(0))
		constexpr IntegralVector(const Vector<U, OtherDimension>& other)
			: Vector<T, Dimension>(other)
		{}

		template<typename U, size_t OtherDimension> requires (kIsNumber<U> && OtherDimension > size_t(0))
		constexpr IntegralVector(const U (&other)[OtherDimension])
			: Vector<T, Dimension>(other)
		{}

		template<typename U> requires (kIsNumber<U>)
		constexpr IntegralVector(U other)
			: Vector<T, Dimension>(other)
		{}

		constexpr ~IntegralVector() = default;

		constexpr IntegralVector& operator=(const IntegralVector&) = default;
		constexpr IntegralVector& operator=(IntegralVector&&) = default;

		constexpr operator Vector<T, Dimension>() const { return Vector<T, Dimension>(this->vec); }

		constexpr IntegralVector& operator%=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] %= other.vec[i];
			return *this;
		}

		constexpr IntegralVector operator~() const
		{
			IntegralVector v;

			for (size_t i = 0; i < Dimension; i++)
				v.vec[i] = ~this->vec[i];

			return v;
		}

		constexpr IntegralVector& operator&=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] &= other.vec[i];
			return *this;
		}

		constexpr IntegralVector& operator|=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] |= other.vec[i];
			return *this;
		}

		constexpr IntegralVector& operator^=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] ^= other.vec[i];
			return *this;
		}

		constexpr IntegralVector& operator<<=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] <<= other.vec[i];
			return *this;
		}

		constexpr IntegralVector& operator>>=(const IntegralVector& other)
		{
			for (size_t i = 0; i < Dimension; i++)
				this->vec[i] >>= other.vec[i];
			return *this;
		}
	};
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator%(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v %= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator&(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v &= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator|(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v |= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator^(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v ^= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator<<(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v <<= b;
	return v;
}

template<typename T, size_t Dimension>
constexpr Math::IntegralVector<T, Dimension> operator>>(const Math::IntegralVector<T, Dimension>& a, const Math::IntegralVector<T, Dimension>& b)
{
	Math::IntegralVector<T, Dimension> v = a;
	v >>= b;
	return v;
}

namespace Math
{
	template<typename T> requires (kIsFloat<T>) using Vector1 = Vector<T, 1>;
	template<typename T> requires (kIsFloat<T>) using Vector2 = Vector<T, 2>;
	template<typename T> requires (kIsFloat<T>) using Vector3 = Vector<T, 3>;
	template<typename T> requires (kIsFloat<T>) using Vector4 = Vector<T, 4>;

	template<typename T> requires (kIsIntegral<T>) using Vector1I = IntegralVector<T, 1>;
	template<typename T> requires (kIsIntegral<T>) using Vector2I = IntegralVector<T, 2>;
	template<typename T> requires (kIsIntegral<T>) using Vector3I = IntegralVector<T, 3>;
	template<typename T> requires (kIsIntegral<T>) using Vector4I = IntegralVector<T, 4>;

	template<typename T, size_t Dimension>
	constexpr T Dot(const Vector<T, Dimension>& a, const Vector<T, Dimension>& b)
	{
		T result = T();

		for (size_t i = 0; i < Dimension; ++i)
			result += a[i] * b[i];

		return result;
	}

	template<typename T, size_t Dimension>
	constexpr bool BehindRelative(const Vector<T, Dimension>& a, const Vector<T, Dimension>& b)
	{
		return Dot(a, b) < T();
	}

	template<typename T, size_t Dimension>
	constexpr bool BehindAbsolute(const Vector<T, Dimension>& a, const Vector<T, Dimension>& b)
	{
		return Dot(a, Vector<T, Dimension>::One) < Dot(a, Vector<T, Dimension>::One);
	}
}

#endif