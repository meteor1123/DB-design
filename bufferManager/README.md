# cs525-s18-shuang

For basic functions：
In this Assignment, we add buffer_mgr.c to implement buffer functions.In addition, we use the file storage_mgr.c in A1 to continue our work.

strategy implemntation.
We create one struct-BuffInfo to store buff related information, such as times of writing and reading. parameter strategy aims to determine to use FIFO or LRU algorithm.
We use one link list to record page frame and calculate times of pining. 
Basic methods are:
	void init_PagePointer(PagePointer *pointer);
	void init_BPmanager(Manager *manager);
	PagePointer *getPagePointer(BM_BufferPool *const bm, PageNumber pNum);
And methods traverse_list to count nodes;
	    remove_at_head remove first node;
	    remove_at_current remove current node;
	    add_node insert node.
When node will be removed, we will check if it is dirty and if it needs to be writen to disk;
When node will be added, we will check if buffer is full and if we need to replace old page using different algorithm.
buffer storage implementation. 	
In init and shutdown method, we will malloc or free one buffer pool and related page pointer.





Zehao Lv- algorithm

Silin Huang,Guenzhe Sun- static methods

Guangwei Ban- buffer methods and bug fix
