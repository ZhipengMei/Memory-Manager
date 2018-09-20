#include "MemoryManager.h"
#include <iomanip>
#include <iostream>
using namespace std;

namespace MemoryManager
{
	// IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
	//
	// This is the only static memory that you may use, no other global variables
	// may be created, if you need to save data make it fit in MM_pool
	//
	// IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT

	static int usedMem;
	static int inUseMem;

	const int MM_POOL_SIZE = 65536;
	char MM_pool[MM_POOL_SIZE];

	// I have provided this tool for your use
	void memView(int start, int end)
	{
		const unsigned int SHIFT = 8;
		const unsigned int MASK = 1 << SHIFT - 1;

		unsigned int value;	// used to facilitate bit shifting and masking

		cout << "         Pool                     Unsignd  Unsigned " << endl;
		cout << "Mem Add  indx   bits   chr ASCII#  short      int    " << endl;
		cout << "-------- ---- -------- --- ------ ------- ------------" << endl;

		for (int i = start; i <= end; i++)
		{
			cout << (long*)(MM_pool + i) << ':';	// the actual address in hexadecimal
			cout << '[' << setw(2) << i << ']';		// the index into MM_pool

			value = MM_pool[i];
			cout << " ";
			for (int j = 1; j <= SHIFT; j++)	// the bit sequence for this byte (8 bits)
			{
				cout << ((value & MASK) ? '1' : '0');
				value <<= 1;
			}
			cout << " ";

			cout << '|' << *(char*)(MM_pool + i) << "| (";		// the ASCII character of the 8 bits (1 byte)
			cout << setw(4) << ((int)(*((unsigned char*)(MM_pool + i)))) << ")";	// the ASCII number of the character

			cout << " (" << setw(5) << (*(unsigned short*)(MM_pool + i)) << ")";	// the unsigned short value of 16 bits (2 bytes)
			cout << " (" << setw(10) << (*(unsigned int*)(MM_pool + i)) << ")";	// the unsigned int value of 32 bits (4 bytes)

			cout << endl;
		}
	}

	// Initialize set up any data needed to manage the memory pool
	void initializeMemoryManager(void)
	{
		// your solution goes here
		int freeHead = 0;	//starting index of the freelist
		int inUseHead = 2;	//starting index of the inUselist
		int usedHead = 4;	//starting index for the used list

		int nextLink = 2;	//offset index of the next link
		int prevLink = 4;	//offset index for the prev link

		*(unsigned short*)(MM_pool + freeHead) = 6;			//freelist starts at byte 6
		*(unsigned short*)(MM_pool + 6) = MM_POOL_SIZE - 6;	//we used 6 bytes to get things started
		*(unsigned short*)(MM_pool + inUseHead) = 0;		//nothing in the inUse list yet
		*(unsigned short*)(MM_pool + usedHead) = 0;			//nothing in the used list yet
	}


	// return a pointer inside the memory pool
	// If no chunk can accommodate aSize call onOutOfMemory() - still 
	void* allocate(int aSize)
	{

		//define locations for the lists
		int freeHead = 0;	//starting index of the freelist
		int inUseHead = 2;	//starting index of the inUselist
		int usedHead = 4;	//starting index of the used list - deallicated memory


		if (freeMemory() > aSize) {

			int size = aSize + 6;		//offset index from the Node start to the size of memory allocated
			int oldFreeHead = *(unsigned short*)(MM_pool + freeHead);			// previous node's freeHead
			int oldinUseHead = *(unsigned short*)(MM_pool + inUseHead);			// previous node's inUseHead
			int nextFreeHead = *(unsigned short*)(MM_pool + freeHead) + size;	// new noed's next freeHead

			int oldPrevLink = oldinUseHead + 4;	//offset previous node's index from the Node start of the previous link

			int currNextLink = oldFreeHead + 2;	//offset current node's index from the Node start of the next link
			int currPrevLink = oldFreeHead + 4;	//offset current node's index from the Node start of the previous link

			int currinUseHead = *(unsigned short*)(MM_pool + inUseHead); // new inUseHead

			// added new node, refresh freeHead and inUseHead
			*(unsigned short*)(MM_pool + inUseHead) = oldFreeHead;		// current inUseHead 
			*(unsigned short*)(MM_pool + freeHead) = nextFreeHead;		// current freeHead 

			// setup prevLink of the previous node
			if (oldFreeHead != 6) {
				*(unsigned short*)(MM_pool + oldPrevLink) = oldFreeHead;
			}

			// setup nextLink and prevLink of the newly added node
			*(unsigned short*)(MM_pool + currNextLink) = oldinUseHead;	// current nextLink
			*(unsigned short*)(MM_pool + currPrevLink) = 0;				// current prevLink

			// storing aSize into memory
			*(unsigned short*)(MM_pool + oldFreeHead) = aSize;

			// Free Memory left
			*(unsigned short*)(MM_pool + nextFreeHead) = MM_POOL_SIZE - nextFreeHead;

			//add size into inUseMem
			inUseMem += size;

			//return new memory address starting after size block
			return (void*)(MM_pool + *(unsigned short*)MM_pool - aSize);

		}
		else {
			onOutOfMemory();
			return 0;
		}
	}

	// Free up a chunk previously allocated
	void deallocate(void* aPointer)
	{

		int size = MM_pool[((char*)aPointer - MM_pool) - 6] + 6;		// parse aPointer address into a value at that address
		int index = (char*)aPointer - MM_pool - 6;	// parse aPointer address into an pool index
		int currDelNode = index;

		//define locations for the lists
		int freeHead = 0;	//starting index of the freelist
		int inUseHead = 2;	//starting index of the inUselist
		int usedHead = 4;	//starting index of the used list - deallicated memory

		int nextLink = 2;	//offset index of the next link
		int prevLink = 4;	//offset index for the prev link

		int prevNode = *(unsigned short*)(MM_pool + currDelNode + nextLink);	//new prev index for node before allocate node
		int nextNode = *(unsigned short*)(MM_pool + currDelNode + prevLink);	//new next index for head node

		//rearranging the linked node
		*(unsigned short*)(MM_pool + prevNode + prevLink) = nextNode;
		*(unsigned short*)(MM_pool + nextNode + nextLink) = prevNode;

		// oldUsedHead
		int oldUsedHead = *(unsigned short*)(MM_pool + usedHead);

		// unlink the deleted node
		*(unsigned short*)(MM_pool + currDelNode + prevLink) = 0;	//previous node is always 0 for the head node
		*(unsigned short*)(MM_pool + currDelNode + nextLink) = oldUsedHead;	//next node is the olde usedHead

		// Previous deleted node's previous node should be currently delete node
		*(unsigned short*)(MM_pool + oldUsedHead + prevLink) = currDelNode;

		//set the usedHead to the most recent deleted node
		*(unsigned short*)(MM_pool + usedHead) = currDelNode;

		//add size into usedMem
		usedMem += size;
	}

	//---
	//--- support routines
	//--- 

	// Will scan the memory pool and return the total free space remaining
	int freeMemory(void)
	{
		// total pool size - value located at freeHead
		int freeHead = 0;	//starting index of the freelist
		return MM_POOL_SIZE - *(unsigned short*)(MM_pool + freeHead);
	}

	// Will scan the memory pool and return the total deallocated memory
	int usedMemory(void)
	{
		return usedMem;
	}

	// Will scan the memory pool and return the total in use memory
	int inUseMemory(void)
	{ 
		int totalUsedMem = MM_POOL_SIZE - freeMemory() - 6;
		return totalUsedMem - usedMem;
	}

	// Call if no space is left for the allocation request
	void onOutOfMemory(void)
	{
		cout << "Memory pool out of memory" << endl;
	}

}
