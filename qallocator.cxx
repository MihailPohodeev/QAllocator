#include <stdlib.h>
#include <iostream>
#include "qallocator.h"

Q::QAllocator::QAllocator(U32 dataCapacity, U32 treeCapacity) : _dataCapacity(dataCapacity), _treeCapacity(treeCapacity)
{
	initBuffers(dataCapacity, treeCapacity);
}

Q::QAllocator::~QAllocator()
{
	free(_dataBuffer);
	free(_treeBuffer);
}

void Q::QAllocator::initBuffers(U32 dataCapacity, U32 treeCapacity)
{
	_dataBuffer = (U8*)malloc(dataCapacity);
	_treeBuffer = (U8*)malloc(treeCapacity);
	
	// set current pointer to _treeBuffer start + align.
	_currentTreeTablePosition = _treeBuffer;

	// align pointer by 8 bytes.
	_currentTreeTablePosition = (void*)((integer_value_of_pointer)_currentTreeTablePosition & ~7);
	nil = allocate_tree_node();

	// set nil-treenode.
	nil->parent 	= nil;
	nil->descriptor = 0;
	nil->size 	= 0;
	nil->isRed	= false;
	nil->segment 	= nullptr;
	nil->usefulData	= nullptr;
	nil->left 	= nil;
	nil->right	= nil;

	_currentDescriptor = 1;
}

struct Q::QAllocator::treenode* Q::QAllocator::allocate_treenode_in_buffer()
{
	// if pointer greater than end of TREE-TABLE -> return nullptr;
	//	 ________________________________
	// 	  ... |   N   |  N+1  |  N+2  |  |N+3 |
	// 	 _____|_______|_______|_______|__|    |
	// 	                                 A    A
	//                   end of buffer-------|    |
	//     here pointer out of TREE-TABLE --------- 
	if (((integer_value_of_pointer)_currentTreeTablePosition + sizeof(struct treenode)) > \
			((integer_value_of_pointer)_treeBuffer + _treeCapacity))
		return nullptr;
	struct treenode* result = (struct treenode*)_currentTreeTablePosition;
	_currentTreeTablePosition = (void*)((integer_value_of_pointer)_currentTreeTablePosition + sizeof(struct treenode));
	return result;
}

// add treenode with descriptor from parameter to tree and balance tree.
struct Q::QAllcator::treenode* Q::QAllocator::add_node_in_tree( U32 descriptor )
{
	struct treenode* node = allocate_treenode_in_buffer();
	node->descriptor = descriptor;
	node->left 	 = nil;
	node->right	 = nil;
	node->isRed	 = true;

	if (nil->parent == nil)
	{
		nil->parent  = node;
		node->parent = nil;
	}
	else
	{
		struct treenode* curr = nil->parent;
		while(1)
		{
			if (descriptor > curr->descriptor)
			{
				if (curr->right == nil)
				{
					node->parent = curr;
					curr->right = node;
					break;
				}
				else
				{
					curr = curr->right;
				}
			}
			else
			{
				if (curr->left == nil)
				{
					node->parent = curr;
					curr->left = node;
					break;
				}
				else
				{
					curr = curr->left;
				}
			}
		}
		d
	}
}

// after adding treenode in tree -> balance tree, starts param.
// parameter1 - treenode - start for balancing.
void Q::QAllocator::balance_tree(struct treenode* node)
{

}

// allocate memory and return descriptor ( U32 ).
U32 Q::QAllocator::allocate( U32 size, U8 align)
{
	struct treenode* node = allocate_treenode();
	if (node == nullptr)
	{
		// TODO
		// resize TREE-TABLE
	}
	
	if (nil->parent == nullptr)
	{
		nil->parent = node;
		node->parent = nil;
	}
	
	node->size = size;
	node->right = node->left = nil;
	node->isRed = true;
	node->descriptor = _currentDescriptor;
	_currentDescriptor++;

	return node->descriptor;
}



