#pragma once
#include <ostream>

template<typename T>
concept CheckedUintOperand = std::integral<T>;

// I'M TIRED OF USING UNSIGNED INTS AND THEN I GET A CRASH AND SOMEHOW AN UNSIGNED INT HAS A VALUE OF 4 BILLION AND I DON'T KNOW WHERE IT GOT THAT
// THIS UNSIGNED INT WRAPPER CHECKS FOR OVERFLOW WITH EVERY OPERATION
class CheckedUint {
public:
	

	constexpr static inline bool CHECK_OVERFLOW = true;

	// intializes value
	CheckedUint(unsigned int initialValue);
	CheckedUint(unsigned long initialValue);
	CheckedUint(unsigned long long initialValue);
	//CheckedUint(size_t initialValue);

	// intializes value statically
	//consteval CheckedUint(unsigned int initialValue);
	//consteval CheckedUint(unsigned long initialValue);
	//consteval CheckedUint(size_t initialValue);

	// checks for negative
	CheckedUint(int initialValue);
	//CheckedUint(long initialValue);

	// does not initialize value
	CheckedUint();

	CheckedUint operator+=(const CheckedUint& other);
	CheckedUint operator-=(const CheckedUint& other);
	CheckedUint operator*=(const CheckedUint& other);
	CheckedUint operator/=(const CheckedUint& other);

	template <CheckedUintOperand T>
	CheckedUint operator+=(const T& other);
	template <CheckedUintOperand T>
	CheckedUint operator-=(const T& other);
	template <CheckedUintOperand T>
	CheckedUint operator*=(const T& other);
	template <CheckedUintOperand T>
	CheckedUint operator/=(const T& other);

	CheckedUint operator++(int); // post
	CheckedUint operator++(); // pre
	CheckedUint operator--(int); // post
	CheckedUint operator--(); // pre

	//constexpr CheckedUint operator=(CheckedUint other);

	unsigned int value;

	operator unsigned int() const;
	//operator unsigned long() const;
	//operator unsigned long long() const;

	//operator int() const;
	//operator long() const;
	//operator long long() const;
};

std::ostream& operator<<(std::ostream& out, const CheckedUint& a);

CheckedUint operator+(CheckedUint a, const CheckedUint& b);
CheckedUint operator-(CheckedUint a, const CheckedUint& b);
CheckedUint operator*(CheckedUint a, const CheckedUint& b);
CheckedUint operator/(CheckedUint a, const CheckedUint& b);

template <CheckedUintOperand T>
CheckedUint operator+(CheckedUint a, const T& b);
template <CheckedUintOperand T>
CheckedUint operator-(CheckedUint a, const T& b);
template <CheckedUintOperand T>
CheckedUint operator*(CheckedUint a, const T& b);
template <CheckedUintOperand T>
CheckedUint operator/(CheckedUint a, const T& b);

template <CheckedUintOperand T>
CheckedUint operator+(const T& b, CheckedUint a);
template <CheckedUintOperand T>
CheckedUint operator-(const T& b, CheckedUint a);
template <CheckedUintOperand T>
CheckedUint operator*(const T& b, CheckedUint a);
template <CheckedUintOperand T>
CheckedUint operator/(const T& b, CheckedUint a);

constexpr std::strong_ordering operator<=>(const CheckedUint& a, const CheckedUint& b)
{
	return a.value <=> b.value;
}

template <CheckedUintOperand T>
constexpr std::strong_ordering operator<=>(const CheckedUint& a, const T& b);
template <CheckedUintOperand T>
constexpr std::strong_ordering operator<=>(const T& b, CheckedUint a);

template<CheckedUintOperand T>
inline CheckedUint operator+(CheckedUint a, const T& b)
{
	return a + CheckedUint(b);
}

template<CheckedUintOperand T>
inline CheckedUint operator-(CheckedUint a, const T& b)
{
	return a - CheckedUint(b);
}

template<CheckedUintOperand T>
inline CheckedUint operator*(CheckedUint a, const T& b)
{
	return a * CheckedUint(b);
}

template<CheckedUintOperand T>
inline CheckedUint operator/(CheckedUint a, const T& b)
{
	return a / CheckedUint(b);
}

template<CheckedUintOperand T>
inline constexpr std::strong_ordering operator<=>(const CheckedUint& a, const T& b)
{
	return a <=> CheckedUint(b);
}

template<CheckedUintOperand T>
inline CheckedUint operator+(const T& b, CheckedUint a)
{
	return CheckedUint(b) + a;
}

template<CheckedUintOperand T>
inline CheckedUint operator-(const T& b, CheckedUint a)
{
	return CheckedUint(b) - a;
}

template<CheckedUintOperand T>
inline CheckedUint operator*(const T& b, CheckedUint a)
{
	return CheckedUint(b) * a;
}

template<CheckedUintOperand T>
inline CheckedUint operator/(const T& b, CheckedUint a)
{
	return CheckedUint(b) / a;
}

template<CheckedUintOperand T>
inline constexpr std::strong_ordering operator<=>(const T& b, CheckedUint a)
{
	return CheckedUint(b) <=> a;
}

template<CheckedUintOperand T>
inline CheckedUint CheckedUint::operator+=(const T& other)
{
	CheckedUint a(other);
	return (*this) += a;
}

template<CheckedUintOperand T>
inline CheckedUint CheckedUint::operator-=(const T& other)
{
	CheckedUint a(other);
	return (*this) -= a;
}


template<CheckedUintOperand T>
inline CheckedUint CheckedUint::operator*=(const T& other)
{
	CheckedUint a(other);
	return (*this) *= a;
}


template<CheckedUintOperand T>
inline CheckedUint CheckedUint::operator/=(const T& other)
{
	CheckedUint a(other);
	return (*this) /= a;
}

template <>
struct std::hash<CheckedUint>
{
	inline std::size_t operator()(const CheckedUint& k) const
	{
		using std::hash;

		return k.value;
	}
};
