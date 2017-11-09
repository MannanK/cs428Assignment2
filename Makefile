all:	clean controlClient	nodeCreator

controlClient:	controlClient.o
	g++ -pthread -std=c++11 Node.o controlClient.o -o controlClient

controlClient.o:	controlClient.cpp
	g++ -std=c++11 -c Node.cpp controlClient.cpp
	
nodeCreator: nodeCreator.o
	g++ -pthread -std=c++11 Node.o nodeCreator.o -o nodeCreator
	
nodeCreator.o:	nodeCreator.cpp
	g++ -std=c++11 -c Node.cpp nodeCreator.cpp

clean:
	rm -f *.o controlClient nodeCreator
