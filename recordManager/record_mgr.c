#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "record_mgr.h"

void transferTo(char *offset, int size, RM_PageData* src) {
	memcpy(offset, src, size); offset += size;
}
void transferFrom(char *offset, int size, RM_PageData* det) {
	memcpy(det, offset, size); offset += size;
}

int caculateSlots(Schema* schema) {
	int result = 0;
	result = (int)((PAGE_SIZE - (2 * sizeof(int)) - 1) / (getRecordSize(schema) + ((double)1 / CHAR_BIT)));
	return result;
}
#define TO_OFFSET(offset, size, src) memcpy(offset,src,size); offset += size;
#define FROM_OFFSET(offset, size, dest) memcpy(dest,offset,size); offset += size;

void load(BM_PageHandle* pageHandle, RM_PageData* pageData, Schema* schema)
{
    
    char* off = pageHandle->data;
    FROM_OFFSET(off, sizeof(int), &pageData->next);
    FROM_OFFSET(off, sizeof(int), &pageData->prev);
    
    pageData->bitMap = (uint8_t*)off;
    off = off + ((caculateSlots(schema)/CHAR_BIT) + 1);
    pageData->data = off;    
}

void update(RM_TableData* tblData)
{
    RM_TableMgmtData *mgmtData =  tblData->mgmtData;
    RM_PageData *pageData = &mgmtData->pageInContext;
    char* off;
    
    //Check if empty slots are available.
    int count = caculateSlots(tblData->schema);
    int signal = 0;
    int i = 0;
    while (i < count) {
		int result = -1;
		uint8_t bit = pageData->bitMap[i / (CHAR_BIT * sizeof(uint8_t))] & (1 << (i % (CHAR_BIT * sizeof(uint8_t))));
		result = (bit != 0);

        if (result == 0) {
            signal = i;
        }
        i++;
    }

    if (signal == 0) {
        
        //Not the only page in freepage List
        if (mgmtData->pageInContext.next != mgmtData->pageHandle.pageNum) {
            mgmtData->freePage = mgmtData->pageInContext.next;
            
            int temp = 0;
            off = mgmtData->pageHandle.data;
            TO_OFFSET(off, sizeof(int), &temp);
            TO_OFFSET(off, sizeof(int), &temp);
            markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);

            //setting prev of the next page to itself
            BM_PageHandle pageHandleTemp;
            
            pinPage(&mgmtData->bufferPool, &pageHandleTemp, mgmtData->freePage);
            
            off = pageHandleTemp.data;
            off += 4;
            TO_OFFSET(off, sizeof(int), &pageHandleTemp.pageNum);
            markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);
            
            unpinPage(&mgmtData->bufferPool, &pageHandleTemp);
            
        }
        
        else{
            
            mgmtData->freePage = 0;
            
            int temp = 0;
            off = mgmtData->pageHandle.data;
            TO_OFFSET(off, sizeof(int), &temp);
            TO_OFFSET(off, sizeof(int), &temp);
            markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);
            
        }
        
        return;
        
    }
    
    //If not a new page
    if (mgmtData->pageInContext.next != 0) {
        return;
    }
    
    //To newly appended block
    if (mgmtData->freePage == 0) {
        
        mgmtData->freePage = mgmtData->pageHandle.pageNum;
        pageData->next = pageData->prev = mgmtData->pageHandle.pageNum;
        
        //Mark current page
        off = mgmtData->pageHandle.data;
        TO_OFFSET(off, sizeof(int), &mgmtData->pageHandle.pageNum);
        TO_OFFSET(off, sizeof(int), &mgmtData->pageHandle.pageNum);
        markDirty(&mgmtData->bufferPool, &mgmtData->pageHandle);
        
    }
    
    else{
        
        off = mgmtData->pageHandle.data;
        TO_OFFSET(off, sizeof(int), &mgmtData->freePage);
        TO_OFFSET(off, sizeof(int), &mgmtData->pageHandle.pageNum);
        
        BM_PageHandle pageHandle;
        
        //Load current freepage
        pinPage(&mgmtData->bufferPool, &pageHandle, mgmtData->freePage);
        off = pageHandle.data;
        off += 4;
        TO_OFFSET(off, sizeof(int), &mgmtData->pageHandle.pageNum);
        
        markDirty(&mgmtData->bufferPool, &pageHandle);
        
        mgmtData->freePage = mgmtData->pageHandle.pageNum;
        
        unpinPage(&mgmtData->bufferPool, &pageHandle);
        
    }
    
}

// Initilization and table handle
RC initRecordManager (void *mgmtData)
{
	RM_ScanHandle *rm = (RM_ScanHandle *)malloc(sizeof(RM_ScanHandle));
	rm->mgmtData = mgmtData;
	rm->rel = (RM_TableData *)malloc(sizeof(RM_TableData));
	
	ReplacementStrategy strategy = RS_LRU;
	int numPages = 10;
	BM_BufferPool *BM = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
	
    return RC_OK;
}


RC shutdownRecordManager ()
{
	/*
	BM_BufferPool *BM = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
	shutdownBufferPool(BM);
	// R_ID = NULL;

	free(SCHEMA_STR);
	SCHEMA_STR = NULL;

	free(SCHEMA);
	SCHEMA = NULL;


	// Shutdown Buffer Manager
	rc = -99;
	rc = shutdownBufferPool(BM);

	if (rc != RC_OK) {
		return rc;
	}

	free(BM);
	BM = NULL;

	free(REL);
	REL = NULL;
	free(BM);
	*/
    return RC_OK;
}


RC createTable (char *name, Schema *schema)
{
    SM_FileHandle *fileHandle;
    char data[PAGE_SIZE];
    char *off = data;
	int schemaSize = 0;
	int recordSize = 0;

	int count = schemaSize + recordSize;
    
	schemaSize += count;
	recordSize += count;

    schemaSize += sizeof(int) * (4 + schema->keySize); //RecordCount+FreePage+NumAttr+KeySize
    schemaSize += schema->numAttr * (2 * sizeof(int) + 32);
                                                        //DataType+TypeLength+FiledNameLength
    if (schemaSize > PAGE_SIZE) {
        return RC_SCHEMA_ERROR;
    }
    
    recordSize += getRecordSize(schema) + (sizeof(int) * 2) + 1; //Next + Prev + BitMap
    
    if (recordSize > PAGE_SIZE) {
        return RC_SCHEMA_ERROR;
    }
  
    memset(off, 0, PAGE_SIZE);
    
    int a = 0;          //Record count
    TO_OFFSET(off, sizeof(int), &a);//0
    
    a = 0;              //FreePage Number
    TO_OFFSET(off, sizeof(int), &a);//4
    
    TO_OFFSET(off, sizeof(int), &schema->numAttr);//8
    TO_OFFSET(off, sizeof(int), &schema->keySize);//12
    
    int i = 0;
    
    while (i < schema->keySize) {
        TO_OFFSET(off, sizeof(int), &schema->keyAttrs[i]);//16
        i++;
    }
    
    
    for (i = 0; i < schema->numAttr; i++) {
        
        TO_OFFSET(off, sizeof(int), &schema->dataTypes[i]);
        TO_OFFSET(off, sizeof(int), &schema->typeLength[i]);
        TO_OFFSET(off, 32, schema->attrNames[i]);
    }
    
    fileHandle = (SM_FileHandle *) malloc(sizeof(SM_FileHandle));
	RC rc = createPageFile(name);
    if (rc != RC_OK) {
        return rc;
    }
	openPageFile(name, fileHandle);
    
	writeBlock(0, fileHandle, data);
    
	closePageFile(fileHandle);
    free(fileHandle);
    
    return RC_OK;
}

RC openTable(RM_TableData *rel, char *name)
{
    rel->name = strdup(name);
    RM_TableMgmtData *tblMgmtData = (RM_TableMgmtData*) malloc(sizeof(RM_TableMgmtData));
    rel->mgmtData = tblMgmtData;
    char* off;
    
    RC st = initBufferPool(&tblMgmtData->bufferPool, rel->name, 1024, RS_FIFO, NULL);
    
    if (st != RC_OK) {
        return st;
    }
    st = pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, (PageNumber)0);
    if (st != RC_OK) {
        return st;
    }
    
    off = (char *) tblMgmtData->pageHandle.data;
    
    FROM_OFFSET(off, sizeof(int), &tblMgmtData->count);
    FROM_OFFSET(off, sizeof(int), &tblMgmtData->freePage);
    
    char* tempOff = off;
    int numAttr, keySize;
    
    FROM_OFFSET(tempOff, sizeof(int), &(numAttr));
    FROM_OFFSET(tempOff, sizeof(int), &(keySize));
    
    rel->schema = (Schema*) malloc(sizeof(Schema));
    
    rel->schema->numAttr = numAttr;
    rel->schema->attrNames = (char**)malloc(sizeof(char*) * numAttr);
    rel->schema->dataTypes = (DataType*)malloc(sizeof(DataType)*numAttr);
    rel->schema->typeLength = (int*) malloc(sizeof(int) * numAttr);
    rel->schema->keySize = keySize;
    rel->schema->keyAttrs = (int*) malloc(sizeof(int) * keySize);
    int j = 0;
    while (j < numAttr) {
        rel->schema->attrNames[j] = (char*)malloc(32);
        j++;
    }
    
    FROM_OFFSET(off, sizeof(int), &(rel->schema->numAttr));
    FROM_OFFSET(off, sizeof(int), &(rel->schema->keySize));
    
    int i = 0;
    while (i < rel->schema->keySize) {
        FROM_OFFSET(off, sizeof(int), &rel->schema->keyAttrs[i]);
        i++;
    }
    
    for (i = 0; i < rel->schema->numAttr; i++) {
        
        FROM_OFFSET(off, sizeof(int), &rel->schema->dataTypes[i]);
        FROM_OFFSET(off, sizeof(int), &rel->schema->typeLength[i]);
        FROM_OFFSET(off, 32, rel->schema->attrNames[i]);
    }
    st =unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    if (st != RC_OK) {
        return st;
    }
    
    return RC_OK;
}

//Write stuffs and close the table
RC closeTable (RM_TableData *rel)
{
    RM_TableMgmtData *tblMgmtData = rel->mgmtData;
	PageNumber pagNum = (PageNumber)0;
    RC st = pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, pagNum);

    char* off;
    off = tblMgmtData->pageHandle.data;
    markDirty(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    TO_OFFSET(off, sizeof(int), &tblMgmtData->count);
    TO_OFFSET(off, sizeof(int), &tblMgmtData->freePage);
    
    unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    shutdownBufferPool(&tblMgmtData->bufferPool);
    free(rel->schema);
    free(rel->mgmtData);
    free(rel->name);
    
    return RC_OK;
}

RC deleteTable (char *name)
{
    destroyPageFile(name);
    return RC_OK;
}

int getNumTuples (RM_TableData *rel)
{
	int count = ((RM_TableMgmtData *)rel->mgmtData)->count;
    return count;
}

// Handle record
RC insertRecord (RM_TableData *rel, Record *record)
{
    RM_TableMgmtData *tblMgmtData = rel->mgmtData;
	BufferInfo *poolMgmtData = tblMgmtData->bufferPool.mgmtData;
    RC st;
    
    if(tblMgmtData->freePage == 0)
    {
		appendEmptyBlock(poolMgmtData->fileHandle);
		record->id.slot = 0;        
        record->id.page = poolMgmtData->fileHandle->totalNumPages - 1;       
        
        if ((st = pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, record->id.page))!=RC_OK) {
            return st;
        }
        
        char* off = tblMgmtData->pageHandle.data;
        
        int a = 0; //Record count
        TO_OFFSET(off,sizeof(int),&a);//0
        
        a = 0; //FreePage Number
        TO_OFFSET(off, sizeof(int), &a);//4
        
        off = NULL;
        
        load( &tblMgmtData->pageHandle, &tblMgmtData->pageInContext, rel->schema);
        
    }
    
    else    {
        record->id.page = tblMgmtData->freePage;
        
        if ((st = pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, record->id.page))!=RC_OK) {
            return st;
        }
        
        load( &tblMgmtData->pageHandle, &tblMgmtData->pageInContext, rel->schema);
        
        int count = caculateSlots(rel->schema);
        
        int i = 0;
        while (i < count) {
			int result = -1;
			uint8_t bit = tblMgmtData->pageInContext.bitMap[i / (CHAR_BIT * sizeof(uint8_t))] & (1 << (i % (CHAR_BIT * sizeof(uint8_t))));
			result = (bit != 0);

			if (result == 0) {
                record->id.slot = i;
                break;
            }
            i++;
        }
        
    }
    
    if (record->id.page < 1 || record->id.slot < 0) {
        return RC_SCHEMA_ERROR;
    }
    
    markDirty(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    char* off = tblMgmtData->pageInContext.data;
    off += record->id.slot * getRecordSize(rel->schema);
    
    TO_OFFSET(off, getRecordSize(rel->schema), record->data);
    
	tblMgmtData->pageInContext.bitMap[record->id.slot / (CHAR_BIT * sizeof(uint8_t))] |= (1 << (record->id.slot % (CHAR_BIT * sizeof(uint8_t))));
    tblMgmtData->count++;
    
    update(rel);
    
	unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    return RC_OK;
}


RC deleteRecord (RM_TableData *rel, RID id)
{
    RM_TableMgmtData* tblMgmtData = rel->mgmtData;
    
    pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, id.page);
    
    
    load(&tblMgmtData->pageHandle, &tblMgmtData->pageInContext, rel->schema);
    
    markDirty(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
	tblMgmtData->pageInContext.bitMap[id.slot / (CHAR_BIT * sizeof(uint8_t))] &= ~(1 << (id.slot % (CHAR_BIT * sizeof(uint8_t))));
	unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    tblMgmtData->count--;
    
    update(rel);
    
    return RC_OK;
    
}


RC updateRecord (RM_TableData *rel, Record *record)
{
    RM_TableMgmtData* tblMgmtData = rel->mgmtData;
    
    pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, record->id.page);
    
    
    load(&tblMgmtData->pageHandle, &tblMgmtData->pageInContext, rel->schema);
    
    if (record->id.page < 1 || record->id.slot < 0) {
        return RC_SCHEMA_ERROR;
    }
    
    markDirty(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    char* off = tblMgmtData->pageInContext.data;
    off += record->id.slot*getRecordSize(rel->schema);
    
    TO_OFFSET(off, getRecordSize(rel->schema), record->data);
    
	tblMgmtData->pageInContext.bitMap[record->id.slot / (CHAR_BIT * sizeof(uint8_t))] |= (1 << (record->id.slot % (CHAR_BIT * sizeof(uint8_t))));
	unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    return RC_OK;
}

RC getRecord (RM_TableData *rel, RID id, Record *record)
{
    RM_TableMgmtData* tblMgmtData = rel->mgmtData;
    
    record->id.page = id.page;
    record->id.slot = id.slot;
    
    pinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle, record->id.page);
        
    load(&tblMgmtData->pageHandle, &tblMgmtData->pageInContext, rel->schema);
    
    if (record->id.page < 1 || record->id.slot < 0) {
        return RC_SCHEMA_ERROR;
    }
	int result = -1;
	uint8_t bit = tblMgmtData->pageInContext.bitMap[record->id.slot / (CHAR_BIT * sizeof(uint8_t))] & (1 << (record->id.slot % (CHAR_BIT * sizeof(uint8_t))));
	result = (bit != 0);
    if (result!=1) {
        return RC_SCHEMA_ERROR;
    }
    
    char* off = tblMgmtData->pageInContext.data;
    off += record->id.slot*getRecordSize(rel->schema);
    
    FROM_OFFSET(off, getRecordSize(rel->schema), record->data);
    
    unpinPage(&tblMgmtData->bufferPool, &tblMgmtData->pageHandle);
    
    return RC_OK;
}

// Handle scans
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
    
    RM_ScanMgmtData *scanMgmtData = (RM_ScanMgmtData *) malloc(sizeof(RM_ScanMgmtData));
    
    scanMgmtData->cond = cond;
    scanMgmtData->scanCount = 0;
    scanMgmtData->rid.slot = 0;
    scanMgmtData->rid.page = -1;
    
    scan->rel = rel;
	scan->mgmtData = scanMgmtData;
    return RC_OK;
}

RC next (RM_ScanHandle *scan, Record *record)
{
    RM_ScanMgmtData *scanMgmtData = scan->mgmtData;
    RM_TableMgmtData *tblMgmtData = scan->rel->mgmtData;
    
    Value *val = (Value*)malloc(sizeof(Value));
    val->v.boolV = TRUE;
    
    if (tblMgmtData->count < 1) {
        return RC_RM_NO_MORE_TUPLES;
    }
    do
    {
        if (scanMgmtData->scanCount==0) {
            
            scanMgmtData->rid.page = 1;
            scanMgmtData->rid.slot = 0;
            
            pinPage(&tblMgmtData->bufferPool, &scanMgmtData->pageHandle, (PageNumber)scanMgmtData->rid.page);
            
            load(&scanMgmtData->pageHandle, &scanMgmtData->pageData, scan->rel->schema);
            
        }
        
        else if (scanMgmtData->scanCount == tblMgmtData->count){
            
            unpinPage(&tblMgmtData->bufferPool, &scanMgmtData->pageHandle);
            
            scanMgmtData->rid.page = -1;
            scanMgmtData->rid.slot = -1;
            scanMgmtData->scanCount = 0;
            
            return RC_RM_NO_MORE_TUPLES;
        }
        
        else{
            
            scanMgmtData->rid.slot += 1;
            
            if (scanMgmtData->rid.slot == caculateSlots(scan->rel->schema)) {
                
                unpinPage(&tblMgmtData->bufferPool, &scanMgmtData->pageHandle);
                
                scanMgmtData->rid.page+=1;
                scanMgmtData->rid.slot = 0;
                
                pinPage(&tblMgmtData->bufferPool, &scanMgmtData->pageHandle, (PageNumber)scanMgmtData->rid.page);
                load(&scanMgmtData->pageHandle, &scanMgmtData->pageData, scan->rel->schema);
                
            }
            
        }
        
        record->id.slot = scanMgmtData->rid.slot;
        record->id.page = scanMgmtData->rid.page;
        
        if (record->id.page < 1 || record->id.slot < 0) {
            return RC_SCHEMA_ERROR;
        }
		int result = -1;
		uint8_t bit = scanMgmtData->pageData.bitMap[record->id.slot / (CHAR_BIT * sizeof(uint8_t))] & (1 << (record->id.slot % (CHAR_BIT * sizeof(uint8_t))));
		result = (bit != 0);
        if (result !=1) {
            return RC_SCHEMA_ERROR;
        }
        
        char* off = scanMgmtData->pageData.data;
        off += record->id.slot*getRecordSize(scan->rel->schema);
        
        FROM_OFFSET(off, getRecordSize(scan->rel->schema), record->data);
        
        scanMgmtData->scanCount += 1;
        
        if (scanMgmtData->cond !=NULL) {
            evalExpr(record, scan->rel->schema, scanMgmtData->cond, &val);
        }
        
    }while(!val->v.boolV && scanMgmtData->cond !=NULL);
    
    free(val);
    
    return RC_OK;
}


RC closeScan (RM_ScanHandle *scan)
{
    free(scan->mgmtData);
    return RC_OK;
}

// dealing with schemas
int getRecordSize (Schema *schema)
{
    int size = 0;
    int i = 0;
    while (i < schema->numAttr) {
        size += schema->typeLength[i];
        i++;
    }
    return size;
}

Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
    Schema* schema = (Schema*) malloc(sizeof(Schema));
    
    schema->numAttr = numAttr;
    schema->attrNames = (char**)malloc(numAttr*sizeof(char*));
    schema->dataTypes = (DataType*)malloc(numAttr*sizeof(DataType));
    schema->typeLength = (int*) malloc(numAttr*sizeof(int));
    schema->keySize = keySize;
    schema->keyAttrs = (int*) malloc(keySize*sizeof(int));
    int i = 0;
    while (i < numAttr) {
        schema->attrNames[i] = (char*)malloc(32*1);
        i++;
    }

    for (i = 0; i < numAttr; i++) {
        
        strncpy(schema->attrNames[i], attrNames[i],32*1);
        schema->dataTypes[i] = dataTypes[i];
        
        switch (dataTypes[i]) {
                
            case DT_INT:
                schema->typeLength[i] = sizeof(int);
                break;
            case DT_BOOL:
                schema->typeLength[i] = sizeof(bool);
                break;
            case DT_FLOAT:
                schema->typeLength[i] = sizeof(float);
                break;
            case DT_STRING:
                schema->typeLength[i] = typeLength[i];
                break;
            default:
                break;
                
        }
        
        if (i < keySize) {
            schema->keyAttrs[i] = keys[i];
        }
        
    }
    
    return schema;

}

RC freeSchema (Schema *schema)
{
    free(schema);
    
    return RC_OK;
}

// Handle records
RC createRecord (Record **record, Schema *schema)
{
    *record = (Record *) malloc(sizeof(Record));
    (*record)->data = (char *) malloc((getRecordSize(schema)));
    return RC_OK;
}

RC freeRecord (Record *record)
{
    free(record->data);
    free(record);
    return RC_OK;
}

RC getAttr (Record *record, Schema *schema, int attrNum, Value **value)
{
    void* off = record->data;
    *value = (Value*) malloc(sizeof(Value));
    int gap = 0, i = 0;
    
    while (i < attrNum) {
        gap += schema->typeLength[i];
        i++;
    }
    
    off += gap;
    (*value)->dt = schema->dataTypes[attrNum];
    
    switch (schema->dataTypes[attrNum]) {
            
        case DT_INT:
            memcpy(&((*value)->v.intV), off, schema->typeLength[attrNum]);
            break;
            
        case DT_BOOL:
            memcpy(&((*value)->v.boolV), off, schema->typeLength[attrNum]);
            break;
            
        case DT_FLOAT:
            
            memcpy(&((*value)->v), off, schema->typeLength[attrNum]);
            break;
            
        case DT_STRING:
            
            ((*value)->v.stringV) = (char*)malloc(schema->typeLength[attrNum]+1);
            FROM_OFFSET(off, schema->typeLength[attrNum], (*value)->v.stringV);
            (*value)->v.stringV[schema->typeLength[attrNum]] = '\0';
            break;
            
        default:
            break;
            
    }
    
    return RC_OK;
}

RC setAttr (Record *record, Schema *schema, int attrNum, Value *value)
{
    void* offset = record->data;
    int gap = 0, i = 0;
    
    while (i < attrNum) {
        gap += schema->typeLength[i];
        i++;
    }
    
    offset += gap;
    
    switch (value->dt) {
            
        case DT_INT:
            TO_OFFSET(offset, schema->typeLength[attrNum], &value->v.intV);
            break;
        case DT_BOOL:
            TO_OFFSET(offset, schema->typeLength[attrNum], &value->v.boolV);
            break;
        case DT_FLOAT:
            
            TO_OFFSET(offset, schema->typeLength[attrNum], &value->v);
            break;
            
        case DT_STRING:
            
            TO_OFFSET(offset, schema->typeLength[attrNum], value->v.stringV);
            break;
            
        default:
            break;
            
    }
    
    
    return RC_OK;
}
