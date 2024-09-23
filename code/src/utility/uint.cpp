#include "uint.hpp"
#include <debug/assert.hpp>

CheckedUint CheckedUint::operator+=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value <= std::numeric_limits<unsigned int>::max() - other.value);
    }
    return CheckedUint(value += other.value);
}

CheckedUint CheckedUint::operator-=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value >= other.value);
    }
    return CheckedUint(value -= other.value);
}

CheckedUint CheckedUint::operator*=(const CheckedUint& other)
{
    if (CHECK_OVERFLOW) {
        Assert(value <= std::numeric_limits<unsigned int>::max() / other.value);
    }
    return CheckedUint(value *= other.value);
}

CheckedUint CheckedUint::operator/=(const CheckedUint& other)
{
    return CheckedUint(value /= other.value);
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

//CheckedUint::operator unsigned long() const
//{
//    return value;
//}
//
//CheckedUint::operator unsigned long long() const
//{
//    return value;
//}
//
//CheckedUint::operator int() const
//{
//    if (CHECK_OVERFLOW) {
//        Assert(value <= std::numeric_limits<int>::max());
//    }
//    return value;
//}
//
//CheckedUint::operator long() const
//{
//    return value;
//}
//
//CheckedUint::operator long long() const
//{
//    return value;
//}

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



constexpr std::strong_ordering operator<=>(const CheckedUint& a, const unsigned int& b)
{
    return a <=> CheckedUint(b);
}

CheckedUint::CheckedUint(unsigned int initialValue): value(initialValue)
{
}

CheckedUint::CheckedUint(unsigned long initialValue): value(initialValue)
{
    Assert(initialValue <= std::numeric_limits<unsigned int>::max());
}

CheckedUint::CheckedUint(unsigned long long initialValue) : value(initialValue)
{
    Assert(initialValue <= std::numeric_limits<unsigned int>::max());
}

//CheckedUint::CheckedUint(size_t initialValue): value(initialValue)
//{
//    Assert(initialValue <= std::numeric_limits<unsigned int>::max());
//}

//consteval CheckedUint::CheckedUint(size_t initialValue): value(initialValue) {
//    static_assert(initialValue <= std::numeric_limits<unsigned int>::max());
//}

CheckedUint::CheckedUint(int initialValue): value(unsigned int(initialValue))
{
    if (CHECK_OVERFLOW) {
        Assert(initialValue >= 0);
    }
    
}

CheckedUint::CheckedUint()
{
}