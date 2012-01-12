.PHONY: all clean test

all:
	make -C third_party all
	make -C src all

clean:
	make -C third_party clean
	make -C src clean

test:
	make -C third_party all
	make -C src test

