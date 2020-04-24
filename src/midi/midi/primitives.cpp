#include "primitives.h"

midi::Duration midi::operator+(const Duration& a, const Duration& b)
{
	return Duration(value(a) + value(b));
}

midi::Time midi::operator+(const Time& a, const Duration& b)
{
	return Time(value(a) + value(b));
}

midi::Time midi::operator+(const Duration& a, const Time& b)
{
	return Time(value(a) + value(b));
}

midi::Duration midi::operator-(const Time& a, const Time& b)
{
	return Duration(value(a) - value(b));
}

midi::Duration midi::operator-(const Duration& a, const Duration& b)
{
	return Duration(value(a) - value(b));
}

midi::Duration& midi::operator+=(Duration& a, const Duration& b)
{
	a = a + b;
	return a;
}

midi::Duration& midi::operator-=(Duration& a, const Duration& b)
{
	a = a - b;
	return a;
}

midi::Time& midi::operator+=(Time& a, const Duration& b)
{
	a = a + b;
	return a;
}