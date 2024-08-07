#include <stdlib.h>
#include <iostream>
#include "qallocator.hxx"

// ~~~ADDITIONAL FUNCTIONS AND METHODS~~~
// get grandparent of treenode.
struct Q::QAllocator::treenode* Q::QAllocator::grandparent(struct Q::QAllocator::treenode* node)
{
	if (node->parent == nil)
		return nullptr;
	node = node->parent;
	if (node->parent == nil)
		return nullptr;
	return node->parent;
}

// get uncle of treenode.
struct Q::QAllocator::treenode* Q::QAllocator::uncle(struct Q::QAllocator::treenode* node)
{
	if (node->parent == nil)
		return nullptr;
	node = node->parent;
	if (node->parent == nil)
		return nullptr;
	if (node == node->parent->left)
		return node->parent->right;
	else if (node == node->parent->right)
		return node->parent->left;
	return nullptr;
}

// rotate left for treenode
void Q::QAllocator::rotate_left(struct Q::QAllocator::treenode* node)
{
	struct Q::QAllocator::treenode* pivot = node->right;

	pivot->parent = node->parent;
	if (node->parent != nil)
	{
		if(node->parent->left == node)
			node->parent->left  = pivot;
		else
			node->parent->right = pivot;
	}
	else
		nil->parent = pivot;

	node->right = pivot->left;
	if (pivot->left != nil)
		pivot->left->parent = node;

	node->parent = pivot;
	pivot->left = node;
}

// rotate right for treenode
void Q::QAllocator::rotate_right(struct Q::QAllocator::treenode* node)
{
	struct Q::QAllocator::treenode* pivot = node->left;

	pivot->parent = node->parent;
	if (node->parent != nil)
	{
		if(node->parent->left == node)
			node->parent->left  = pivot;
		else
			node->parent->right = pivot;
	}
	else 
		nil->parent = pivot;

	node->right = pivot->right;
	if (pivot->right != nil)
		pivot->right->parent = node;

	node->parent = pivot;
	pivot->right = node;
}

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
	nil = allocate_treenode_in_buffer();

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

// allocate memory in TREE-TABLE for treenode.
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
struct Q::QAllocator::treenode* Q::QAllocator::add_node_in_tree( U32 descriptor )
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
	}
	balance_tree_insertion(node);
	return node;
}

// after adding treenode in tree -> balance tree, starts param.
// parameter1 - treenode - start for balancing.
void Q::QAllocator::balance_tree_insertion(struct treenode* node)
{
	if (node->parent == nil)
	{
		node->isRed = false;
		return;
	}
	if (node->parent->isRed == false)
	{
		return;
	}
	struct treenode* u = uncle(node);
	if ((u == nullptr) && (u->isRed == true))
	{
		node->parent->isRed = false;
		u->isRed = false;
		struct treenode* g = grandparent(node);
		g->isRed = true;
		balance_tree_insertion(g);
		return;
	}
	struct treenode* g = grandparent(node);
	
	if ((node == node->parent->right) && (node->parent == g->left))
	{
		rotate_left(node->parent);
		node = node->left;
	}
	else if ((node == node->parent->left) && (node->parent == g->right))
	{
		rotate_right(node->parent);
		node = node->right;
	}

	node->parent->isRed = false;
	g->isRed = true;
	if ((node == node->parent->left) && (node->parent == g->left))
		rotate_right(g);
	else
		rotate_left(g);
}

// allocate memory and return descriptor ( U32 ).
U32 Q::QAllocator::allocate( U32 size, U8 align)
{
	struct treenode* node = add_node_in_tree(_currentDescriptor);
	node->size = size;

	if (node == nullptr)
	{
		// TODO
		// resize TREE-TABLE
	}
	
	_currentDescriptor++;

	return node->descriptor;
}

// resize buffer
// parameter1 - pointer of buffer's start.
// parameter2 - old size.
// parameter3 - new size.
bool Q::QAllocator::resize(U8* buf, U32 oldSize, U32 newSize)
{
	U8* newBuff = (U8*)malloc(newSize);
	if (newBuff == nullptr)
		return false;

	for( int i = 0; (i < oldSize || i < newSize); i++ )
		newBuff[i] = buf[i];

	free(buf);
	buf = newBuff;
	return true;
}






