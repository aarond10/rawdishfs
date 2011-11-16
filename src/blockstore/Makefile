include ../Makefile.config
CPPFLAGS:= ${CPPFLAGS} -g
LDFLAGS:= ${LDFLAGS} -lpthread -lstdc++ -lglog -lgtest -lgtest_main

all: fileblockstore_test

blockstore.a: fileblockstore.o
	ar cr $@ $^

fileblockstore_test: fileblockstore_test.o blockstore.a ../util/util.a
	g++ -o $@ $^ ${LDFLAGS}

.PHONY: clean
clean:
	rm -f *.a *.o fileblockstore_test

.PHONY: test
test: fileblockstore_test
	valgrind ./fileblockstore_test
