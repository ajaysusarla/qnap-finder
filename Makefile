all: qnap-finder

qnap-finder: qnap-finder.c qnap-finder.h
	gcc -o qnap-finder qnap-finder.c -lpthread -lm

clean:
	rm -f *.o qnap-finder
