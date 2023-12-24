	/*dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */

#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================

struct MemBlock_LIST List;
bool is_initialized = 0;
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return;
	is_initialized = 1;
	//=========================================
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)daStart) ;
	curBlkMetaData->size=initSizeOfAllocatedSpace;
	curBlkMetaData->is_free=1;
	LIST_INIT(&List);
	LIST_INSERT_TAIL(&List,curBlkMetaData);
	//=========================================
}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()

	if(size==0)
		return NULL;

	if (!is_initialized)
	{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}

	struct BlockMetaData* ptr= LIST_FIRST(&List);
	uint32 oldSize = 0;
	uint32 counter=1;

	while(1){
		if(ptr->is_free == 1 && (ptr->size) == (size + sizeOfMetaData())){
			ptr->is_free = 0;
			return (ptr+1);
		}
		else if(ptr->is_free == 1 && (ptr->size) > (size + sizeOfMetaData())){
			if((ptr->size-(size+sizeOfMetaData()))<=sizeOfMetaData()){
				ptr->is_free = 0;
				return (ptr+1);
			}
			oldSize = ptr->size;
			ptr->is_free = 0;
			ptr->size = size + sizeOfMetaData();
			char *x = (char *)ptr + sizeOfMetaData() + size;
			struct BlockMetaData *Block = (struct BlockMetaData *) x;//user
			Block->size = oldSize - (size+sizeOfMetaData());
			Block->is_free = 1;
			LIST_INSERT_AFTER(&List,ptr,Block);
			return (ptr+1);
		}
		if(counter==LIST_SIZE(&List))
			break;
		if(ptr->prev_next_info.le_next !=NULL){
			ptr = ptr->prev_next_info.le_next;
			counter++;
		}
		else
			break;
	}

	if(counter==LIST_SIZE(&List)){
		uint32* poi = (uint32*)sbrk(size + sizeOfMetaData());
		if(poi!=(uint32 *)-1){
			uint32 all = size + sizeOfMetaData();
			struct BlockMetaData *ptr1 = (struct BlockMetaData *)poi;
			struct BlockMetaData *ptr2 = (struct BlockMetaData *)((uint32)ptr1 + all);
			ptr1->size=all;
			ptr1->is_free=0;
			ptr2->size=PAGE_SIZE-all;
			ptr2->is_free=1;
			LIST_INSERT_TAIL(&List,ptr1);
			LIST_INSERT_TAIL(&List,ptr2);
			return (ptr1+1);
		}
	}
	return NULL;
}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================

void *alloc_block_BF(uint32 size)
{
		//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
		//panic("alloc_block_BF is not implemented yet");
		//return NULL;
	//done by karim
	if(size == 0)
			{return NULL;}
			uint32 min= 1000000000;
			struct BlockMetaData* ptr= LIST_FIRST(&List);
			struct BlockMetaData* minptr= LIST_FIRST(&List);
			uint32 oldSize = 0;
			uint8 found = 2;
			while(10000000==10000000){
				if(ptr->is_free == 1 && (ptr->size) == (size + sizeOfMetaData())){ptr->is_free = 0;minptr = ptr;found = 0;break;}
				else if(ptr->is_free == 1 && (ptr->size) > (size + sizeOfMetaData()) && ptr->size < min){found = 1;minptr = ptr;min = ptr->size;}
				if(ptr->prev_next_info.le_next!=NULL)
					ptr = ptr->prev_next_info.le_next;
				else
					break;
			}
			if(found == 0){
				return (minptr+1);
			}
			else if(found == 1){
				if(minptr->size - (size+sizeOfMetaData()) < sizeOfMetaData()){
					minptr->is_free=0;
				}
			else{
					oldSize = minptr->size;
					minptr->is_free = 0;
					minptr->size = size + sizeOfMetaData();
					char *x = (char *)minptr + sizeOfMetaData() + size;
					struct BlockMetaData *Block = (struct BlockMetaData *) x;//user
					Block->size = oldSize - (size+sizeOfMetaData());
					Block->is_free = 1;
					LIST_INSERT_AFTER(&List,minptr,Block);
			   }
			    return (minptr+1);
			}
			else
				return NULL;
			//panic("alloc_block_BF is not implemented yet");
}


//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	if(va==NULL)
		return;
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");

	struct BlockMetaData* ptrLast= LIST_LAST(&List);
	struct BlockMetaData* ptrFirst= LIST_FIRST(&List);
	struct BlockMetaData *ptr= (struct BlockMetaData*)va-1;
	//Scenario 1 No Coalesce

	if(ptr->prev_next_info.le_next == NULL && LIST_PREV(LIST_LAST(&List))->is_free == 1){
			LIST_PREV(LIST_LAST(&List))->size += ptr->size;
			ptr->size = 0;
			ptr->is_free = 0;
			LIST_REMOVE(&List,ptr);
	}
	else if(ptrFirst == ptr && ptr->prev_next_info.le_next->is_free == 1){
		ptr->size += ptr->prev_next_info.le_next->size;
		ptr->is_free = 1;
		ptr->prev_next_info.le_next->is_free = 0;
		ptr->prev_next_info.le_next->size = 0;
		ptr=ptr->prev_next_info.le_next;
		LIST_REMOVE(&List,ptr);
	}
	else if(LIST_PREV(LIST_LAST(&List))->is_free == 0 && ptr->prev_next_info.le_next == NULL){
		ptr->is_free = 1;
	}
	else if(ptr->prev_next_info.le_next->is_free == 0 && ptr->prev_next_info.le_prev == NULL){
		ptr->is_free = 1;
	}


	else if(ptr->prev_next_info.le_next->is_free == 0 && ptr->prev_next_info.le_prev->is_free == 0){
		ptr->is_free = 1;
	}

	else if(ptr->prev_next_info.le_next->is_free == 0 && ptr->prev_next_info.le_prev->is_free == 1){
			ptr->prev_next_info.le_prev->size+=ptr->size;
			ptr->size=0;
			ptr->is_free=0;
			LIST_REMOVE(&List,ptr);
		}
	else if(ptr->prev_next_info.le_next->is_free == 1 && ptr->prev_next_info.le_prev->is_free == 0){
				ptr->size += ptr->prev_next_info.le_next->size;
				ptr->is_free = 1;
				ptr->prev_next_info.le_next->is_free = 0;
				ptr->prev_next_info.le_next->size = 0;
				ptr=ptr->prev_next_info.le_next;
				LIST_REMOVE(&List,ptr);
			}
	else if(ptr->prev_next_info.le_next->is_free == 1 && ptr->prev_next_info.le_prev->is_free == 1){

					ptr->prev_next_info.le_prev->size += ptr->size + ptr->prev_next_info.le_next->size;
					ptr->prev_next_info.le_prev->is_free = 1;
					ptr->is_free = 0;
					ptr->size = 0;
					ptr->prev_next_info.le_next->is_free = 0;
					ptr->prev_next_info.le_next->size = 0;
					struct BlockMetaData *ptr1= (struct BlockMetaData*)ptr->prev_next_info.le_next;
					LIST_REMOVE(&List,ptr1);
					LIST_REMOVE(&List,ptr);

			}
	else if(ptr == ptrFirst && ptr->prev_next_info.le_next->is_free == 0){
		ptr->is_free = 1;
	}

}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
//ehab and ali
//2 new added functions prevblock and nextblock
//the purpose of these 2 functions to make the re allocated block in the heap have
//an access to the next block and the previous one
//previous
void *PREV_BLOCK(void *block){
	struct BlockMetaData*metadata = (struct BlockMetaData*)((char*)block - sizeOfMetaData());
	uint32 block_size = metadata->size;
	void*prev_block=(char*)block-block_size;
	return prev_block;
}

//next
void*NEXT_BLOCK(void*block){
	struct BlockMetaData*metadata = (struct BlockMetaData*)((char*)block-sizeOfMetaData());
	uint32 block_size = metadata->size;
	void*next_block = (char*)block + block_size;
	return next_block;
}

void *realloc_block_FF(void* va, uint32 new_size)
{

	// ehab mahmoud and ali tarek task
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()

	if (new_size > get_block_size((void*)USER_LIMIT)){
		 return NULL;
	 }
	 if (va == NULL) {
		if (new_size == 0) {
			return NULL;
		} else {
			return alloc_block_FF(new_size);
		}
	 } else if (new_size == 0 && va != NULL) {
		 free_block(va);
		 return NULL;
	 }

	struct BlockMetaData* metadata = (struct BlockMetaData*)((char*)va - sizeOfMetaData());
	uint32 original_size = metadata->size;

	if (original_size == new_size) {
		return va;
	} else if (original_size > new_size) {
		if (original_size - new_size > sizeOfMetaData()) {
			struct BlockMetaData* split_block = (struct BlockMetaData*)((char*)metadata + sizeOfMetaData() + new_size);
			split_block->size = original_size - new_size - sizeOfMetaData();
			split_block->is_free = 1;
			metadata->size = new_size + sizeOfMetaData();
			LIST_INSERT_AFTER(&List, metadata, split_block);
			if(split_block->prev_next_info.le_next->is_free==1)
			{
				split_block->size += split_block->prev_next_info.le_next->size;
				split_block->is_free = 1;
				split_block->prev_next_info.le_next->is_free = 0;
				split_block->prev_next_info.le_next->size = 0;
				split_block=split_block->prev_next_info.le_next;
				LIST_REMOVE(&List,split_block);
			}

		}
		return va;
	} else { // original_size < new_size
			struct BlockMetaData* next_block = NEXT_BLOCK(va);
			uint32 next_size = get_block_size(next_block);

			if (next_size > 0 && next_size + original_size >= new_size+sizeOfMetaData() && metadata->prev_next_info.le_next->is_free) {
				if (next_size + original_size - new_size > sizeOfMetaData()) {
					free_block((void*)va);
					metadata->is_free=0;
					uint32 old=metadata->size;
					metadata->size=new_size+sizeOfMetaData();
					struct BlockMetaData* split_block = (struct BlockMetaData*)((char*)metadata +metadata->size);
					split_block->size=old-metadata->size;
					split_block->is_free=1;
					LIST_INSERT_AFTER(&List, metadata, split_block);
				} else {
						metadata->size = original_size + next_size;
						LIST_REMOVE(&List,next_block);
					}

					return va;
				} else { // next block is not free

					struct BlockMetaData* ptr= LIST_FIRST(&List);
					uint32 oldSize = 0;
					uint32 counter=1;

					while(1){

						if(ptr->is_free == 1 && (ptr->size) == (new_size + sizeOfMetaData())){
							ptr->is_free = 0;

							free_block((void*)va);
							return (ptr+1);
						}
						else if(ptr->is_free == 1 && (ptr->size) > (new_size + sizeOfMetaData())){
							if((ptr->size-(new_size+sizeOfMetaData()))<=sizeOfMetaData()){
								ptr->is_free = 0;
								free_block((void*)va);
								return (ptr+1);
							}
							oldSize = ptr->size;
							ptr->is_free = 0;
							void*va2=(void*) (ptr+1);
							short *ba=(short*)va2;
							uint32 h=((uint32)(va2))+new_size;
							short z=*(short*)va;
							while(ba<=(short*)h)
							{
								*ba=z;
								ba++;
							}

							ptr->size =  (new_size) + sizeOfMetaData();
							char *x = (char *)ptr + sizeOfMetaData() + new_size;
							struct BlockMetaData *Block = (struct BlockMetaData *) x;//user
							Block->size = oldSize - (new_size+sizeOfMetaData());
							Block->is_free = 1;
							LIST_INSERT_AFTER(&List,ptr,Block);
							free_block((void*)va);
							return (ptr+1);
						}
						if(counter==LIST_SIZE(&List))
							break;
						if(ptr->prev_next_info.le_next !=NULL){
							ptr = ptr->prev_next_info.le_next;
							counter++;
						}
						else
							break;
					}
				}
			}

	return va;
}
