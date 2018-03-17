#include "storage_mgr.h"
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "dberror.h"
#define I386_PGBYTES 4096

//////////////////
/* basic method */

//check filehandle
RC handleCheck(SM_FileHandle *fileHandle)
{
	if (fileHandle == NULL || fileHandle->mgmtInfo == NULL) {
		return RC_FILE_HANDLE_NOT_INIT;
	}
	else
		return RC_OK;
}

//read data; basic method
RC readMethod(int pageNum, SM_FileHandle *fileHandle, SM_PageHandle memPage) {

	//Should we reauthorize?
	if (handleCheck(fileHandle) != RC_OK) return RC_FILE_HANDLE_NOT_INIT;

	FILE* filePtr = fileHandle->mgmtInfo;

	//checks for invalid pages
	if (pageNum >= fileHandle->totalNumPages || pageNum<0)
		return RC_READ_NON_EXISTING_PAGE;

	//seeks the start position of data
	if (fseek(filePtr, pageNum*PAGE_SIZE, SEEK_SET) != 0)
		return RC_IM_NO_MORE_ENTRIES;

	fileHandle->curPagePos = pageNum;

	//reads the data and verifies if the read is successful
	if (fread(memPage, PAGE_SIZE, 1, filePtr) != 1)
		return RC_IM_NO_MORE_ENTRIES;

	return RC_OK;
}

//write data; basic method
RC writeMethod(int pageNum, SM_FileHandle *fileHandle, SM_PageHandle memPage) {

	//Should we reauthorize?
	if (handleCheck(fileHandle) != RC_OK) return RC_FILE_HANDLE_NOT_INIT;

	FILE* filePtr = fileHandle->mgmtInfo;

	//checks for invalid pages
	if (pageNum >= fileHandle->totalNumPages)
		ensureCapacity(pageNum, fileHandle);
	//return RC_WRITE_NON_EXISTING_PAGE;

	//seeks the start position of data
	if (fseek(filePtr, pageNum*PAGE_SIZE, SEEK_SET) != 0)
		return RC_IM_NO_MORE_ENTRIES;

	fileHandle->curPagePos = pageNum;

	//writes the data and verifies if the write is successful
	if (fwrite(memPage, PAGE_SIZE, 1, filePtr) != 1)
		return RC_WRITE_FAILED;

	return RC_OK;

}

/////////////////////////////
/* manipulating page files */
RC createPageFile(char *fileName) {

	FILE *newPtr = fopen(fileName, "w+");

	if (newPtr == NULL)
		return RC_WRITE_FAILED;

	//create null page
	char *pg = (char*)calloc(PAGE_SIZE, sizeof(char));
	memset(pg, '\0', PAGE_SIZE);

	int flag = (int)fwrite(pg, PAGE_SIZE, 1, newPtr);
	free(pg);

	if (flag != 1)
	{
		fclose(newPtr);
		return RC_RM_NO_MORE_TUPLES;
	}

	return RC_OK;

}

RC openPageFile(char *fileName, SM_FileHandle *fHandle) {

	FILE *filePtr = fopen(fileName, "r+");

	if (filePtr == NULL)
		return RC_FILE_NOT_FOUND;

	if (fseek(filePtr, 0, SEEK_END) != 0)
		return RC_IM_NO_MORE_ENTRIES;

	long size = ftell(filePtr);
	rewind(filePtr);

	handleCheck(fHandle);

	fHandle->fileName = fileName;
	fHandle->curPagePos = 0;
	fHandle->totalNumPages = (int)size / PAGE_SIZE;
	fHandle->mgmtInfo = filePtr;

	return RC_OK;
}


RC closePageFile(SM_FileHandle *fHandle) {

	handleCheck(fHandle);

	if (fclose(fHandle->mgmtInfo) == 0)
		return RC_OK;
	else
		return RC_OPERATATION_FAILED;
}


RC destroyPageFile(char *fileName) {

	if (remove(fileName))
	{
		return RC_FILE_NOT_FOUND;
	}
	else
		return RC_OK;
}

//////////////////////////////
/* reading blocks from disc */
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(pageNum, fHandle, memPage);
}


int getBlockPos(SM_FileHandle *fHandle) {

	handleCheck(fHandle);
	return fHandle->curPagePos;

}


RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(0, fHandle, memPage);
}


RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(fHandle->curPagePos - 1, fHandle, memPage);
}


RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(fHandle->curPagePos, fHandle, memPage);
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(fHandle->curPagePos + 1, fHandle, memPage);
}


RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return readMethod(fHandle->totalNumPages - 1, fHandle, memPage);
}


///////////////////////////////////
/* writing blocks to a page file */
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return writeMethod(pageNum, fHandle, memPage);
}


RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	handleCheck(fHandle);
	return writeMethod(fHandle->curPagePos, fHandle, memPage);
}


RC appendEmptyBlock(SM_FileHandle *fHandle) {

	handleCheck(fHandle);
	FILE* filePtr = fHandle->mgmtInfo;
	if (fseek(filePtr, 0, SEEK_END) != 0)
	{
		return RC_IM_NO_MORE_ENTRIES;
	}

	char *pg = (char*)calloc(PAGE_SIZE, sizeof(char));
	memset(pg, 0, PAGE_SIZE);

	if (fwrite(pg, PAGE_SIZE, 1, filePtr) != 1) {
		free(pg);
		return RC_WRITE_FAILED;
	}
	free(pg);

	//modified
	fHandle->totalNumPages++;
	fHandle->curPagePos = (int)ftell(filePtr) / PAGE_SIZE;

	return RC_OK;

}


RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {

	handleCheck(fHandle);

	if (fHandle->totalNumPages<numberOfPages) {

		int addedPages;
		addedPages = numberOfPages - fHandle->totalNumPages;

		int i;
		for (i = 0; i<addedPages; i++) {
			appendEmptyBlock(fHandle);
		}
	}

	return RC_OK;
}

void initStorageManager(void) {
	char *pg = (char*)calloc(PAGE_SIZE, sizeof(char));
	memset(pg, '\0', PAGE_SIZE);
	free(pg);

}
