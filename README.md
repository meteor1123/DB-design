# cs525-s18-SHuang
This is readme for Group4 's Assignment2.

For basic functions：
In this Assignment, we add buffer_mgr.c to implement buffer functions, strategy.c to implement FIFO and LRU, and 2 new DEFINE CONST PARAMETER in dberror.h.
In addition, we use the file storage_mgr.c in A1 to continue our work.

strategy implemntation.
We create one struct-Manager to control FIFO and LRU strategy and use struct PagePointer to record page frame. With these 2 struct, we implemnt FIFO and LRU strategy using linked list(struct FIFOnode and LRUnode). All these implmented in strategy.c
Basic methods are:
	void init_PagePointer(PagePointer *pointer);
	void init_BPmanager(Manager *manager);
	PagePointer *getPagePointer(BM_BufferPool *const bm, PageNumber pNum);
And methods 
	RC updateLRU(PagePointer *pointer, BM_BufferPool *const bm);
	RC updateFIFO(PagePointer* pointer, BM_BufferPool *const bm);
Both aims to insert page frame into buffer pool following different strategy.

buffer storage implementation. 	
In init and shutdown method, we will malloc or free one buffer pool and related strategy manager, page pointer.
As for other interfaces, i use strategy.c's methods to help me to implement them following buffer's concept.

Bonus: lock implementation.
The easiest way to implement lock is 'pthread_mutex_lock' provided by linux. We add it as const function in buffer_mgr. In every buffer read/write method, we add one lock and when the operation ends, un lock it.




Zehao Lv- lock

Silin Huang,Guenzhe Sun- strategy methods

Guangwei Ban- buffer methods and bug fix
