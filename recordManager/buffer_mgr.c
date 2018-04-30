#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "assert.h"

#define RC_OPERATION_FAILED 404
//Customer Function

//search Page Frame
PageFrameLinkedNode *searchNode(BM_BufferPool *const bm, PageNumber pNum)
{
	BufferInfo *bi = bm->mgmtData;
	PageFrameLinkedNode *fm;
	//get info from bm
	fm = bi->Head;
	int count = bm->numPages;

	//traverse
	int i = 0;
	for (; i < count; i++, fm = fm->nextNode) {
		if (fm->page.pageNum == pNum) {
			return fm;
		}
	}
	if (i == count) {
		return NULL;
	}
	return NULL;
}


/*Customer Function ENDS*/

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
                  const int numPages, ReplacementStrategy strategy,
                  void *stratData)
{
    
    bm->pageFile = (char*) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
    /*bufferinfo*/
    BufferInfo* bi = (BufferInfo*) malloc(sizeof(BufferInfo));
    bm->mgmtData = bi;
	/*filehandle*/
	bi->fileHandle = (SM_FileHandle*) malloc(sizeof(SM_FileHandle));    
	bi->fileHandle->curPagePos = 0;
	bi->fileHandle->fileName = NULL;
	bi->fileHandle->totalNumPages = 0;
	bi->fileHandle->mgmtInfo = NULL;
	bi->readCount = 0;
	bi->writeCount = 0;

	bi->Head = NULL;
	bi->Tail = NULL;

	bi->tail = NULL;
	bi->head = NULL;
    
    if(openPageFile((char*)pageFileName, bi->fileHandle) != RC_OK){
        return RC_OPERATION_FAILED;
    }
    
    int i = 0;
    PageFrameLinkedNode *fm;
    
    while (i<numPages) {
        MyQueue* nname = (MyQueue*)malloc(sizeof(MyQueue));

        fm = (PageFrameLinkedNode*) malloc(sizeof(PageFrameLinkedNode));
        fm->page.data  = (char*) calloc(PAGE_SIZE, sizeof(char));
		/*init page frame*/
		(fm->page).pageNum = -1;

		fm->fixCount = 0;
		fm->isDirty = 0;
		fm->nextNode = NULL;
        
        //Insert Node into QUEUE and Bufferinfo

        nname->nextNode = NULL;
        nname->prevNode = NULL;
        nname->current = NULL;
        nname->current = fm;
        if (bi->head == NULL) {
            bi->head = nname;
            bi->tail = nname;
        }
        
        else {
            bi->tail->nextNode = nname;
            bi->tail = nname;
        }
        /*bufferinfo*/
        if (bi->Head == NULL)
        {
            //empty QUEUE
            bi->Head = fm;
            bi->Tail = fm;
        }
        
        else {
            (bi->Tail)->nextNode = fm;
            bi->Tail = fm;
        }
        i++;
    }
    
    return RC_OK;
}

RC shutdownBufferPool(BM_BufferPool *const bm)
{
    if (bm==NULL) {
        return RC_OPERATION_FAILED;
    }
    
    BufferInfo *mgmtData = bm->mgmtData;
    
    
	PageFrameLinkedNode *prev = NULL;
	PageFrameLinkedNode *fm = mgmtData->Head;
    
    int i =0;
    while (i<bm->numPages) {
        if(fm->fixCount != 0){            
            return RC_OPERATION_FAILED;
        }
        fm = fm->nextNode;
        i++;
    }
    //Force flush the buffer pool
    forceFlushPool(bm);
    
    i =0;
    fm =  mgmtData->Head;
    while (i<bm->numPages) {
        
        free(fm->page.data);
        prev = fm;
        if (fm->nextNode!= NULL) fm = fm->nextNode;
        free(prev);
        i++;
    }
    
    free(mgmtData->fileHandle);
    
	MyQueue *que = NULL;
	MyQueue *node = mgmtData->head;
    i =0;
    while (i<bm->numPages) {
        
        que = node->nextNode;
        
        free(node);
        
        node = que;
        i++;
    }
    
    //End Memory Freeing

    free(mgmtData);
    
    return RC_OK;
}

//Force flushes the pool
RC forceFlushPool(BM_BufferPool *const bm)
{
    if (bm==NULL) {
        return RC_OPERATION_FAILED;
    }
    
    BufferInfo *mgmtData = bm->mgmtData;   
    
    PageFrameLinkedNode *fm =  mgmtData->Head;
    
    int i = 0;
    while (i<bm->numPages) {
        if(fm->isDirty & (fm->fixCount == 0)){
            RC state = writeBlock(fm->page.pageNum, mgmtData->fileHandle, (SM_PageHandle) fm->page.data);
            
            if (state != RC_OK) return RC_OPERATION_FAILED;
            
            fm->isDirty = FALSE;
            mgmtData->writeCount++;
        }
        fm = fm->nextNode;
        i++;
    }  
    return RC_OK;    
}


// Buffer Manager Interface Access Pages
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{    
    PageFrameLinkedNode *fm = searchNode(bm,page->pageNum);
    
    //founded
    if(fm != NULL)
    {	//mark
        fm->isDirty = true;
        
        return RC_OK;
    }
    
    return RC_OPERATION_FAILED;
    
}

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{    
    
    PageFrameLinkedNode *fm = searchNode(bm,page->pageNum);

    //If found the frame in buffer
    if(fm != NULL)
    {
        assert(fm->fixCount>0);
        
        fm->fixCount--;
        
        return RC_OK;
    }
    
    
    return RC_OPERATION_FAILED;
    
}

RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{

    BufferInfo *bi = bm->mgmtData;
    
    PageFrameLinkedNode *fm = searchNode(bm,page->pageNum);
    
    //founded
    if(fm != NULL)
    {
		//force to disk
        if(fm->isDirty && fm->fixCount==0){
            writeBlock(fm->page.pageNum, bi->fileHandle, (SM_PageHandle) fm->page.data);
            fm->isDirty = FALSE;
			bi->writeCount++;
        }		        
        return RC_OK;
    }   
    return RC_OPERATION_FAILED;
}

PageFrameLinkedNode *getReplaceableFrame(BM_BufferPool* bm){
    
    PageFrameLinkedNode* fm = NULL;
    BufferInfo* bi = bm->mgmtData;

    if (bm->strategy == RS_LRU) {
        
        fm = bi->head->current;
        
        if (fm->fixCount == 0) {
            //Update the Queue, insert/mv the frame to the tail of QUEUE

            BufferInfo *bi = bm->mgmtData;
            
            MyQueue *que = bi->head;
            MyQueue *prev = NULL;
            RC result;
            int i = 0;
            while (i < bm->numPages) {
                //move the node to the end of QUEUE
                if (que->current == fm) {
                    //RC result = reQueue(que, prev, bm);
                    if (bi->tail == que) {
                        //has been last node
                        result = RC_OK;
                    }
                    else if ((bi->head == que && bi->tail == que)) {
                        //only one node, no need to move
                        result = RC_OK;
                    }
                    
                    else if (bi->head == que && prev == NULL) {
                        bi->head = que->nextNode;
                        bi->tail->nextNode = que;
                        que->nextNode = NULL;
                        bi->tail = que;
                        
                        result = RC_OK;
                    }else{
                        
                        prev->nextNode = que->nextNode;
                        bi->tail->nextNode = que;
                        que->nextNode = NULL;
                        bi->tail = que;
                        result = RC_OK;
                    }
                    
                    if (result != RC_OK) return NULL;
                    break;
                }
                
                
                prev = que;
                que = que->nextNode;
                i++;
            }
            return fm;
        }
        
    }
    
    else if(bm->strategy == RS_FIFO){
        
        fm = bi->head->current;
        
        int i = 0;
        while (i<bm->numPages) {
            
            if (fm->fixCount == 0) {

                BufferInfo *bi = bm->mgmtData;
                
                MyQueue *que = bi->head;
                MyQueue *prev = NULL;
                RC result;
                int i = 0;
                while (i < bm->numPages) {
                    //move the node to the end of QUEUE
                    if (que->current == fm) {
                        //RC result = reQueue(que, prev, bm);
                        if (bi->tail == que) {
                            //has been last node
                            result = RC_OK;
                        }
                        else if ((bi->head == que && bi->tail == que)) {
                            //only one node, no need to move
                            result = RC_OK;
                        }
                        
                        else if (bi->head == que && prev == NULL) {
                            bi->head = que->nextNode;
                            bi->tail->nextNode = que;
                            que->nextNode = NULL;
                            bi->tail = que;
                            
                            result = RC_OK;
                        }else{
                            
                            prev->nextNode = que->nextNode;
                            bi->tail->nextNode = que;
                            que->nextNode = NULL;
                            bi->tail = que;
                            result = RC_OK;
                        }
                        
                        if (result != RC_OK) return NULL;
                        break;
                    }
                    
                    
                    prev = que;
                    que = que->nextNode;
                    i++;
                }
                return fm;
            }            
            fm = fm->nextNode;
            i++;
        }        
    }    
    return NULL;
}

RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page,
            const PageNumber pageNum)
{
    BufferInfo *bi = bm->mgmtData;    
    PageFrameLinkedNode *fm = searchNode(bm,pageNum);
    
    if(fm != NULL)
    {
        page->pageNum = fm->page.pageNum;
        page->data = fm->page.data;
        fm->fixCount++;

        BufferInfo *bi = bm->mgmtData;
        
        MyQueue *que = bi->head;
        MyQueue *prev = NULL;
        RC result;
        int i = 0;
        while (i < bm->numPages) {
            //move the node to the end of QUEUE
            if (que->current == fm) {
                //RC result = reQueue(que, prev, bm);
                if (bi->tail == que) {
                    //has been last node
                    result = RC_OK;
                }
                else if ((bi->head == que && bi->tail == que)) {
                    //only one node, no need to move
                    result = RC_OK;
                }
                
                else if (bi->head == que && prev == NULL) {
                    bi->head = que->nextNode;
                    bi->tail->nextNode = que;
                    que->nextNode = NULL;
                    bi->tail = que;
                    
                    result = RC_OK;
                }else{
                    
                    prev->nextNode = que->nextNode;
                    bi->tail->nextNode = que;
                    que->nextNode = NULL;
                    bi->tail = que;
                    result = RC_OK;
                }
                
                if (result != RC_OK) return RC_OPERATION_FAILED;
                break;
            }
            
            
            prev = que;
            que = que->nextNode;
            i++;
        }
        return RC_OK;        
    }
    
    //flush to disk    
    fm = getReplaceableFrame(bm);
    
    if (fm==NULL){
        
        return RC_OPERATION_FAILED;
    }    
    forcePage(bm, &fm->page);
    
    if (pageNum >= bi->fileHandle->totalNumPages) {
		//not found
        fm->fixCount++;
        fm->page.pageNum = pageNum;
        page->data = fm->page.data;
        page->pageNum = pageNum;        
        return RC_OK;
    }
    
    RC result = readBlock(pageNum, bi->fileHandle, fm->page.data);
    if (result != RC_OK) {
        result = RC_OPERATION_FAILED;
    }
    if (result == RC_OPERATION_FAILED) {
    }
    
    else if (result != RC_OK) {
        return result;
    }
    
	bi->readCount++;
    fm->fixCount++;
    fm->page.pageNum = pageNum;
    page->data = fm->page.data;
    page->pageNum = pageNum;       
    return RC_OK;       
}

// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm)
{

    BufferInfo *bi = bm->mgmtData;
    
    PageFrameLinkedNode *node = bi->Head;
    
	int count = bm->numPages;
	PageNumber* store = malloc(sizeof(bool)*count);
	for (int i = 0; i < count; i++) {
		if (node != NULL)
			store[i] = node->page.pageNum;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return store;
}


bool *getDirtyFlags(BM_BufferPool *const bm)
{

	BufferInfo *bi = bm->mgmtData;

	PageFrameLinkedNode *node = bi->Head;
	int count = bm->numPages;
	bool* store = malloc(sizeof(bool)*count);
	//traverse
	for (int i = 0; i < count; i++) {
		if (node != NULL && node->isDirty)
			store[i] = 1;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return store;
}
int *getFixCounts (BM_BufferPool *const bm)
{
    
    BufferInfo *bi = bm->mgmtData;
    
    PageFrameLinkedNode *node = bi->Head;
	int count = bm->numPages;
    PageNumber* store = malloc(sizeof(int)*count);
    
	for (int i = 0; i < count; i++) {
		if (node != NULL && node->fixCount)
			store[i] = node->fixCount;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}  
    
    return store;

}

int getNumReadIO (BM_BufferPool *const bm)
{
    return ((BufferInfo *)bm->mgmtData)->readCount;
}

int getNumWriteIO (BM_BufferPool *const bm)
{
    return ((BufferInfo *)bm->mgmtData)->writeCount;
}

