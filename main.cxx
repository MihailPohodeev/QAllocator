#include <iostream>

#include "qallocator.hxx"

int main(int argc, char** argv)
{
	Q::QAllocator qalloc;
	std::cout << "1 TABLE : \n";
	qalloc.DEBUG_print_tree_table();
	qalloc.allocate(64, 8);
	std::cout << "2 TABLE : \n";
	qalloc.DEBUG_print_tree_table();
	qalloc.allocate(4096, 16);
	std::cout << "3 TABLE : \n";
	qalloc.DEBUG_print_tree_table();
	qalloc.allocate(2048, 32);
	std::cout << "4 TABLE : \n";
	qalloc.DEBUG_print_tree_table();
	return 0;
}
