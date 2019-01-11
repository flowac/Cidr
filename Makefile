all:
#	gcc src/main.c -o templeVCS -O3 -Wall -Wno-parentheses -lssl -lcrypto
	gcc src/main.c -o templeVCS -g -Wall -Wno-parentheses -lssl -lcrypto
c:	clean
clean:
	rm -f templeVCS

