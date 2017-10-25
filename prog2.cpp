#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include "Node.h"

using namespace std;

vector<Node*> nodes;

void *testThread(void *aThing) {
	cout << (int*)aThing << endl;
}

int main(int argc, char *argv[]) {
	string fileName = "input.txt";

	ifstream inputFile(fileName);
	
	if (inputFile.is_open()) {
		string newString;
		int nodeID, controlPort, dataPort;
		string hostName, neighborsString;
		
		while (getline(inputFile, newString)) {
			std::istringstream ss1;
			ss1.str(newString);
			ss1 >> nodeID >> hostName >> controlPort >> dataPort >> neighborsString;
			
			//cout << neighborsString << endl;
			std::istringstream ss2;
			ss2.str(neighborsString);
			int temp;
			
			Node* tempNode = new Node(nodeID, hostName, controlPort, dataPort);
			
			while(ss2 >> temp) {
				tempNode->addNeighbor(temp);
			}
			
			nodes.push_back(tempNode);
			
			//tempNode->outputNode();
		}
		
		inputFile.close();
			
	} else cout << "\nUnable to open file." << endl;
	
	int i=0;
	pthread_t myThread;
	for(i = 0; i < nodes.size(); i++) {
		pthread_create(&myThread, NULL, testThread, &i);		
	}
	pthread_join(myThread, NULL);
	cout << "I am also here" << endl;
	
}

