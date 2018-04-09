
SRC := \
	dberror.c \
	storage_mgr.c \
	test_assign1_1.c
OBJ := \
	dberror.o \
	storage_mgr.o \
	test_assign1_1.o

test_assign1 : $(OBJ)
	gcc -o test_assign1  $?
	

%.o: %.c
	gcc -g -c $<

clean:
	rm -rf test_assign1  *.o
