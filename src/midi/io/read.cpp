#include "read.h"

template<typename T>
void io::read_to(std::istream& in, T* buffer, unsigned n) {
	in.read(reinterpret_cast<char*>(buffer), sizeof(buffer) * n);
}

template<typename T, typename std::enable_if<std::is_fundamental<T>::value, T>::type * = nullptr>
T io::read(std::istream& in)
{
	T res;
	read_to(in, &res);
	return res;
}

template<typename T>
std::unique_ptr<T[]> io::read_array(std::istream& in, unsigned n) {
	std::unique_ptr<T[]> res = std::make_unique<T[]>(n);
	read_to(in, res.get(), n);
	return res;
}