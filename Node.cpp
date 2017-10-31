#include <iostream>
#include <stdlib.h>
#include <string>
#include "Node.h"

using namespace std;

Node :: Node() {
	nodeID = -1;
	hostname = "";
	controlPort = -1;
	dataPort = -1;
}

Node :: Node (int nodeID_, string hostname_, int controlPort_, int dataPort_) {
	nodeID = nodeID_;
	hostname = hostname_;
	controlPort = controlPort_;
	dataPort = dataPort_;
}

void Node :: addNeighbor(int neighborID) {
	neighbors.push_back(neighborID);
}

//Remove a neighbor from the neighbor vector
void Node :: removeNeighbor(int neighborID) {
	int i;
	int index = -1;
	for(i=0; i < neighbors.size(); i++) {
		if(neighbors.at(i) == neighborID) {
			index = i;
			break;
		}
	}
	
	if(index != -1) {
		neighbors.erase(neighbors.begin()+index);
	}
}

//Simple output for Node
void Node :: outputNode() {
	cout << "Node ID: " << nodeID << endl;
	cout << "HostName: " << hostname << endl;
	cout << "Control Port: " << controlPort << endl;
	cout << "Data Port: " << dataPort << endl;
	cout << "Neighbors: ";
	int i;
	for(i=0; i < neighbors.size(); i++) {
		cout << neighbors.at(i) << ", ";
	}
	cout << endl << endl;
}
