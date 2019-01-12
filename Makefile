all:	clean
	gcc src/main.c -o templeVCS -O3 -Wall -Wno-parentheses -lssl -lcrypto
debug:	clean
	gcc src/main.c -o templeVCS -g -Wall -Wno-parentheses -lssl -lcrypto
c:	clean
d:	debug
clean:
	rm -f templeVCS

