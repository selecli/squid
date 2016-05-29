include ../../../Make.properties

INCLUDE	= -I../bdb_lib -I.. -I../.. 
LIBRARY = 
#LIB		= -ldb

all: 
	$(EXD_LINK) -o refresh_cli $(LIB) $(INCLUDE) $(LIBRARY) refresh_cli.c
	$(EXD_LINK) -o refreshd_perf_test $(LIB) $(INCLUDE) $(LIBRARY) refreshd_perf_test.c
	$(EXD_LINK) -c -o common.o $(LIB) $(INCLUDE) $(LIBRARY) ../common.c
	$(EXD_LINK) -c -o log.o $(LIB) $(INCLUDE) $(LIBRARY) ../log.c -D__THREAD_SAFE__
	$(EXD_LINK) -c -o md5.o $(LIB) $(INCLUDE) $(LIBRARY) ../md5.c 
	$(EXD_LINK) -g -o check_db $(LIB) $(INCLUDE) $(LIBRARY) check_db.c log.c
	$(EXD_LINK) -o hash_db  $(LIB) $(INCLUDE) $(LIBRARY) hash_db.c

install:
	/bin/cp -f refresh_cli ../
	/bin/cp -f refreshd_perf_test ../
	/bin/cp -f check_db ../
	/bin/cp -f hash_db ../

clean:
	rm -f *.o
	rm -f refresh_cli
	rm -f test_fc
	rm -f t_common
	rm -f refreshd_perf_test
	rm -f write_db
	rm -f check_db
	rm -f hash_db
