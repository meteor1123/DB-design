
SRC := \
	dberror.c \
	storage_mgr.c \
	buffer_mgr.c \
	test_assign2_1.c
OBJ := \
	dberror.o \
	storage_mgr.o \
	buffer_mgr.o \
	test_assign2_1.o

test: $(OBJ)
	gcc -o test2 $?
	

%.o: %.c
	gcc -g -c $<

clean:
	rm -rf test2 *.o
