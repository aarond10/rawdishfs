include Makefile.config

.PHONY: all
all:
	make -C util all
	make -C rpc all
	make -C blockstore all

.PHONY: clean
clean:
	make -C util clean
	make -C rpc clean
	make -C blockstore clean

.PHONY: test
test:
	make -C util test
	make -C rpc test
	make -C blockstore test
