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
