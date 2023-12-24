#include <inc/queue.h>
//new to use list inser head
#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"
//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}
//Handle the page fault
void page_fault_handler(struct Env * curenv, uint32 fault_va){
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX)){
	if(LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)) < (curenv->page_WS_max_size))
	{
		struct WorkingSetElement *ptr_WS_element;
		LIST_FOREACH(ptr_WS_element, &(curenv->SecondList)){
				if(ROUNDDOWN(ptr_WS_element->virtual_address,PAGE_SIZE) == ROUNDDOWN(fault_va,PAGE_SIZE)){
				struct WorkingSetElement *ele = LIST_LAST(&(curenv->ActiveList));
				env_page_ws_print(curenv);
				pt_set_page_permissions(curenv->env_page_directory,fault_va , PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
				LIST_REMOVE(&curenv->SecondList,ptr_WS_element);
				if(LIST_SIZE(&(curenv->ActiveList)) == curenv->ActiveListSize){
					LIST_REMOVE(&curenv->ActiveList,ele);
					pt_set_page_permissions(curenv->env_page_directory,ele->virtual_address, PERM_WRITEABLE | PERM_USER,PERM_PRESENT);
					LIST_INSERT_HEAD(&curenv->SecondList,ele);
				}
				LIST_INSERT_HEAD(&curenv->ActiveList,ptr_WS_element);
				env_page_ws_print(curenv);
				return;
				}
			}
				struct FrameInfo *new_frame;
				uint32 perm = PERM_USER | PERM_WRITEABLE | PERM_PRESENT;
				allocate_frame(&new_frame);
				map_frame(curenv->env_page_directory, new_frame, fault_va, perm);
				// 2- Read the faulted page from page file to memory
				int ret1 = pf_read_env_page(curenv, (void *)fault_va);
				bool exisits_in_user_heap=0;
				bool exisits_in_user_stack=0;
				if(fault_va>=USTACKBOTTOM && fault_va<USTACKTOP){
					exisits_in_user_stack=1;
				}
				if(fault_va>=USER_HEAP_START && fault_va<USER_HEAP_MAX){
					exisits_in_user_heap=1;
				}
				if (ret1 == E_PAGE_NOT_EXIST_IN_PF)
				{
					if (exisits_in_user_heap == 0 && exisits_in_user_stack == 0)
					{
							sched_kill_env(curenv->env_id);
					}
				}

				env_page_ws_print(curenv);
				struct WorkingSetElement *new_element = env_page_ws_list_create_element(curenv, fault_va);
				if(LIST_SIZE(&(curenv->ActiveList)) < curenv->ActiveListSize){
					LIST_INSERT_HEAD(&curenv->ActiveList,new_element);
				}
				else{
					struct WorkingSetElement *ele = LIST_LAST(&(curenv->ActiveList));
					env_page_ws_print(curenv);
					LIST_REMOVE(&curenv->ActiveList,ele);
					pt_set_page_permissions(curenv->env_page_directory,ele->virtual_address , PERM_USER | PERM_WRITEABLE ,PERM_PRESENT);
					LIST_INSERT_HEAD(&curenv->SecondList,ele);
					pt_set_page_permissions(curenv->env_page_directory,fault_va , PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
					LIST_INSERT_HEAD(&curenv->ActiveList,new_element);
				}
	}
	else{
			// 2- Read the faulted page from page file to memory
				struct WorkingSetElement *ptr_WS_element;
				LIST_FOREACH(ptr_WS_element, &(curenv->SecondList)){
					if(ROUNDDOWN(ptr_WS_element->virtual_address,PAGE_SIZE) == ROUNDDOWN(fault_va,PAGE_SIZE)){
							struct WorkingSetElement *ele = LIST_LAST(&(curenv->ActiveList));
							env_page_ws_print(curenv);
							LIST_REMOVE(&curenv->SecondList,ptr_WS_element);
								LIST_REMOVE(&curenv->ActiveList,ele);
								pt_set_page_permissions(curenv->env_page_directory,ele->virtual_address, PERM_WRITEABLE | PERM_USER,PERM_PRESENT);
								LIST_INSERT_HEAD(&curenv->SecondList,ele);
							pt_set_page_permissions(curenv->env_page_directory,ptr_WS_element->virtual_address , PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
							LIST_INSERT_HEAD(&(curenv->ActiveList),ptr_WS_element);
							env_page_ws_print(curenv);
							return;
							}
					}
				struct FrameInfo *new_frame;
				uint32 perm = PERM_USER | PERM_WRITEABLE;
				allocate_frame(&new_frame);
				map_frame(curenv->env_page_directory, new_frame, fault_va, perm);
				int ret1 = pf_read_env_page(curenv, (void *)fault_va);
						bool exisits_in_user_heap=0;
						bool exisits_in_user_stack=0;
						if(fault_va>=USTACKBOTTOM && fault_va<USTACKTOP){
							exisits_in_user_stack=1;
						}
						if(fault_va>=USER_HEAP_START && fault_va<USER_HEAP_MAX){
							exisits_in_user_heap=1;
						}
						if (ret1 == E_PAGE_NOT_EXIST_IN_PF)
						{
							if (exisits_in_user_heap == 0 && exisits_in_user_stack == 0)
							{
									sched_kill_env(curenv->env_id);
							}
						}
				// Remove the first entered element in ActiveList (FIFO concept)
				struct WorkingSetElement *first_entered_element = LIST_LAST(&(curenv->ActiveList));
				LIST_REMOVE(&(curenv->ActiveList), first_entered_element);
				pt_set_page_permissions(curenv->env_page_directory,first_entered_element->virtual_address , PERM_USER | PERM_WRITEABLE ,PERM_PRESENT);

				// Remove the LRU element in SecondList
				struct WorkingSetElement *lru_element = LIST_LAST(&(curenv->SecondList));
				uint32 *ptr_page_table = NULL;
				LIST_REMOVE(&(curenv->SecondList), lru_element);
				struct FrameInfo *modified_frame = get_frame_info(curenv->env_page_directory,lru_element->virtual_address,&ptr_page_table);
				uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, lru_element->virtual_address);
				if(page_permissions & PERM_MODIFIED){
					int ret = pf_update_env_page(curenv,lru_element->virtual_address ,modified_frame);
				}
				unmap_frame(curenv->env_page_directory,lru_element->virtual_address);
				// Put the first entered element in ActiveList at the head of SecondList
				LIST_INSERT_HEAD(&(curenv->SecondList), first_entered_element);
				// Put the faulted page at the head of the active list
				struct WorkingSetElement *new_element = env_page_ws_list_create_element(curenv, fault_va);
				pt_set_page_permissions(curenv->env_page_directory,fault_va , PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
				LIST_INSERT_HEAD(&curenv->ActiveList,new_element);
				//Senario2 LRU Replacemnt

			}
	}

	if(isPageReplacmentAlgorithmFIFO())
	{
		if(wsSize < (curenv->page_WS_max_size))
			{
				//EHAB MAHMOUD WORKING
				//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
				//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
				// Write your code here, remove the panic and write your code
				//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
				//start
				// 1- allocate, map
				uint32 perm = PERM_USER | PERM_WRITEABLE;
				struct FrameInfo *new_frame;
				allocate_frame(&new_frame);
				map_frame(curenv->env_page_directory, new_frame, fault_va, perm);
				// 2- Read the faulted page from page file to memory
				int ret1 = pf_read_env_page(curenv, (void *)fault_va);
				bool exisits_in_user_heap=0;
				bool exisits_in_user_stack=0;
				if(fault_va>=USTACKBOTTOM && fault_va<USTACKTOP){
					exisits_in_user_stack=1;
				}
				if(fault_va>=USER_HEAP_START && fault_va<USER_HEAP_MAX){
					exisits_in_user_heap=1;
				}
				if (ret1 == E_PAGE_NOT_EXIST_IN_PF)
				{
					// Step 3: If the page does not exist on page file
					// 1. If it is a stack or a heap page, then, it’s OK.
					if (exisits_in_user_heap == 0 && exisits_in_user_stack == 0)
					{
						sched_kill_env(curenv->env_id);
					}
				}
				// TODO: 4. Reflect the changes in the page working set list (i.e. add new element to list & update its last one)

				struct WorkingSetElement *new_element = env_page_ws_list_create_element(curenv, fault_va);
				LIST_INSERT_TAIL(&curenv->page_WS_list,new_element);
				if(LIST_SIZE(&curenv->page_WS_list) == 0)
						{
							curenv->page_last_WS_element = 0;
						}
						//2. if full -> make the last element point to the first in list
						if(LIST_SIZE(&curenv->page_WS_list) == curenv->page_WS_max_size)
						{
							curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
						}
				// 1. If WS list is empty -> update last element
				//refer to the project presentation and documentation for details
			}
		else{
			env_page_ws_print(curenv);
			uint32 perm = PERM_USER | PERM_WRITEABLE;
			struct FrameInfo *new_frame;
			allocate_frame(&new_frame);
			map_frame(curenv->env_page_directory, new_frame, fault_va, perm);
			// 2- Read the faulted page from page file to memory
			int ret1 = pf_read_env_page(curenv, (void *)fault_va);
			bool exisits_in_user_heap=0;
			bool exisits_in_user_stack=0;
			if(fault_va>=USTACKBOTTOM && fault_va<USTACKTOP){
				exisits_in_user_stack=1;
			}
			if(fault_va>=USER_HEAP_START && fault_va<USER_HEAP_MAX){
				exisits_in_user_heap=1;
			}
			if (ret1 == E_PAGE_NOT_EXIST_IN_PF)
			{
				// Step 3: If the page does not exist on page file
				// 1. If it is a stack or a heap page, then, it’s OK.
				if (exisits_in_user_heap == 0 && exisits_in_user_stack == 0)
				{
						sched_kill_env(curenv->env_id);
				}
			}
				struct WorkingSetElement *ws = curenv->page_last_WS_element->prev_next_info.le_next;
				struct WorkingSetElement *ws_element = curenv->page_last_WS_element;
				struct WorkingSetElement *new_element = env_page_ws_list_create_element(curenv, fault_va);
				if(ws_element == LIST_LAST(&curenv->page_WS_list)){
					LIST_REMOVE(&curenv->page_WS_list, ws_element);
					uint32 *ptr_page_table = NULL;
					struct FrameInfo *modified = get_frame_info(curenv->env_page_directory,ws_element->virtual_address,&ptr_page_table);
					uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, ws_element->virtual_address);
					if(page_permissions & PERM_MODIFIED){
						int ret = pf_update_env_page(curenv,ws_element->virtual_address,modified);
					}
					pt_set_page_permissions(curenv->env_page_directory,ws_element->virtual_address , PERM_USER | PERM_WRITEABLE ,PERM_PRESENT);
					unmap_frame(curenv->env_page_directory,ws_element->virtual_address);
					// Put the first entered element in ActiveList at the head of SecondList
					pt_set_page_permissions(curenv->env_page_directory,new_element->virtual_address , PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
					LIST_INSERT_TAIL(&curenv->page_WS_list,new_element);
					curenv->page_last_WS_element = LIST_FIRST(&curenv->page_WS_list);
					env_page_ws_print(curenv);
				}
				else{
					LIST_REMOVE(&curenv->page_WS_list,ws_element);
					uint32 *ptr_page_table = NULL;
					struct FrameInfo *modified = get_frame_info(curenv->env_page_directory,ws_element->virtual_address,&ptr_page_table);
					uint32 page_permissions = pt_get_page_permissions(curenv->env_page_directory, ws_element->virtual_address);
					if(page_permissions & PERM_MODIFIED){
						int ret = pf_update_env_page(curenv,ws_element->virtual_address ,modified);
				}
				pt_set_page_permissions(curenv->env_page_directory,ws_element->virtual_address , PERM_USER | PERM_WRITEABLE ,PERM_PRESENT);
				unmap_frame(curenv->env_page_directory,ws_element->virtual_address);
				// Put the first entered element in ActiveList at the head of SecondList
				pt_set_page_permissions(curenv->env_page_directory,new_element->virtual_address, PERM_USER | PERM_WRITEABLE | PERM_PRESENT,0);
				LIST_INSERT_BEFORE(&curenv->page_WS_list,ws,new_element);
				curenv->page_last_WS_element = ws;
				env_page_ws_print(curenv);
			}
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}


