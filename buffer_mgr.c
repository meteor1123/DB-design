#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "strategy.h"
#include "assert.h"

/* Buffer Manager Interface Pool Handling
*/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData)
{
    
    bm->pageFile = (char*) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    
    Manager* manager = (Manager*) malloc(sizeof(Manager));
    bm->manager = manager;
    manager->fileHandle = (SM_FileHandle*) malloc(sizeof(SM_FileHandle));
    
    init_BPmanager(manager);
    
    if(openPageFile((char*)pageFileName, manager->fileHandle) != RC_OK){
        return RC_FILE_NOT_FOUND;
    }
    
    int i = 0;
    PagePointer *pointer;
    
    //Initializes each pointer and data
    while (i<numPages) {
        
        pointer = (PagePointer*) malloc(sizeof(PagePointer));
        pointer->page.data  = (char*) calloc(PAGE_SIZE, sizeof(char));
        init_PagePointer(pointer);
        insert_PageFrame(pointer, bm);
        i++;
    }
    
    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm)
{
    if (bm==NULL) {
        return RC_FILE_NOT_FOUND;
    }
    
    Manager *manager = bm->manager;
    THREAD_LOCK();
    
    PagePointer *prevFrame = NULL, *pointer =  manager->Head;
    
    int i =0;
    while (i<bm->numPages) {
        if(pointer->fix_Count != 0){
            THREAD_UNLOCK();
            return RC_FILE_NOT_FOUND;
        }
        pointer = pointer->nextNode;
        i++;
    }
    
    forceFlushPool(bm);    
    i =0;
    pointer =  manager->Head;

    while (i<bm->numPages) {
        
        free(pointer->page.data);
        prevFrame = pointer;
        if (pointer->nextNode!= NULL) pointer = pointer->nextNode;
        free(prevFrame);
        i++;
    }
    //free
    free(manager->fileHandle);
    
    LRUNode *LRUNext = NULL, *LRUFrame = manager->LRU_Head;
    FIFONode *FIFONext = NULL, *FIFOFrame = manager->FIFO_Head;
    i =0;

    while (i<bm->numPages) {
        
        LRUNext = LRUFrame->nextNode;
        FIFONext = FIFOFrame->nextNode;
        
        free(LRUFrame);
        free(FIFOFrame);
        
        LRUFrame = LRUNext;
        FIFOFrame = FIFONext;
        i++;
    }
        
    THREAD_UNLOCK();
    free(manager);
    manager=NULL;
    
    return RC_OK;
}

//Force flushes the pool
RC forceFlushPool(BM_BufferPool *const bm)
{
    if (bm!=NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();

		PagePointer *pointer = manager->Head;

		int i = 0;
		while (i<bm->numPages) {
			if (pointer->isDirty & (pointer->fix_Count == 0)) {
				RC state = writeBlock(pointer->page.pageNum, manager->fileHandle, (SM_PageHandle)pointer->page.data);

				if (state != RC_OK) return state;

				pointer->isDirty = FALSE;
				manager->writeCount++;
			}
			pointer = pointer->nextNode;
			i++;
		}

		THREAD_UNLOCK();

		return RC_OK;
    }
	else
		return RC_FILE_NOT_FOUND;
    
    
    
}


/* Buffer Manager Interface Access Pages
*/
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if (bm != NULL && page != NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = getPagePointer(bm, page->pageNum);

		//If found the pointer in buffer
		if (pointer != NULL)
		{
			pointer->isDirty = true;
			THREAD_UNLOCK();
			return RC_OK;
		}
		THREAD_UNLOCK();
		return RC_OK;
	}
	else
		return RC_FILE_NOT_FOUND;
    
    
}

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if (bm != NULL && page != NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = getPagePointer(bm, page->pageNum);

		//If found the pointer in buffer
		if (pointer != NULL)
		{
			assert(pointer->fix_Count > 0);

			pointer->fix_Count--;
			THREAD_UNLOCK();
			return RC_OK;
		}

		THREAD_UNLOCK();
		return RC_OK;
	}
	else
		return RC_FILE_NOT_FOUND;
    
    
    
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
	if (bm != NULL && page != NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = getPagePointer(bm, page->pageNum);

		//If found the pointer in buffer
		if (pointer != NULL)
		{
			if (pointer->isDirty && pointer->fix_Count == 0) {
				writeBlock(pointer->page.pageNum, manager->fileHandle, (SM_PageHandle)pointer->page.data);
				pointer->isDirty = FALSE;
				manager->writeCount++;

			}

			THREAD_UNLOCK();
			return RC_OK;
		}

		THREAD_UNLOCK();
		return RC_OK;
	}
	else
		return RC_FILE_NOT_FOUND;
    
    

}

PagePointer *getReplaceableFrame(BM_BufferPool* bm){
    
    PagePointer* pointer = NULL;
    Manager* manager = bm->manager;
    
    if (bm->strategy == RS_LRU) {
        
        pointer = manager->LRU_Head->pointing;
        
        if (pointer->fix_Count == 0) {
            updateLRU(pointer, bm);
            updateFIFO(pointer, bm);
            return pointer;
        }
        
    }
    
    else if(bm->strategy == RS_FIFO){
        
        pointer = manager->FIFO_Head->pointing;
        
        int i = 0;
        while (i<bm->numPages) {
            
            if (pointer->fix_Count == 0) {
                updateLRU(pointer, bm);
                updateFIFO(pointer, bm);
                return pointer;
            }
            
            pointer = pointer->nextNode;
            i++;
        }
        
    }
    
    return NULL;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum)
{

    if (bm==NULL || page == NULL) {
        return RC_FILE_NOT_FOUND;
    }
        
    Manager *manager = bm->manager;
    THREAD_LOCK();
    PagePointer *pointer = getPagePointer(bm,pageNum);
    
    if(pointer != NULL)
    {
        page->pageNum = pointer->page.pageNum;
        page->data = pointer->page.data;
        pointer->fix_Count++;
        updateLRU(pointer, bm);
        
        THREAD_UNLOCK();
        return RC_OK;
        
    }
    
    pointer = getReplaceableFrame(bm);    
    if (pointer==NULL){
        THREAD_UNLOCK();
        return RC_FILE_NOT_FOUND;
    }    
    forcePage(bm, &pointer->page);
    if (pageNum >= manager->fileHandle->totalNumPages) {
        pointer->fix_Count++;
        pointer->page.pageNum = pageNum;
        page->data = pointer->page.data;
        page->pageNum = pageNum;
        
        THREAD_UNLOCK();
        return RC_OK;
    }    
    RC status = readBlock(pageNum, manager->fileHandle, pointer->page.data);    
    if (status == RC_READ_NON_EXISTING_PAGE) {}    
    else if (status!=RC_OK) {
        THREAD_UNLOCK();
        return status;
    }    
    manager->readCount++;
    pointer->fix_Count++;
    pointer->page.pageNum = pageNum;
    page->data = pointer->page.data;
    page->pageNum = pageNum;
    
    THREAD_UNLOCK();
    return RC_OK;    
    
}
/* Statistics Interface
*/
PageNumber *getPagePointerContents (BM_BufferPool *const bm)
{
	if (bm != NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = manager->Head;

		PageNumber* pageNumArray = malloc(sizeof(int)*bm->numPages);

		int i = 0;
		while (i < bm->numPages) {

			pageNumArray[i] = pointer->page.pageNum;

			pointer = pointer->nextNode;
			i++;
		}

		THREAD_UNLOCK();
		return pageNumArray;
	}
	else
		return NULL;   
}

int *getFixCounts (BM_BufferPool *const bm)
{
	if (bm != NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = manager->Head;

		PageNumber* fixCountsArray = malloc(sizeof(int)*bm->numPages);

		int i = 0;
		while (i < bm->numPages) {

			fixCountsArray[i] = pointer->fix_Count;

			pointer = pointer->nextNode;
			i++;
		}

		THREAD_UNLOCK();
		return fixCountsArray;
	}
	else
		return 0; 
}

int getNumReadIO (BM_BufferPool *const bm)
{
	if (bm != NULL) {
		Manager* manager = bm->manager;
		return manager->readCount;
	}
	else
		return 0;
}

int getNumWriteIO (BM_BufferPool *const bm)
{
    if (bm!=NULL) {
		Manager* manager = bm->manager;
		return manager->writeCount;
    }
	else
		return 0;    
}

bool *getDirtyFlags (BM_BufferPool *const bm)
{
    if (bm!=NULL) {
		Manager *manager = bm->manager;
		THREAD_LOCK();
		PagePointer *pointer = manager->Head;
		bool* dirtyFlagsArray = malloc(sizeof(bool)*bm->numPages);
		int i = 0;
		while (i<bm->numPages) {

			dirtyFlagsArray[i] = pointer->isDirty;

			pointer = pointer->nextNode;
			i++;
		}
		THREAD_UNLOCK();
		return dirtyFlagsArray;        
    } 
	else
		return false;;
}
