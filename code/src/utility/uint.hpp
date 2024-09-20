#pragma once
#include <ostream>

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
	CheckedUint(long initialValue);

	// does not initialize value
	CheckedUint();

	CheckedUint operator+=(const CheckedUint& other);
	CheckedUint operator-=(const CheckedUint& other);
	CheckedUint operator*=(const CheckedUint& other);
	CheckedUint operator/=(const CheckedUint& other);

	CheckedUint operator++(int); // post
	CheckedUint operator++(); // pre
	CheckedUint operator--(int); // post
	CheckedUint operator--(); // pre

	//constexpr CheckedUint operator=(CheckedUint other);

	unsigned int value;

	operator unsigned int() const;
};

std::ostream& operator<<(std::ostream& out, const CheckedUint& a);

CheckedUint operator+(CheckedUint a, const CheckedUint& b);
CheckedUint operator-(CheckedUint a, const CheckedUint& b);
CheckedUint operator*(CheckedUint a, const CheckedUint& b);
CheckedUint operator/(CheckedUint a, const CheckedUint& b);

constexpr int operator<=>(const CheckedUint& a, const CheckedUint& b);