include ../Makefile.config

all: fileutil rpc_test

fileutil: fileutil.o
	libtool --mode=link g++ -o $@ ${LDFLAGS} $^

rpc_test: rpc_test.o rpc.o
	libtool --mode=link g++ -o $@ ${GTEST_OBJ} ${LDFLAGS} $^

.PHONY: clean
clean:
	rm -f *.a *.o rpc_test fileutil

.PHONY: test
test: rpc_test
	valgrind ./rpc_test
