# cs525-s18-shuang

Basic Functions:
This assignment is to creat a record manager. The record manager handles tables with a fixed schema. Clients can insert records, delete records, update records, and scan through the records in a table. A scan is associated with a search condition and only returns records that match the search condition. Each table should be stored in a separate page file and your record manager should access the pages of the file through the buffer manager implemented in the last assignment.

    Schema Functions include: 
        extern int getRecordSize (Schema *schema);
        extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int                                *typeLength, int keySize, int *keys);
        extern RC freeSchema (Schema *schema);
    Table Functions include:
        extern RC initRecordManager (void *mgmtData);
        extern RC shutdownRecordManager ();
        extern RC createTable (char *name, Schema *schema);
        extern RC openTable (RM_TableData *rel, char *name);
        extern RC closeTable (RM_TableData *rel);
        extern RC deleteTable (char *name);
        extern int getNumTuples (RM_TableData *rel);
    Records Functions include:
        extern RC createRecord (Record **record, Schema *schema);
        extern RC freeRecord (Record *record);
        extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value);
        extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value);
        extern RC insertRecord (RM_TableData *rel, Record *record);
        extern RC deleteRecord (RM_TableData *rel, RID id);
        extern RC updateRecord (RM_TableData *rel, Record *record);
        extern RC getRecord (RM_TableData *rel, RID id, Record *record);
    Scans Functions include:
        extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond);
        extern RC next (RM_ScanHandle *scan, Record *record);
        extern RC closeScan (RM_ScanHandle *scan);

We constructed RM_PageData class to store the page number of previous slot and next slot. There is a bitMap instance because we struct offset methods to adjust the input data by using the uint8_t in the "stdint.h" header. We also constructed the RM_TableMgmtData and RM_ScanMgmtData classes to store the PageHandle and PageData classes. 

To compile the assignment #3, change direction to the assignment3 folder,
    type make, then run by typing ./assign3_1
It will go through it.

Contributions:
    Guangwei Ban:  Scans Functions
    Guenzhe Sun:    Table Funtions
    Silin Huang:        Environmental Settings and Records
    Zehao Lu:           Debug and Records Fucntions and Documentation



