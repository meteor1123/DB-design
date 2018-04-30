SRC := \
buffer_mgr.c \
buffer_mgr_stat.c \
dberror.c \
storage_mgr.c \
record_mgr.c \
rm_serializer.c \
expr.c \
test_assign3_1.c
	
	
OBJ := \
buffer_mgr.o \
buffer_mgr_stat.o \
dberror.o \
storage_mgr.o \
record_mgr.o \
rm_serializer.c \
expr.o \
test_assign3_1.o
	
	

assign3: $(OBJ)
	gcc -o assign3 $?
	

%.o: %.c
	gcc -g -c $<

clean:
	rm -rf assign3 *.o
