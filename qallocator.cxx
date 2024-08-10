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

template <typename T>
void swap(T& a, T& b)
{
	T temp = a;
	a = b;
	b = temp;
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

// get sibling of treenode.
struct Q::QAllocator::treenode* Q::QAllocator::sibling(struct Q::QAllocator::treenode* node)
{
	if (node->parent == nil)
		return nullptr;
	if (node == node->parent->right)
		return node->parent->left;
	else
		return node->parent->right;
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

	node->left = pivot->right;
	if (pivot->right != nil)
		pivot->right->parent = node;

	node->parent = pivot;
	pivot->right = node;
}

// find a treenode by descriptor
struct Q::QAllocator::treenode* Q::QAllocator::find(descriptor_t descriptor)
{
	struct treenode* node = nil->parent;
	while(1)
	{
		if (node == nil)
			return nil;
		if (node->descriptor == descriptor)
			return node;
		if (descriptor > node->descriptor)
		{
			node = node->right;
			continue;
		}
		else
		{
			node = node->left;
			continue;
		}
	}
	return nil;
}

struct Q::QAllocator::treenode* Q::QAllocator::find_max_treenode(struct treenode* node)
{
	if (node == nil)
		return nil;
	struct treenode* next = node->right;
	while(next != nil)
	{
		node = next;
		next = next->right;
	}
	return node;
}

// DRY principles не выполнены. Срочно переделать.
// {
struct Q::QAllocator::treenode* Q::QAllocator::find_min_treenode(struct Q::QAllocator::treenode* node)
{
	if (node == nil)
		return nil;
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
	if (node == _nil)
		return _nil;
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
		std::cout << "Before : \n";
		DEBUG_print_tree();

		// ~~~Rebuilding RBTree from old buffer~~~
		// allocate new x2 buffer.

		Iterator beg = begin();
		Iterator e   = end();

		U8* newBuff = _treeBuffer;
		_treeBuffer = (U8*)malloc(_treeCapacity * 2);

		if (_treeBuffer == nullptr)
		{
			_treeBuffer = newBuff;
			return nullptr;
		}

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

//		void* _oldCurrentTreeTablePosition = _currentTreeTablePosition;
//		void* _oldBufferStart = (void*)((integer_value_of_pointer)nil + sizeof(struct treenode));
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
		while(beg != e)
		{
			struct treenode* newNode = add_node_in_tree((*beg)->descriptor);
			newNode->size 		= (*beg)->size;
			newNode->align 		= (*beg)->align;
			newNode->segment 	= (*beg)->segment;
			newNode->usefulData 	= (*beg)->usefulData;
			++beg;
		}

		// change capacity of TREE-TABLE.
		_treeCapacity *= 2;
		free(newBuff);

		std::cout << "after : \n";
		DEBUG_print_tree();
	}

	struct treenode* result = (struct treenode*)_currentTreeTablePosition;
	_currentTreeTablePosition = (void*)((integer_value_of_pointer)_currentTreeTablePosition + sizeof(struct treenode));
	return result;
}

// deallocate memory in TREE-TABLE.
// parameter1 - treenode.
void Q::QAllocator::deallocate_treenode_in_buffer(struct treenode* node)
{
	struct treenode* last = (struct treenode*)((integer_value_of_pointer)_currentTreeTablePosition - sizeof(struct treenode));
	*node = *last;
	bzero(last, sizeof(struct treenode));

	if (node != last)
	{
		if (node->right != nil)
			node->right->parent = node;
		if (node->left != nil)
			node->left->parent = node;

		if (node->parent == nil)
			nil->parent = node;
		else
		{
			if (node->parent->right == last)
				node->parent->right = node;
			else
				node->parent->left = node;
		}
	}

	_currentTreeTablePosition = (void*)((integer_value_of_pointer)_currentTreeTablePosition - sizeof(struct treenode));
}

// add treenode with descriptor from parameter to tree and balance tree.
struct Q::QAllocator::treenode* Q::QAllocator::add_node_in_tree( descriptor_t descriptor )
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

// remove treenode in tree.
// parameter1 - treenode.
void Q::QAllocator::remove_node_in_tree(descriptor_t descriptor)
{
	// find treenode with descriptor in tree;
	struct treenode* node = find(descriptor);

	if (node == nil)
		return;

	// find minimal element in right subtree where node - root;
	struct treenode* M = find_max_treenode(node->left);
	M = (M != nil ? M : node);

	std::cout << M->descriptor << '\n';

	if (M != node)
	{
		node->descriptor = M->descriptor;
		node->size 		 = M->size;
		node->align 	 = M->align;
		node->segment 	 = M->segment;
		node->usefulData = M->usefulData;
		/*
		swap(node->descriptor, maxRight->descriptor);
		swap(node->size, maxRight->size);
		swap(node->align, maxRight->align);
		swap(node->segment, maxRight->segment);
		swap(node->usefulData, maxRight->usefulData);
		*/
	}
	// get not-nil subtree of node M;
	struct treenode* N = (M->right != nil ? M->right : M->left);

	// if M hasn't children;
	if (N == nil)
	{
		if (M->parent == nil)
		{
			deallocate_treenode_in_buffer(M);
			return;
		}
//		if (M == M->parent->right)
//			M->parent->right = nil;
//		else
//			M->parent->left = nil;
		N = M;
	}
	else
	{
		// set N instead M.
		N->parent = M->parent;
		if (M->parent == nil)
			nil->parent = N;
		else
		{
			if (M == M->parent->right)
				M->parent->right = N;
			else
				M->parent->left  = N;
		}
	}
	if (!M->isRed)
	{
		if (!N->isRed)
			balance_tree_removing(N);
		else
			N->isRed = false;
	}
	if (N == M)
	{
		if (M->parent->right == M)
			M->parent->right = nil;
		else if (M->parent->left == M)
			M->parent->left = nil;
	}
	deallocate_treenode_in_buffer(M);
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
		return;
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

// after removing treenode -> balance tree, starts param.
// parementer1 - treenode - start for balancing.
void Q::QAllocator::balance_tree_removing(struct treenode* node)
{
	std::cout << "case1\n";
	if (node->parent == nil)
		return;
	std::cout << "case2\n";
	struct treenode* s = sibling(node);
	if (s->isRed)
	{
		node->parent->isRed = true;
		s->isRed = false;
		if (node == node->parent->left)
			rotate_left(node->parent);
		else
			rotate_right(node->parent);
	}
	std::cout << "case3\n";
	s = sibling(node);
	if ((!node->parent->isRed) && (!s->isRed) && (!s->left->isRed) && (!s->right->isRed))
	{
		s->isRed = true;
		balance_tree_removing(node->parent);
		return;
	}
	std::cout << "case4\n";
	s = sibling(node);
	if ((node->parent->isRed) && (!s->isRed) && (!s->left->isRed) && (!s->right->isRed))
	{
		s->isRed = true;
		node->parent->isRed = false;
		return;
	}
	std::cout << "case5\n";
	if (!s->isRed)
	{
		if (node == node->parent->left && !s->right->isRed && s->left->isRed)
		{
			s->isRed = true;
			s->left->isRed = false;
			rotate_right(s);
		}
		else if (node == node->parent->right && !s->left->isRed && s->right->isRed)
		{
			s->isRed = true;
			s->right->isRed = false;
			rotate_left(s);
		}
	}
	std::cout << "case6\n";
	s = sibling(node);
	s->isRed = node->parent->isRed;
	node->parent->isRed = false;

	if (node == node->parent->left)
	{
		s->right->isRed = false;
		rotate_left(node->parent);
	}
	else
	{
		s->left->isRed = false;
		rotate_right(node->parent);
	}
}

// allocate memory and return descriptor ( U32 ).
descriptor_t Q::QAllocator::allocate( U32 size, U8 align)
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

void Q::QAllocator::deallocate(descriptor_t descriptor)
{
	remove_node_in_tree(descriptor);
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
void* Q::QAllocator::get_pointer(descriptor_t descriptor)
{
	struct treenode* node = find(descriptor);
	if (node == nil)
		return nullptr;
	return node->usefulData;
}
