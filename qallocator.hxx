#ifndef _QALLOCATOR_H_
#define _QALLOCATOR_H_

#include <iostream>
#include <mutex>

#include "setup.hxx"

#define DEFAULT_DATA_BUFFER_SIZE 1024 	// bytes
#define DEFAULT_TREE_TABLE_SIZE  512 	// bytes

namespace Q
{

	class QAllocator
	{
	private:
		// multithread-nesessary dependencies.
		mutable std::mutex m;
		
		// node of RBT for efficient searching pointer by descriptor
		struct treenode
		{
			struct treenode* parent;	// pointer to parent of node
			U32 descriptor;			// descriptor of segment
			U32 size;			// size of segment
			bool isRed;			// Indicator of color (Red/Black)
			void* segment;			// pointer to segment of memory
			void* usefulData;		// pointer to useful data (segment-pointer + align)
			struct treenode *left, *right;	// pointer to left and right
		};

		U32 _dataCapacity;	// capacity of buffer
		U32 _treeCapacity;	// capacity of table with descriptors

		U8* _dataBuffer;	// pointer to data buffer
		U8* _treeBuffer;	// pointer to table with descriptors

		//	DATA-BUFFER
		//	 ____________________________________________________
		//	| T1  |T2|  T3  | T4 |T5|           FREE             |
		//	|_____|__|______|____|__|____________________________| 
		//
		//	There are 5 segments of different sizes in data buffer.
		//
		//
		//	TREE-TABLE
		//	 ____________________________________________________
		//	| nil | T1  |  T2 | T3  |  T4 | T5  |  	  FREE       |
		//	|_____|_____|_____|_____|_____|_____|________________|
		//
		//	There are 6 treenodes(5 descriptors + 1 nil-node) in buffer.

		U32 _currentDescriptor;
		
		struct treenode* nil;	
		/*
			nil-treenode
			 	  
			   _______
			  |	  |
			  |	 nil
			  |	   
			  |
			  |_____
			        |
			        V
			       root     
			      /    \
			     X      X   
			    / \    / \
			   X   X    X  nil
			  /\   /\   /\
			  nil  nil  nil    
		*/

		// ~~~POINTERS~~~
		
		void* _currentTreeTablePosition;	// position on TREE-TABLE for next treenode.
		void* _currentDataBufferPosition;	// position on DATA-BUFFER for next segment.

		// ~~~ADDITIONS FUCTIONS AND METHODS~~~

		void initBuffers(U32, U32); // init all parameters in constructor.
		
		// allocate memory for treenode in TREE-TABLE. (this func uses in add_node_in_tree(U32)).
		struct treenode* allocate_treenode_in_buffer();

		// add treenode in tree with descriptor.
		// parameter1 - descriptor.
		struct treenode* add_node_in_tree(U32);
		
		// after adding treenode in tree -> balance tree, starts param.
		// parameter1 - treenode - start for balancing.
		void balance_tree_insertion(struct treenode*);

		struct treenode* grandparent(struct treenode*);		// find grandparent of treenode.
		struct treenode* uncle(struct treenode*);		// find uncle of treenode.
		void rotate_left(struct treenode*);			// rotate left of node.
		void rotate_right(struct treenode*);			// rotate right of node.

		// resize data buffer
		// parameter1 - new size.
		bool resize_data_buffer(U32);
		
	public:
		// constructors
		QAllocator( U32 = DEFAULT_DATA_BUFFER_SIZE, U32 = DEFAULT_TREE_TABLE_SIZE );
		~QAllocator();

		int DEBUG_get_treenode_size() const
		{
			return sizeof(struct treenode);
		}

		void DEBUG_print_tree_table()
		{
			for(U32 i = 0; i < _treeCapacity; i++)
			{	
				if (i != 0 && i % sizeof(struct treenode) == 0)
					std::cout << '\n';
				std::cout << (int)(_treeBuffer[i]) << ' ';
			}
			std::cout << '\n';
		}

		void DEBUG_print_data_buffer()
		{
			for (U32 i = 0; i < _dataCapacity; i++)
			{
				std::cout << (int)(_dataBuffer[i]) << ' ';
			}
			std::cout << '\n';
		}
		// allocate memory and return descriptor ( U32 ).
		// parameter1 - size of segment;
		// parameter2 - align
		U32 allocate( U32, U8 = 8 );
		// get descriptor ( U32 ) and deallocate memory.
		void deallocate( U32 );
		// defragmentate some segments in data heap.
		bool some_defragmentation( U16 );
		// defragmentate all segments.
		bool defragmentation();
		// get descriptor ( U32 ) and return pointer.
		// parameter1 - descriptor.
		void* get_pointer( U32 );
	};
}

#endif
