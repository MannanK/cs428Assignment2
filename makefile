all:	prog2

prog2:	prog2.o
	g++ -pthread -std=c++11 Node.o prog2.o -o prog2

prog2.o:	prog2.cpp
	g++ -std=c++11 -c Node.cpp prog2.cpp

clean:
	rm -f *.o prog2
