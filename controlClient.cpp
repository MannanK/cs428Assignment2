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

void *controlThread(void *aThing) {
	int index = *((int *) aThing);
	nodes.at(index)->outputNode();
}

void *dataThread(void *aThing) {
	int index = *((int *) aThing);
	nodes.at(index)->outputNode();
}

int main(int argc, char *argv[]) {
	string fileName = "input.txt";

	ifstream inputFile(fileName);
	
	if (inputFile.is_open()) {
		string newString;
		int nodeID, controlPort, dataPort;
		string hostName;
		
		while (getline(inputFile, newString)) {
			std::istringstream ss1;
			ss1.str(newString);
			ss1 >> nodeID >> hostName >> controlPort >> dataPort;
			
			Node* tempNode = new Node(nodeID, hostName, controlPort, dataPort);
			
			string delimiter = "\t";
			size_t pos = 0;
			string token;
			int count = 0;
			
			while ((pos = newString.find(delimiter)) != string::npos) {
				token = newString.substr(0, pos);
				newString.erase(0, pos + delimiter.length());
			
				if(count < 4) {
					count++;
				}
				
				else {
					tempNode->addNeighbor(stoi(token));
				}
			}
			
			tempNode->addNeighbor(stoi(newString));
			
			nodes.push_back(tempNode);
		}
		
		inputFile.close();
			
	} else cout << "\nUnable to open file." << endl;
	
	int i=0;
	pthread_t myThread;
	for(i = 0; i < nodes.size(); i++) {
		pthread_create(&myThread, NULL, controlThread, &i);
		pthread_join(myThread, NULL);
		pthread_create(&myThread, NULL, dataThread, &i);
		pthread_join(myThread, NULL);		
	}
	cout << endl;
	
	pthread_join(myThread, NULL);
	cout << "I am also here" << endl;
	
	while(true) {}
	
}

