all:
	gcc src/main.c -o templeVCS -O3 -Wall
c:	clean
clean:
	rm templeVCS

