include Makefile.config

.PHONY: all
all:
	make -C ../third_party all
	make -C util all
	make -C rpc all
	make -C blockstore all

.PHONY: clean
clean:
	make -C ../third_party clean
	make -C util clean
	make -C rpc clean
	make -C blockstore clean
