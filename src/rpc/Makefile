include ../Makefile.config
CPPFLAGS:= ${CPPFLAGS} -g
LDFLAGS:= ${LDFLAGS} -lpthread -lstdc++ -lglog -lgtest -lgtest_main -lmsgpack -lepoll_threadpool

all: rpc_test

rpc_test: rpc_test.o rpc.o
	g++ -o $@ $^ ${LDFLAGS}

.PHONY: clean
clean:
	rm -f *.a *.o rpc_test

.PHONY: test
test: rpc_test
	valgrind ./rpc_test
