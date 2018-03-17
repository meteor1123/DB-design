
SRC := \
	dberror.c \
	storage_mgr.c \
	test_assign2_1.c \
	buffer_mgr.c \
	buffer_mgr_stat.c \
	strategy.c

	
OBJ := \
	dberror.o \
	storage_mgr.o \
	test_assign2_1.o \
	buffer_mgr.o \
	buffer_mgr_stat.o \
	strategy.o

assign2: $(OBJ)
	gcc -o assign2 $?
	

%.o: %.c
	gcc -g -c $<

clean:
	rm -rf assign2 *.o