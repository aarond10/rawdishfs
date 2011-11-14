all: epoll_threadpool

clean:
	make -C epoll_threadpool/src clean
	rm -rf ../src/include ../src/lib

.PHONY: epoll_threadpool
epoll_threadpool:
	make -C epoll_threadpool/src
	mkdir -p ../src/include
	mkdir -p ../src/lib
	cp epoll_threadpool/src/lib/* ../src/lib
	cp -r epoll_threadpool/src/include/* ../src/include
