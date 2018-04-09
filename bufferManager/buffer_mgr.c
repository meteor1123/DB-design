#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "buffer_mgr.h"
#include "dberror.h"
#include "storage_mgr.h"


//function
int traverse_list(PageFrameLinkedNode *head) {
	int count;
	while (head != NULL) {
		head = head->nextNode;
		count++;
	}
	return count;
}

RC remove_at_head(bufferInfo *bufferinfo) {
	PageFrameLinkedNode *head = (PageFrameLinkedNode *) malloc(sizeof(PageFrameLinkedNode));
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	head = bufferinfo->head;
	node = head;

	RC result;
	if (head == NULL || head->pinCount !=0 ) return RC_OPERATION_FAILED;
	//if dirty, write to disk
	if (head->isDirty != 0) {
		result = writeBlock(head->page->pageNum, bufferinfo->fileHandle,
			head->page->data);

		if (result != RC_OK)
			return RC_OPERATION_FAILED;
		bufferinfo->writeCount++;
	}

	if (head->nextNode != NULL) {
		head = head->nextNode;
		head->prevNode = NULL;
	}
	else {
		head = NULL;
	}
	free(node);
	bufferinfo->head = head;
	return RC_OK;
}

RC remove_at_current(bufferInfo *bufferinfo) {
	PageFrameLinkedNode *prev = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	node = bufferinfo->current;
	if (node == NULL) {
		return RC_OPERATION_FAILED;
	}
	RC result;
	//if dirty, write to disk
	if (node->isDirty != 0) {
		result = writeBlock(node->page->pageNum, bufferinfo->fileHandle,
			node->page->data);

		if (result != RC_OK)
			return RC_OPERATION_FAILED;
		bufferinfo->writeCount++;

	}

	if (node->prevNode == NULL) {	//head node
		return remove_at_head(bufferinfo);
	}
	if (node->nextNode == NULL) {	//tail node
		node = NULL;
	}
	// not head, not tail
	prev = node->prevNode;
	prev->nextNode = node->nextNode;
	prev->nextNode->prevNode = prev;
	free(node);
	return RC_OK;
}

RC add_node(bufferInfo *bufferinfo, PageFrameLinkedNode *newNode) {
	PageFrameLinkedNode *head = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	head = bufferinfo->head;
	node = head;

	int count = traverse_list(head);
	if (count <= bufferinfo->pageCount) {
		while (node->nextNode == NULL) {
			node = node->nextNode;
		}
		node->nextNode = newNode;
		newNode->prevNode = node;
		newNode->nextNode = NULL;
		return RC_OK;
	}
	else
	//	queue is full
	{
		if (bufferinfo->strategy == 0) {
			return remove_at_head(bufferinfo);
		}
		if (bufferinfo->strategy == 1) {
			return remove_at_current(bufferinfo);
		}
		return RC_OPERATION_FAILED;
	}

}

void init_pageNode(PageFrameLinkedNode *newNode,int strategy) {
	newNode->page->pageNum = NO_PAGE;
	newNode->isDirty = 0;
	newNode->pinCount = 0;
	newNode->strategy = strategy;
	newNode->prevNode = NULL;
	newNode->nextNode = NULL;
	newNode->page->data = (char *)malloc(PAGE_SIZE * sizeof(char));
}
PageFrameLinkedNode *searchNode(BM_BufferPool *const bm, BM_PageHandle *const page) {
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = buffinfo->head;

	for (int i = 0; i < bm->numPages; i++) {
		if (node->page == page) {
			return node;
		}
		if (node->nextNode != NULL) {
			node = node->nextNode;
		}
	}
	return NULL;
}

// Buffer Manager Interface Pool Handling
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
	const int numPages, ReplacementStrategy strategy,
	void *stratData) {
	//check
	if (bm == NULL) {
		return RC_OPERATION_FAILED;
	}
	if (stratData == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	if (numPages <= 0) {
		return	RC_OPERATION_FAILED;
	}

	/*init part*/
	//init buffer info
	bufferInfo *bufferinfo = (bufferInfo *)malloc(sizeof(bufferInfo));
	bufferinfo->strategy = strategy;
	bufferinfo->pageCount = numPages;
	bufferinfo->readCount = 0;
	bufferinfo->writeCount = 0;

	PageFrameLinkedNode *node = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	PageFrameLinkedNode *node2 = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	PageFrameLinkedNode *head = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
	init_pageNode(node, bufferinfo->strategy);
	node = node2;
	head = node;
	for (int i = 0; i < bufferinfo->pageCount-1; i++)
	{
		PageFrameLinkedNode *newNode = (PageFrameLinkedNode *)malloc(sizeof(PageFrameLinkedNode));
		init_pageNode(newNode, bufferinfo->strategy);
		node->nextNode = newNode;
		node = node->nextNode;
		node->prevNode = node2;
		node2 = node;
	}
	node->nextNode = NULL;
	bufferinfo->head = head;
	bufferinfo->current = head;
	//init file handle
	SM_FileHandle *fileHandle = (SM_FileHandle*)malloc(sizeof(SM_FileHandle));
	RC result = openPageFile((char*)pageFileName, fileHandle);
	if (result != RC_OK) {
		return RC_OPERATION_FAILED;
	}
	

	//init buffer pool
	bm->pageFile = (char *)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->mgmtData = bufferinfo;

	return RC_OK;

}
RC shutdownBufferPool(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}

	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *head = buffinfo->head;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = head;
	//force flush
	forceFlushPool(bm);
	//check 
	//for (; node->nextNode != NULL; ) {
	//	if (node->isDirty == 1) {	//still dirty, flush failed;
	//		return RC_OPERATION_FAILED;
	//	}
	//	node = node->nextNode;
	//}

	//to end
	for (int i = 0; i< bm->numPages; i++ ) {
		if (node->isDirty == 1) {	//still dirty, flush failed;
			return RC_OPERATION_FAILED;
		}
		free(node->page->data);
		if (node->nextNode != NULL) {
			node = node->nextNode;
			free(node->prevNode);
		}
	}
	//free
	while (node != head) {

		node = node->prevNode;

		free(node->nextNode->page->data);
		free(node->nextNode);
	}
	//for (int i = 0; i< bm->numPages; i++) {
	//	free(node->page.data);
	//	node = node->prevNode;
	//	free(node->nextNode);
	//}
	/*attention*/
	free(head->page->data);
	free(head);
	free(buffinfo);

	return RC_OK;


}
RC forceFlushPool(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *head = buffinfo->head;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = head;

	//loop all page frame
	for (int i = 0; i < bm->numPages; i++) {
		if (node->isDirty == 1 && node->pinCount == 0) {
			RC result = forcePage(bm, node->page);
			if (result != RC_OK) {
				return result;
			}
			buffinfo->writeCount++;
		}
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return RC_OK;
}

// Buffer Manager Interface Access Pages
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = searchNode(bm,page);

	if (node == NULL) {
		return RC_OPERATION_FAILED;
	}

	node->isDirty = 1;
	return RC_OK;

}
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	if (page == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	//search
	node = searchNode(bm, page);
	if (node != NULL) {
		node->pinCount--;
		if (node->pinCount >= 0)
			return RC_OK;
		else
			return RC_OPERATION_FAILED;
	}
	else {	
		return RC_OPERATION_FAILED;
	}



}
//TODO:
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	if (page == NULL) {
		return RC_OPERATION_FAILED;
	}

	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	//search
	node = searchNode(bm, page);
	if (node != NULL && node->isDirty == 1) {
		RC result = openPageFile(bm->pageFile, buffinfo->fileHandle);
		if (result == RC_OK) {
			result = writeBlock(page->pageNum, buffinfo->fileHandle, page->data);
			if (result == RC_OK) {
				buffinfo->writeCount++;
				return RC_OK;
			}
		}
	}
	return RC_OPERATION_FAILED;
}
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page,
	const PageNumber pageNum) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	if (page == NULL) {
		return RC_OPERATION_FAILED;
	}
	if (pageNum <= 0) {
		return RC_OPERATION_FAILED;
	}

	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	//search
	node = searchNode(bm, page);
	if (node != NULL) {
		node->pinCount++;
		return RC_OK;
	}
	else {	//if not exsist in buffer pool
		buffinfo->readCount++;
		node->isDirty = 0;
		node->nextNode = NULL;
		node->page = page;
		node->pinCount = 1;
		node->prevNode = NULL;
		node->strategy = buffinfo->strategy;
		RC result = add_node(buffinfo, node);
		if (result != RC_OK) {
			return RC_OPERATION_FAILED;
		}
		
	}

	return RC_OK;

}

// Statistics Interface
PageNumber *getFrameContents(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = buffinfo->head;
	PageNumber store[buffinfo->pageCount];
	for (int i = 0; i < buffinfo->pageCount; i++) {
		if (node != NULL && node->page != NULL && node->page->pageNum)
			store[i] = node->page->pageNum;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return store;

}
bool *getDirtyFlags(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = buffinfo->head;

	bool store[buffinfo->pageCount];
	for (int i = 0; i < buffinfo->pageCount; i++) {
		if (node != NULL && node->isDirty )
			store[i] = 1;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return store;
}
int *getFixCounts(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	buffinfo = (bufferInfo *)bm->mgmtData;
	PageFrameLinkedNode *node = (PageFrameLinkedNode *)major(sizeof(PageFrameLinkedNode));
	node = buffinfo->head;

	int store[buffinfo->pageCount];
	for (int i = 0; i < buffinfo->pageCount; i++) {
		if (node != NULL && node->pinCount)
			store[i] = node->pinCount;
		else
			store[i] = 0;
		if (node->nextNode != NULL)
			node = node->nextNode;
	}
	return store;
}
int getNumReadIO(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	return buffinfo->readCount;
}
int getNumWriteIO(BM_BufferPool *const bm) {
	//check
	if (bm == NULL || bm->mgmtData == NULL) {
		return RC_OPERATION_FAILED;
	}
	//get info
	bufferInfo *buffinfo = (bufferInfo*)malloc(sizeof(buffinfo));
	return buffinfo->writeCount;
}