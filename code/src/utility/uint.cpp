#include "uint.hpp"
#include <debug/assert.hpp>

constexpr int operator<=>(const CheckedUint& a, const CheckedUint& b) {
    return a <=> b;
}

CheckedUint CheckedUint::operator+=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value <= std::numeric_limits<unsigned int>::max() - other.value);
    }
    return CheckedUint(value + other.value);
}

CheckedUint CheckedUint::operator-=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value >= other.value);
    }
    return CheckedUint(value - other.value);
}

CheckedUint CheckedUint::operator*=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value <= std::numeric_limits<unsigned int>::max() / other.value);
    }
    return CheckedUint(value * other.value);
}

CheckedUint CheckedUint::operator/=(const CheckedUint& other)
{
    return CheckedUint(value / other.value);
}

CheckedUint CheckedUint::operator++(int)
{
    if (CHECK_OVERFLOW) {
        Assert(value != std::numeric_limits<unsigned int>::max());
    }
    return value++;
}

CheckedUint CheckedUint::operator++()
{
    if (CHECK_OVERFLOW) {
        Assert(value != std::numeric_limits<unsigned int>::max());
    }
    return --value;
}

CheckedUint CheckedUint::operator--(int)
{
    if (CHECK_OVERFLOW) {
        Assert(value != std::numeric_limits<unsigned int>::min());
    }
    return value--;
}

CheckedUint CheckedUint::operator--()
{
    if (CHECK_OVERFLOW) {
        Assert(value != std::numeric_limits<unsigned int>::min());
    }
    return --value;
}

CheckedUint::operator unsigned int() const
{
    return value;
}

std::ostream& operator<<(std::ostream& out, const CheckedUint& a)
{
    return out << a.value;
}

CheckedUint operator+(CheckedUint a, const CheckedUint& b)
{
    return a += b;
}

CheckedUint operator-(CheckedUint a, const CheckedUint& b)
{
    return a -= b;
}

CheckedUint operator*(CheckedUint a, const CheckedUint& b)
{
    return a *= b;
}

CheckedUint operator/(CheckedUint a, const CheckedUint& b)
{
    return a /= b;
}

CheckedUint::CheckedUint(unsigned int initialValue): value(initialValue)
{
}

CheckedUint::CheckedUint(int initialValue): value(unsigned int(initialValue))
{
    if (CHECK_OVERFLOW) {
        Assert(initialValue >= 0);
    }
    
}

CheckedUint::CheckedUint()
{
}
