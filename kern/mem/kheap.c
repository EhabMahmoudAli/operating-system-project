
#include <inc/memlayout.h>
#include <inc/types.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"
#include "kheap.h"
struct frame1 arr[1048575];
uint32 incr=0;
int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()

	if(initSizeToAllocate > daLimit){
		return E_NO_MEM;
	}
	start = daStart;
	sbreak = start;
	ksize  = initSizeToAllocate;
	limit =	daLimit;
	uint32 numOfFrames = ROUNDUP(initSizeToAllocate,PAGE_SIZE)/PAGE_SIZE;
	struct FrameInfo *ptr = NULL;
	for(uint32 i=0;i<numOfFrames;i++){
		int alloc = allocate_frame(&ptr);
		if(alloc==E_NO_MEM){
			return E_NO_MEM;
		}
		else{
			map_frame(ptr_page_directory,ptr,sbreak,PERM_WRITEABLE);
			ptr->va = sbreak;
			sbreak+=PAGE_SIZE;
		}
	}
	initialize_dynamic_allocator(daStart, initSizeToAllocate);
	return 0;
}

void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	if(sbreak+increment >= limit || sbreak +increment <start){
		panic("NOT Enough Memory!!!\n");
	}
	if(increment > 0){
		uint32 oldsbrk = sbreak;
		uint32 numOfFrames = (ROUNDUP(increment,PAGE_SIZE))/PAGE_SIZE;
		if(increment + sbreak < ROUNDUP(sbreak,PAGE_SIZE)){
			sbreak = ROUNDUP(sbreak,PAGE_SIZE);
			return (void*)oldsbrk;
		}
		else if(ROUNDUP(sbreak,PAGE_SIZE)-sbreak >= increment-PAGE_SIZE){
			uint32 numOfFrames = ((increment))/PAGE_SIZE;
		}
		sbreak = ROUNDUP((sbreak),PAGE_SIZE);
		for(int i=0;i<numOfFrames;i++){
			struct FrameInfo *ptr1 = NULL;
			int alloc = allocate_frame(&ptr1);
			if(alloc==E_NO_MEM){
				return NULL;
			}
			else{
				map_frame(ptr_page_directory,ptr1,sbreak,PERM_WRITEABLE);
				ptr1->va=sbreak;
				sbreak+=PAGE_SIZE;
			}
		}
		return (void*)oldsbrk;
		/*
		sbreak = ROUNDUP((sbreak+increment),PAGE_SIZE);
		return (void*)oldsbrk;
		//uint32 numOfFrames = (ROUNDUP(increment,PAGE_SIZE)/PAGE_SIZE);
		sbreak = ROUNDUP(sbreak,PAGE_SIZE);
		for(uint32 i = 0; i < numOfFrames; i++){
			struct FrameInfo *ptr = NULL;
			int alloc1 = allocate_frame(&ptr);
			if(alloc1==E_NO_MEM){
				return (void*)-1;
			}
			else{

				map_frame(ptr_page_directory,ptr,sbreak,PERM_WRITEABLE);
				cprintf("sbreakin2 = %x\n",sbreak);
				sbreak+=PAGE_SIZE;
				cprintf("sbreakin3 = %x\n",sbreak);
			}
		}
		cprintf("oldsbreak = %x\n",oldsbrk);
		return (void*)oldsbrk;
		*/
	}
	else if(increment == 0){
		return (void*)sbreak;
	}
	else if(increment < 0){
		sbreak+=increment;
		incr -= increment;
		uint32 numOfFrames = (ROUNDDOWN(incr,PAGE_SIZE))/PAGE_SIZE;
		uint32 x = sbreak;
		if(numOfFrames >= 1){
			uint32 x = (ROUNDUP(sbreak,PAGE_SIZE));
		}
		for(uint32 i=0;i<numOfFrames;i++){
			unmap_frame(ptr_page_directory,ROUNDUP(x,PAGE_SIZE));
			x+=PAGE_SIZE;
			incr-=PAGE_SIZE;
		}
		return (void*)sbreak;
	}
	return (void*)-1 ;
}


void* kmalloc(unsigned int size)
{

	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()

	if(isKHeapPlacementStrategyFIRSTFIT() == 1){
		if(size <= (DYN_ALLOC_MAX_BLOCK_SIZE)){
			return (void*)alloc_block_FF(size);
		}
		else{
			uint32 address = limit+PAGE_SIZE;
			uint32 saddress = address;
			uint32 *ptr_page_table = NULL;
			uint32 c = 0;
			uint32 numOfFrames = ROUNDUP(size,PAGE_SIZE)/PAGE_SIZE;
			uint32 frames = (KERNEL_HEAP_MAX-address)/PAGE_SIZE;
			if(numOfFrames > frames){
				return (void*)NULL;
			}
			struct FrameInfo *ptr = NULL;
			for(int i = 0;i<frames;i++){
				ptr = get_frame_info(ptr_page_directory, address, &ptr_page_table);
				if(ptr == NULL ||(ptr != NULL && ptr->references == 0)){
				   c++;
				}
				else{
					saddress = address + PAGE_SIZE;
					c = 0;
				}
				address += PAGE_SIZE;
				if(c == numOfFrames){
					break;
				}
			}
			uint32 temp2 = saddress;
			if(c < numOfFrames){
				return NULL;
			}
			for(uint32 i=0;i<numOfFrames;i++){
				int alloc = allocate_frame(&ptr);
				if(alloc==E_NO_MEM){
					return NULL;
				}
				else{
					map_frame(ptr_page_directory,ptr,temp2, PERM_WRITEABLE);
					ptr->va=temp2;
					temp2+=PAGE_SIZE;
				}
			}
			arr[index].va=(void *)saddress;
			arr[index].count=numOfFrames;
			index++;
			return (void*)saddress;
		}
	}
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	if(isKHeapPlacementStrategyFIRSTFIT() == 1){
	if(virtual_address>=(void *)start && virtual_address<(void*)limit)
	{
		free_block(virtual_address);
	}
	else if(virtual_address>=(void *)(limit+PAGE_SIZE) && virtual_address<(void *)KERNEL_HEAP_MAX)
	{
		uint32 numOfFrames=0;

		for(int i=0;i<index;i++)
		{
			if(virtual_address==arr[i].va){
				numOfFrames=arr[i].count;
			}

		}
		for(int i=0;i<numOfFrames;i++)
		{
		unmap_frame(ptr_page_directory,(uint32)virtual_address);
		virtual_address+=PAGE_SIZE;
		}
	}
	else
	{

		panic("NOT Enough Memory!!!.....\n");
	}
	//panic("kfree() is not implemented yet...!!");
}
}

//start


//end
//these lines added by ehab mahmoud to make a hash table to work in o(1) order
//instead of doing a for loop
unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");
	//start
	struct FrameInfo* f = to_frame_info(physical_address); //return data about this physical address
		uint32 offset = (physical_address << 20) >> 20;
		if (f->references != 0){ // pointing to someplace
			uint32 virtual_address = (((f->va) >> 12 ) << 12) + offset;
			return virtual_address;
		}
		return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");
	int32 page_table_index = PTX(virtual_address);
	uint32 offset = (virtual_address << 20) >> 20;
	uint32* ptr_page_table = NULL;
	get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	uint32 frame_number = (ptr_page_table[page_table_index] >> 12) << 12;
	uint32 physical_address = frame_number + offset;
	return physical_address;
	//finally done
	//ali tarek
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	if(virtual_address == NULL){
		return kmalloc(new_size);
	}
	else if(new_size == 0){
		 kfree(virtual_address);
		 return NULL ;
	}
	else if(virtual_address>=(void *)start && virtual_address<(void*)limit){
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			return realloc_block_FF(virtual_address,new_size);
		}
		else{
			void *ptr = kmalloc(new_size);
			if(ptr == NULL){
				return NULL;
			}
			else{
				free_block(virtual_address);
			}
			return ptr;
		}
	}
	else if(virtual_address>=(void *)(limit+PAGE_SIZE) && virtual_address<(void *)KERNEL_HEAP_MAX)
	{
		if(new_size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			void * ptr =alloc_block_FF(new_size);
			if(ptr == NULL){
				return NULL;
			}
			else{
				kfree(virtual_address);
			}
			return ptr;
		}
		else{
			uint32 frames=ROUNDUP(new_size,PAGE_SIZE)/PAGE_SIZE;
			uint32 numOfFrames=0;
			for(int i=0;i<index;i++)
			{
				if(virtual_address==arr[i].va){
					numOfFrames=arr[i].count;
				}

			}
			uint32 newframes=frames-numOfFrames;
			if(newframes>0)
			{
			uint32 *ptr_page_table = NULL;
			struct FrameInfo *ptr = NULL;
			uint32 saddress=(uint32)virtual_address;
			uint32 address=saddress;
			uint32 c=0;
			for(int i = 0;i<newframes;i++){
				ptr = get_frame_info(ptr_page_directory, address, &ptr_page_table);
				if(ptr == NULL ||(ptr != NULL && ptr->references == 0)){
				   c++;
				}
				else{
					saddress = address + PAGE_SIZE;
					c = 0;
				}
				address += PAGE_SIZE;
				if(c ==newframes ){
					break;
				}
			}
			uint32 temp2 = saddress;
			if(c < newframes){
				void * ptr =kmalloc(new_size);
				if(ptr == NULL){
					return NULL;
				}
				else{
					kfree(virtual_address);
				}
				return ptr;

			}
				for(uint32 i=0;i<newframes;i++){
					int alloc = allocate_frame(&ptr);
					if(alloc==E_NO_MEM){
						void * ptr =kmalloc(new_size);
						if(ptr == NULL){
							return NULL;
						}
						else{
							kfree(virtual_address);
						}
						return ptr;
					}
					else{
						map_frame(ptr_page_directory,ptr,temp2, PERM_WRITEABLE);
						temp2+=PAGE_SIZE;
					}
				}
				arr[index].count=newframes;
				//get_frame_info(ptr_page_directory, saddress, &ptr_page_table)->count = numOfFrames;
				return virtual_address;

			}
			else if(newframes<0){
				 newframes=numOfFrames-frames;
				 kfree(virtual_address+(newframes*PAGE_SIZE));
				 return virtual_address;
			}
			else{
				return virtual_address;
			}
		}
	}
	return NULL;
	//panic("krealloc() is not implemented yet...!!");	 */
}
//kheap.c : done
