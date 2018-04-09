This is readme for Group4 's Assignment1.


In this Assignment, we add storage_mgr.c and one new DEFINE CONST PARAMETER in dberror.h.


We create 3 new method: checkHandle-to check filehandle if is valid; 
writeMethod-basically write data to file; readMethod-basically read data from file.

If these 3 methods work, it will return RC_OK; 
else, return error codes such as RC_FILE_HANDLE_NOT_INIT,RC_WRITE_FAILED,RC_READ_NON_EXISTING_PAGE and so on.

We also use some system methods(remove,for example) and stream methods(fwrite,fread,fclose) to implemnt all interfaces. 
Of course, if any errors happens, there will be return specific error code. 
If it works, return RC_OK.



Zehao Lv- Basic methods

Silin Huang,Guenzhe Sun- page file methods

Guangwei Ban- Block methods and bug fix
