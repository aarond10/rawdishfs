include ../Makefile.config
CPPFLAGS:= ${CPPFLAGS} -O3
LDFLAGS:= ${LDFLAGS} -lpthread -lstdc++ -lglog -lgtest -lgtest_main -lmsgpack -lepoll_threadpool

all: rpc_test rpc_benchmark

rpc.a: rpc.o
	ar cr $@ $^

rpc_test: rpc_test.o rpc.a
	g++ -o $@ $^ ${LDFLAGS}

rpc_benchmark: rpc_benchmark.o rpc.a
	g++ -o $@ $^ ${LDFLAGS}

.PHONY: clean
clean:
	rm -f *.a *.o rpc_test rpc_benchmark

.PHONY: test
test: rpc_test
	valgrind --db-attach=yes ./rpc_test
	valgrind --db-attach=yes ./rpc_benchmark
