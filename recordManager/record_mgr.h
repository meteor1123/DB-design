#ifndef RECORD_MGR_H
#define RECORD_MGR_H

#include <stdint.h>
#include "dberror.h"
#include "expr.h"
#include "tables.h"

#include "buffer_mgr.h"
#include "storage_mgr.h"
// Bookkeeping for scans
typedef struct RM_ScanHandle
{
  RM_TableData *rel;
  void *mgmtData;
} RM_ScanHandle;

// customer struct
typedef struct RM_PageData {

	int next;       //Stores the page number having empty slots
	int prev;       //Stores the page number having empty slots

	uint8_t *bitMap; //BitMap to identify Empty/Deleted Slots

	char *data;      //Record's begin here(i.e., Slot-0 of this page starts here)

}RM_PageData;

typedef struct RM_TableMgmtData {

	int count;
	int freePage;

	RM_PageData pageInContext;
	BM_BufferPool bufferPool;
	BM_PageHandle pageHandle;

}RM_TableMgmtData;

typedef struct RM_ScanMgmtData {

	BM_PageHandle pageHandle;
	RM_PageData pageData;

	RID rid;
	int scanCount;
	Expr *cond;

}RM_ScanMgmtData;
// table and manager
extern RC initRecordManager (void *mgmtData);
extern RC shutdownRecordManager ();
extern RC createTable (char *name, Schema *schema);
extern RC openTable (RM_TableData *rel, char *name);
extern RC closeTable (RM_TableData *rel);
extern RC deleteTable (char *name);
extern int getNumTuples (RM_TableData *rel);

// handling records in a table
extern RC insertRecord (RM_TableData *rel, Record *record);
extern RC deleteRecord (RM_TableData *rel, RID id);
extern RC updateRecord (RM_TableData *rel, Record *record);
extern RC getRecord (RM_TableData *rel, RID id, Record *record);

// scans
extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond);
extern RC next (RM_ScanHandle *scan, Record *record);
extern RC closeScan (RM_ScanHandle *scan);

// dealing with schemas
extern int getRecordSize (Schema *schema);
extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys);
extern RC freeSchema (Schema *schema);

// dealing with records and attribute values
extern RC createRecord (Record **record, Schema *schema);
extern RC freeRecord (Record *record);
extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value);
extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value);

#endif // RECORD_MGR_H
