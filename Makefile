all:
	gcc src/main.c -o templeVCS -O3 -Wall -Wno-parentheses
c:	clean
clean:
	rm -f templeVCS

