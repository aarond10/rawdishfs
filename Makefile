.PHONY: all clean

all:
	make -C third_party all
	make -C src all

clean:
	make -C third_party clean
	make -C src clean
