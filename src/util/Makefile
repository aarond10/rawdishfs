include ../Makefile.config
CPPFLAGS = -g -I${TOP}
LDFLAGS = -lpthread -lstdc++ -lglog -lgtest -lgtest_main

.PHONY: all
all: bloomfilter_test lrucache_test url_test

.PHONY: clean
clean:
	rm -f *.o bloomfilter_test lrucache_test url_test

util.a: bloomfilter.o
	ar cr $@ $^

bloomfilter_test: bloomfilter_test.o util.a
	g++ -o $@ $^ ${LDFLAGS}

lrucache_test: lrucache_test.o lrucache.h
	g++ -o $@ $^ ${LDFLAGS}

url_test: url_test.o url.h
	g++ -o $@ $^ ${LDFLAGS}

.PHONY: test
test: all
	valgrind ./bloomfilter_test
	valgrind ./lrucache_test
	valgrind ./url_test
