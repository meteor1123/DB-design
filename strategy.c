#include <stdlib.h>
#include "strategy.h"
#include "dberror.h"

void init_PagePointer(PagePointer *pointer) {

	(pointer->page).pageNum = NO_PAGE;

	pointer->fix_Count = 0;
	pointer->isDirty = false;
	pointer->nextNode = NULL;

}

void init_FileHandle(SM_FileHandle* fileHandle) {

	fileHandle->curPagePos = 0;
	fileHandle->fileName = NULL;
	fileHandle->totalNumPages = 0;
	fileHandle->mgmtInfo = NULL;

}

void init_BPmanager(Manager *manager) {

	manager->readCount = 0;
	manager->writeCount = 0;

	manager->Head = NULL;
	manager->Tail = NULL;

	manager->LRU_Tail = NULL;
	manager->LRU_Head = NULL;

	manager->FIFO_Tail = NULL;
	manager->FIFO_Head = NULL;

	init_FileHandle(manager->fileHandle);

}


PagePointer *getPagePointer(BM_BufferPool *const bm, PageNumber pNum)
{
	Manager *manager = bm->manager;
	PagePointer *pointer = manager->Head;

	int i = 0;

	while (i<bm->numPages) {

		if (pointer->page.pageNum == pNum) {
			return pointer;
		}

		pointer = pointer->nextNode;
		i++;
	}

	return NULL;
}

LRUNode* makeLRUNode(){
    
    LRUNode* node = (LRUNode*)malloc(sizeof(LRUNode));
    node->nextNode = NULL;
    node->pointing = NULL;
    
    return node;
    
}

FIFONode* makeFIFONode(){
    
    FIFONode* node = (FIFONode*)malloc(sizeof(FIFONode));
    node->nextNode = NULL;
    node->pointing = NULL;
    
    return node;
    
}

//Insert PagePointer into BufferPool Following specific strategy
RC insert_PageFrame(PagePointer *pointer, BM_BufferPool *const pool){
    
    if (pool == NULL || pointer == NULL) {
        return RC_FILE_NOT_FOUND;
    }
    
    Manager* manager = (Manager*) pool->manager;
    
    if(manager->Head == NULL)
    {
        manager->Head = pointer;
        manager->Tail = pointer;
    }
    
    else{
        (manager->Tail)->nextNode = pointer;
        manager->Tail = pointer;
    }    
    /*				LRU						*/
    LRUNode *LRU_Node = makeLRUNode();
    LRU_Node->pointing = pointer;
    
    
    if (manager->LRU_Head == NULL) {
        manager->LRU_Head = LRU_Node;
        manager->LRU_Tail = LRU_Node;
    }
    
    else{
        manager->LRU_Tail->nextNode = LRU_Node;
        manager->LRU_Tail = LRU_Node;
    }
    
    /*				FIFO						*/
    FIFONode *FIFO_Node = makeFIFONode();
    FIFO_Node->pointing = pointer;
    
    
    if (manager->FIFO_Head == NULL) {
        manager->FIFO_Head = FIFO_Node;
        manager->FIFO_Tail = FIFO_Node;
    }
    
    else{
        manager->FIFO_Tail->nextNode = FIFO_Node;
        manager->FIFO_Tail = FIFO_Node;
    }
    return RC_OK;
    
}

RC moveToEndLRU(LRUNode* pointer, LRUNode* frPrev, Manager *const manager){
    
    if(manager->LRU_Head == NULL){
        return RC_LFR_ERROR;
    }
    
    if ((manager->LRU_Head == pointer && manager->LRU_Tail == pointer) || (manager->LRU_Tail == pointer)) {
        return RC_OK;
    }
    
    if (manager->LRU_Head == pointer && frPrev == NULL) {
        manager->LRU_Head = pointer->nextNode;
        manager->LRU_Tail->nextNode = pointer;
        pointer->nextNode = NULL;
        manager->LRU_Tail = pointer;
        
        return RC_OK;
    }
    
    frPrev->nextNode = pointer->nextNode;
    manager->LRU_Tail->nextNode = pointer;
    pointer->nextNode = NULL;
    manager->LRU_Tail = pointer;
    return RC_OK;
    
}

//Moves the pointer to end (FIFO)
RC moveToEndFIFO(FIFONode* pointer, FIFONode* frPrev, Manager *const manager){
    
    if(manager->FIFO_Head == NULL){
        return RC_FIFO_ERROR;
    }
    
    if ((manager->FIFO_Head == pointer && manager->FIFO_Tail == pointer) || (manager->FIFO_Tail == pointer)) {
        return RC_OK;
    }
    
    if (manager->FIFO_Head == pointer && frPrev == NULL) {
        manager->FIFO_Head = pointer->nextNode;
        manager->FIFO_Tail->nextNode = pointer;
        pointer->nextNode = NULL;
        manager->FIFO_Tail = pointer;
        
        return RC_OK;
    }
    
    frPrev->nextNode = pointer->nextNode;
    manager->FIFO_Tail->nextNode = pointer;
    pointer->nextNode = NULL;
    manager->FIFO_Tail = pointer;
    return RC_OK;
    
}

//Update the passed pointer as Most recenlty used pointer
RC updateLRU(PagePointer* pointer, BM_BufferPool *const bm){
    
    Manager *manager = bm->manager;
    
    LRUNode *fr =manager->LRU_Head;
    LRUNode *frPrev = NULL;
    
    int i=0;
    while (i < bm->numPages) {
        
        if (fr->pointing == pointer) {
            RC status = moveToEndLRU(fr,frPrev,manager);
            if (status != RC_OK) return RC_LFR_ERROR;
            break;
        }
        
        frPrev = fr;
        fr = fr->nextNode;
        i++;
    }
    
    return RC_OK;
}

//Update the pointer as Most recently fetched pointer
RC updateFIFO(PagePointer* pointer, BM_BufferPool *const bm){
    
    Manager *manager = bm->manager;
    
    FIFONode *fr =manager->FIFO_Head;
    FIFONode *frPrev = NULL;
    
    int i=0;
    while (i < bm->numPages) {
        
        if (fr->pointing == pointer) {
            RC status = moveToEndFIFO(fr,frPrev,manager);
            if (status != RC_OK) return RC_FIFO_ERROR;
            break;
        }
        
        frPrev = fr;
        fr = fr->nextNode;
        i++;
    }
    
    return RC_OK;
}


