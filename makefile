all:	controlClient

controlClient:	controlClient.o
	g++ -pthread -std=c++11 Node.o prog2.o -o controlClient

controlClient.o:	controlClient.cpp
	g++ -std=c++11 -c Node.cpp controlClient.cpp

clean:
	rm -f *.o controlClient
