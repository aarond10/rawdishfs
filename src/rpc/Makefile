include ../Makefile.config
CPPFLAGS:= ${CPPFLAGS} -O3
LDFLAGS:= ${LDFLAGS} -lpthread -lstdc++ -lglog -lgtest -lgtest_main -lmsgpack -lepoll_threadpool

all: rpc_test rpc_benchmark

rpc_test: rpc_test.o rpc.o
	g++ -o $@ $^ ${LDFLAGS}

rpc_benchmark: rpc_benchmark.o rpc.o
	g++ -o $@ $^ ${LDFLAGS}

.PHONY: clean
clean:
	rm -f *.a *.o rpc_test rpc_benchmark

.PHONY: test
test: rpc_test
	valgrind ./rpc_test
	valgrind ./rpc_benchmark
