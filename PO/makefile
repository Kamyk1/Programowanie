all: winsys

winsys: winsys.o screen.o
	g++ -g -Wall -pedantic $^ -o $@ -lncurses

winsys.o: winsys.cpp screen.h cpoint.h
	g++ -g -c -Wall -pedantic $< -o $@

screen.o: screen.cpp screen.h
	g++ -g -c -Wall -pedantic $< -o $@

.PHONY: clean

clean:
	-rm winsys winsys.o screen.o