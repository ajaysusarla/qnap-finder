all: qnap-finder


qnap-finder.o: qnap-finder.c qnap-finder.h
	gcc -Wall -ggdb -c -o qnap-finder.o qnap-finder.c

list.o: list.c list.h
	gcc -Wall -ggdb -c -o list.o list.c

qnap-finder: qnap-finder.o list.o
	gcc -Wall -ggdb -o qnap-finder qnap-finder.o list.o -lpthread -lm

clean:
	rm -f *.o qnap-finder
