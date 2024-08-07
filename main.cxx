#include <iostream>

#include "qallocator.hxx"

void* align_of_address(void* address, U8 align);

int main(int argc, char** argv)
{
	//Q::QAllocator qalloc;
	U64 n = 117;
	std::cout << n << ' ';
	U64 m = (U64)align_of_address((void*)n, 8);
	std::cout << m << '\n';

	return 0;
}
