include ../Makefile.config
CPPFLAGS:= ${CPPFLAGS} -g -pg
LDFLAGS:= ${LDFLAGS} -lpthread -lstdc++ -lglog -lgtest -lgtest_main -pg -lepoll_threadpool -lmsgpack

all: fileblockstore_test blockstore_daemon

blockstore.a: fileblockstore.o
	ar cr $@ $^

fileblockstore_test: fileblockstore_test.o blockstore.a ../util/util.a
	g++ -o $@ $^ ${LDFLAGS}

blockstore_daemon: blockstore_daemon.o blockstore.a ../rpc/rpc.o ../util/util.a
	g++ -o $@ $^ ${LDFLAGS}


.PHONY: clean
clean:
	rm -f *.a *.o fileblockstore_test blockstore_daemon

.PHONY: test
test: fileblockstore_test
	valgrind ./fileblockstore_test
