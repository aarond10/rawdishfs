CPPFLAGS = -g
LDFLAGS = -lpthread -lstdc++ -lglog -lgtest -lgtest_main

all: bloomfilter_test lrucache_test url_test

clean:
	rm -f *.o bloomfilter_test lrucache_test url_test

bloomfilter_test: bloomfilter_test.o bloomfilter.o
	g++ -o $@ $^ ${LDFLAGS}

lrucache_test: lrucache_test.o lrucache.h
	g++ -o $@ $^ ${LDFLAGS}

url_test: url_test.o url.h
	g++ -o $@ $^ ${LDFLAGS}

test: all
	valgrind ./bloomfilter_test
	valgrind ./lrucache_test
	valgrind ./url_test
