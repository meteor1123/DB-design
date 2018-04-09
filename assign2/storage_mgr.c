#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "dberror.h"
#include "storage_mgr.h"
/* manipulating page files */
extern void initStorageManager(void) {
	char *pg = (char*)calloc(PAGE_SIZE, sizeof(char));
	memset(pg, '\0', PAGE_SIZE);
	free(pg);
}
extern RC createPageFile(char *fileName) {
	FILE *fp = fopen(fileName, "wb+");

	if (fp == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	else {
		char *newPage= (char *)calloc(PAGE_SIZE, sizeof(char));
		memset(newPage, '\0', PAGE_SIZE);
		if (fwrite(newPage, sizeof(char), PAGE_SIZE, fp) ) {
			return RC_OK;
		}
		else
		{
			free(newPage);
			fclose(fp);
			return RC_WRITE_FAILED;
		}
	}
}

extern RC openPageFile(char *fileName, SM_FileHandle *fHandle) {
	FILE *fp = fopen(fileName, "r+");
	//check fp
	if (fp == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	//caculate page num
		//check file end
	if (fseek(fp, 0L, SEEK_END) != 0)
		return RC_OPERATION_FAILED;
		//find last pointer position
	long size = ftell(fp);
		//relocate
	rewind(fp);

	/*attention*/
	fHandle->fileName = fileName;
	fHandle->totalNumPages = ((int)(size)+1)/PAGE_SIZE;
	fHandle->curPagePos = 0;
	fHandle->mgmtInfo = fp;

	return RC_OK;

}

extern RC closePageFile(SM_FileHandle *fHandle) {
	if (fclose(fHandle->mgmtInfo) != 0)
		return RC_OPERATION_FAILED;
	else
		return RC_OK;
}

extern RC destroyPageFile(char *fileName) {
	if (remove(fileName) != 0)
		return RC_OPERATION_FAILED;
	else
		return RC_OK;
}

/* reading blocks from disc */
extern RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//check file if exsist
	if (fHandle == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	if (fHandle->totalNumPages <= pageNum || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	//check seek if success
	if (fseek(fHandle->mgmtInfo, sizeof(char) * pageNum * PAGE_SIZE , SEEK_SET) != 0) {
		return RC_OPERATION_FAILED;
	}
	else {
		//start to read block
		/*attention*/
		fread(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
		fHandle->curPagePos = pageNum;
		return RC_OK;
	}
}

extern int getBlockPos(SM_FileHandle *fHandle) {

	return fHandle->curPagePos;
}

extern RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return readBlock(0, fHandle, memPage);
}

extern RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return readBlock(fHandle->curPagePos - 1, fHandle, memPage);
}

extern RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return readBlock(fHandle->curPagePos, fHandle, memPage);
}

extern RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return readBlock(fHandle->curPagePos + 1, fHandle, memPage);
}

extern RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return readBlock(fHandle->totalNumPages - 1, fHandle, memPage);
}

/* writing blocks to a page file */
extern RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	//check file if exsist
	if (fHandle == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	if (fHandle->totalNumPages <= pageNum || pageNum < 0) {
		return RC_READ_NON_EXISTING_PAGE;
	}

	//check seek if success
	if (fseek(fHandle->mgmtInfo, sizeof(char) * pageNum * PAGE_SIZE, SEEK_SET) != 0) {
		return RC_OPERATION_FAILED;
	}
	else {
		//start to write block
		/*attention*/
		fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo);
		fHandle->curPagePos = pageNum;
		return RC_OK;
	}
}
extern RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {

	return writeBlock(fHandle->curPagePos, fHandle, memPage);
}
extern RC appendEmptyBlock(SM_FileHandle *fHandle) {

	//check
	if (fHandle == NULL) {
		return RC_FILE_NOT_FOUND;
	}
	FILE *fp = fHandle->mgmtInfo;
	if (fp == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	//check seek if success
	if (fseek(fHandle->mgmtInfo, 0L , SEEK_SET) != 0) {
		return RC_OPERATION_FAILED;
	}
	else {
		//start to append block
		/*attention*/
		char *newPage = (char *)calloc(PAGE_SIZE, sizeof(char));
		memset(newPage, '\0', PAGE_SIZE);

		fwrite(newPage, sizeof(char), PAGE_SIZE , fHandle->mgmtInfo);
		fHandle->curPagePos = fHandle->totalNumPages + 1;
		fHandle->totalNumPages++;

		rewind(fp);

		/*attention*/
		free(newPage);
		return RC_OK;
	}
}
extern RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle) {
	//check file if exsist
	if (fHandle == NULL) {
		return RC_FILE_NOT_FOUND;
	}

	if (fHandle->totalNumPages >= numberOfPages || numberOfPages < 0) {
		return RC_OPERATION_FAILED;
	}
	
	int count = numberOfPages - fHandle->totalNumPages;
	for (int i = 0; i < count; i++) {
		if (appendEmptyBlock(fHandle) != RC_OK)
			return RC_OPERATION_FAILED;
	}
	return RC_OK;
}
