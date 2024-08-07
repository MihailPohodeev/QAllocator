#include <iostream>

#include "qallocator.h"

int main(int argc, char** argv)
{
	Q::QAllocator qalloc;
	qalloc.DEBUG_print_tree_table();
	qalloc.allocate(64, 8);
	qalloc.allocate(4096, 16);
	qalloc.DEBUG_print_tree_table();
	return 0;
}
