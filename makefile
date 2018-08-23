# John McBride
# Description: Perform `make` or `make wish`to build the wish shell
# 		`make clean` will eliminate object files and the wish executible file
# 		`make valgrind` will start the wish shell using the valgrind debugging tool

HEADERS = wish.h
SOURCES = wish.c buffer_io.c utility.c

default: wish

wish.o: $(SOURCES) $(HEADERS)
	gcc -Wall -c $(SOURCES) 

buffer_io.o: $(SOURCES) $(HEADERS)
	gcc -Wall -c buffer_io.c

utility.o: utility.c wish.h
	gcc -Wall -c utility.c

wish: wish.o buffer_io.o utility.o 
	gcc -Wall -o wish wish.o buffer_io.o utility.o

clean:
	rm -f wish
	rm -f *.o

valgrind:
	make wish
	valgrind --leak-check=full --show-reachable=yes ./wish
