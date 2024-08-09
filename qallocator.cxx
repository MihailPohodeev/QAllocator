#include <stdlib.h>
#include <iostream>
#include "qallocator.hxx"

// ~~~ADDITIONAL FUNCTIONS AND METHODS~~~
// copy N bytes from one buffer to another buffer.
void copy_data(void* fromBuffer, void* toBuffer, U32 bytes)
{
	U8* from = (U8*)fromBuffer;
	U8* to	 = (U8*)toBuffer;

	for (U32 i = 0; i < bytes; i++)
	{
		to[i] = from[i];
	}
}

void bzero(void* buffer, U32 size)
{
	U8* buf = (U8*)buffer;
	for (U32 i = 0; i < size; i++)
		buf[i] = 0;
}

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

// DRY principles идут нахуй. Срочно переделать.
// {
struct Q::QAllocator::treenode* Q::QAllocator::find_min_treenode(struct Q::QAllocator::treenode* node)
{
	struct treenode* next = node->left;
	while(next != nil)
	{
		node = next;
		next = next->left;
	}
	return node;	
}

struct Q::QAllocator::treenode* Q::QAllocator::Iterator::find_min_treenode(struct Q::QAllocator::treenode* node)
{
	struct treenode* next = node->left;
	while(next != _nil)
	{
		node = next;
		next = next->left;
	}
	return node;
}
// }

// get aligned address.
void* align_of_address(void* address, U8 align)
{
	integer_value_of_pointer addr = (integer_value_of_pointer)address;
	U8 mask = addr & (align - 1);
	if (mask == 0)
		return address;
	return (void*)(addr + (align - mask));
}

// CLASS ITERATOR:
// constructor of class Iterator.
Q::QAllocator::Iterator::Iterator(struct treenode* nd, struct treenode* nl) : _node(nd), _nil(nl) {}

// get iterator's node.
struct Q::QAllocator::treenode* Q::QAllocator::Iterator::operator*()
{
	return _node;
}

// prefix ++ operator.
Q::QAllocator::Iterator& Q::QAllocator::Iterator::operator++()
{
	if (_node->right != _nil)
		_node = find_min_treenode(_node->right);
	else
	{
		struct treenode* parent = _node->parent;
		while (parent != _nil && _node == parent->right)
		{
			_node 	= parent;
			parent 	= parent->parent;
		}
		_node = parent;
	}
	return *this;
}

// postfix ++ operator.
Q::QAllocator::Iterator Q::QAllocator::Iterator::operator++(int)
{
	Iterator copy = *this;
	++(*this);
	return copy;
}

bool Q::QAllocator::Iterator::operator==(const Q::QAllocator::Iterator& other)
{
	return (this->_node == other._node);
}

bool Q::QAllocator::Iterator::operator!=(const Q::QAllocator::Iterator& other)
{
	return !(*this == other);
}

// get iterator to start of TREE.
Q::QAllocator::Iterator Q::QAllocator::begin()
{
	return Iterator(find_min_treenode(nil->parent), nil);
}

// get iterator to end of TREE.
Q::QAllocator::Iterator Q::QAllocator::end()
{
	return Iterator(nil, nil);
}

// constructor.
Q::QAllocator::QAllocator(U32 dataCapacity, U32 treeCapacity) : _dataCapacity(dataCapacity), _treeCapacity(treeCapacity)
{
	initBuffers(dataCapacity, treeCapacity);
}

// destructor.
Q::QAllocator::~QAllocator()
{
	free(_dataBuffer);
	free(_treeBuffer);
}

void Q::QAllocator::initBuffers(U32 dataCapacity, U32 treeCapacity)
{
	_dataBuffer = (U8*)malloc(dataCapacity);
	bzero(_dataBuffer, dataCapacity);
	_treeBuffer = (U8*)malloc(treeCapacity);
	
	// set current pointer to _treeBuffer start + align.
	_currentTreeTablePosition  = align_of_address(_treeBuffer, 8);
	// set current pointer to _dataBuffer.
	_currentDataBufferPosition = _dataBuffer;

	// allocate memory for nil_node.
	nil = allocate_treenode_in_buffer();

	// set nil-treenode.
	nil->parent 	= nil;
	nil->descriptor = 0;
	nil->size 	= 0;
	nil->align	= 0;
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
	/*
	    if pointer greater than end of TREE-TABLE -> return nullptr;
		 ________________________________
	 	  ... |   N   |  N+1  |  N+2  |  |N+3 |
	 	 _____|_______|_______|_______|__|    |
	 	                                 A    A
	                   end of buffer-------|    |
	    here pointer out of TREE-TABLE --------- 
	     
	*/
	if (((integer_value_of_pointer)_currentTreeTablePosition + sizeof(struct treenode)) > \
			((integer_value_of_pointer)_treeBuffer + _treeCapacity))
	{
		std::cout << "RESIZE TREE TABLE !\n";
		// ~~~Rebuilding RBTree from old buffer~~~
		// allocate new x2 buffer.
		U8* newBuff = (U8*)malloc(_treeCapacity * 2);
		if (newBuff == nullptr)
			return nullptr;
		// swap buffers.
		U8* temp = newBuff;
		newBuff = _treeBuffer;
		_treeBuffer = temp;
		
		/*
		 sturcture of variables below.
			 ______________________________       __________________
			|aligned  |node1 |node2 |node3 | ... |nodeN-1 |nodeN |  |
			|_________|______|______|______|     |________|______|__|
		        A         A                                          A  A
 		        |  	  |					     |  |
		    buffer start  |______ _oldBufferStart 		     |  |
		  		    					     |  |
		   		  	_oldCurrentTreeTablePosition_________|  |
									 	|
								buffer end _____|
		*/

		void* _oldCurrentTreeTablePosition = _currentTreeTablePosition;
		void* _oldBufferStart = (void*)((integer_value_of_pointer)nil + sizeof(struct treenode));
		_currentTreeTablePosition = align_of_address(_treeBuffer, 8);
		//return nil in buffer.
		nil = allocate_treenode_in_buffer();
		nil->parent     = nil;
		nil->descriptor = 0;
		nil->size       = 0;
		nil->align	= 0;
		nil->isRed      = false;
		nil->segment    = nullptr;
		nil->usefulData = nullptr;
		nil->left       = nil;
		nil->right      = nil;
		
		// get descriptors from old buffer -> build new tree -> in the corresponding treenode put values from old treenode.
		for ( integer_value_of_pointer i = (integer_value_of_pointer)_oldBufferStart; \
			i < (integer_value_of_pointer)_oldCurrentTreeTablePosition; i += sizeof(struct treenode) )
		{
			struct treenode* oldNode = (struct treenode*)i;
			struct treenode* newNode = add_node_in_tree(oldNode->descriptor);
			newNode->size 		 = oldNode->size;
			newNode->align		 = oldNode->align;
			newNode->segment 	 = oldNode->segment;
			newNode->usefulData	 = oldNode->usefulData;
		}
		
		// change capacity of TREE-TABLE.
		_treeCapacity *= 2;
		free(newBuff);
	}
	struct treenode* result = (struct treenode*)_currentTreeTablePosition;
	_currentTreeTablePosition = (void*)((integer_value_of_pointer)_currentTreeTablePosition + sizeof(struct treenode));
	return result;
}

// add treenode with descriptor from parameter to tree and balance tree.
struct Q::QAllocator::treenode* Q::QAllocator::add_node_in_tree( U32 descriptor )
{
	struct treenode* node = allocate_treenode_in_buffer();
	if (!node)
		return nullptr;
	node->descriptor = descriptor;
	node->left 	 = nil;
	node->right	 = nil;
	node->isRed	 = true;
	
	// if inserting node is root.
	if (nil->parent == nil)
	{
		nil->parent  = node;
		node->parent = nil;
	}
	else
	{
		// insert node in end of tree and balance it.
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
	if (!node->parent->isRed)
	{
		return;
	}
	struct treenode* u = uncle(node);
	if ((u != nullptr) && (u->isRed))
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
	// result node.
	struct treenode* node = add_node_in_tree(_currentDescriptor);
	// if memory allocation for treenode was fault.
	if (!node)
		return 0;

	node->align = align;

	/*
		structure variables below:
		 _______________________    ________________
		| T1  |T2|     T3     |  ...  | TN - 1 |    |  |
		|_____|__|____________|_    __|________|____|  |
		A				       A    A  A
		|___start of data buffer	       |    |  |
						       |    |  |
		current (_currentDataBufferPosition)___|    |  |
						            |  |
				end of data buffer__________|  |
							       |
		current + alignDifference + size_______________|
			  	  ||
		current    +	realSize

	*/

	void* current = _currentDataBufferPosition;
	void* aligned = align_of_address(current, align);
	U32 alignDifference = (integer_value_of_pointer)aligned - (integer_value_of_pointer)current;
	U32 realSize = size + alignDifference;

	if ((integer_value_of_pointer)current + realSize > (integer_value_of_pointer)_dataBuffer + _dataCapacity)
	{
		std::cout << "RESIZE DATA ! capacity : " << _dataCapacity << '\n';
		DEBUG_print_data_buffer();
		U64 max = _dataCapacity * 2;
		max = max > _dataCapacity + realSize ? max : (_dataCapacity + realSize) * 1.5f;
		resize_data_buffer(max);
	}

	node->segment 	 = current;
	node->usefulData = aligned;
	node->size	 = realSize;

	_currentDataBufferPosition = (void*)((integer_value_of_pointer)_currentDataBufferPosition + realSize);

	_currentDescriptor++;

	return node->descriptor;
}

// resize buffer.
// parameter1 - new size.
bool Q::QAllocator::resize_data_buffer(U32 newSize)
{
	if (newSize < _dataCapacity)
		return false;
	
	// memorize old buffer.
	U8* oldBuff = _dataBuffer;
	//void* oldCurrentDataBufferPosition = _currentDataBufferPosition;
	// create new buffer.
	_dataBuffer = (U8*)malloc(newSize);
	bzero(_dataBuffer, newSize);
	_currentDataBufferPosition = _dataBuffer;

	// copy all data from old buffer to new.
	/*
	 * We bypass the RBT and replace data-segments from old buffer to new
	 *	nil
	 *	 V
	 *	root
	 *	/ \
	 *     X   X
	 *    /\   /\
	 *   ...   ...
	 *
	 * 	Old Buffer : 
	 * 	 __________
	 * 	| T1 | ...
	 * 	|____|_____
	 *
	 * 	New Buffer :*
	 * 	 ___________
	 * 	| T1 | ...
	 * 	|____|______ 
	 *
	*/
	Iterator beg = begin();
	Iterator e = end();
	while(beg != e)
	{
		struct treenode* node = *beg;
		void* segment = _currentDataBufferPosition;
		void* aligned = align_of_address(segment, node->align);

		U32 alignDifference = (integer_value_of_pointer)node->usefulData - (integer_value_of_pointer)node->segment;
		U32 size = node->size - alignDifference;
		copy_data(node->usefulData, aligned, size);

		node->size 		= size + (integer_value_of_pointer)aligned - (integer_value_of_pointer)segment;
		node->segment		= segment;
		node->usefulData	= aligned;

		_currentDataBufferPosition = (void*)((integer_value_of_pointer)aligned + size);
		++beg;
	}
	_dataCapacity = newSize;
	free(oldBuff);
	return true;
}

// get pointer of descriptor
void* Q::QAllocator::get_pointer(U32 descriptor)
{
	struct treenode* node = nil->parent;
	
	if (node == nil)
		return nullptr;

	while(1)
	{
		if (node == nil)
			return nullptr;
		if (descriptor == node->descriptor)
			return node->usefulData;
		if (descriptor > node->descriptor)
			node = node->right;
		else
			node = node->left;
	}
	return nullptr;
}
