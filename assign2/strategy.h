#ifndef strategy_h
#define strategy_h

#include "dt.h"
#include <pthread.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"

typedef struct PagePointer {

	BM_PageHandle page;
	int fix_Count;
	bool isDirty;

	struct PagePointer *nextNode;

}PagePointer;

typedef struct LRUNode{
    
    PagePointer* pointing;
    struct LRUNode *nextNode;
    
}LRUNode;

typedef struct FIFONode{
    
    PagePointer* pointing;
    struct FIFONode *nextNode;
    
}FIFONode;

typedef struct Manager{

	pthread_mutex_t bm_Mutex;
	LRUNode *LRU_Head;
	LRUNode *LRU_Tail;

	FIFONode *FIFO_Head;
	FIFONode *FIFO_Tail;
    
    SM_FileHandle *fileHandle;
    PagePointer *Head;
    PagePointer *Tail;
    
    int readCount;
    int writeCount;
    
}Manager;


void init_PagePointer(PagePointer *pointer);

void init_BPmanager(Manager *manager);

PagePointer *getPagePointer(BM_BufferPool *const bm, PageNumber pNum);

RC updateLRU(PagePointer *pointer, BM_BufferPool *const bm);

RC updateFIFO(PagePointer* pointer, BM_BufferPool *const bm);

RC insert_PageFrame(PagePointer *pointer, BM_BufferPool *const pool);

#endif
