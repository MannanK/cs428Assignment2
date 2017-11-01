#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "Node.h"

using namespace std;

Node :: Node() {
	nodeID = -1;
	hostName = "";
	controlPort = -1;
	dataPort = -1;
}

Node :: Node (int nodeID_, string hostname_, int controlPort_, int dataPort_) {
	nodeID = nodeID_;
	hostName = hostname_;
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
	cout << "HostName: " << hostName << endl;
	cout << "Control Port: " << controlPort << endl;
	cout << "Data Port: " << dataPort << endl;
	cout << "Neighbors: " << endl;
	int i;
	for(i=0; i < neighborInfo.size(); i++) {
		cout << "ID: " << neighborInfo.at(i)->nodeID;
		cout << ", Name: " << neighborInfo.at(i)->hostName;
		cout << ", Control: " << neighborInfo.at(i)->controlPort;
		cout << ", Data: " << neighborInfo.at(i)->dataPort << endl;
	}
	cout << endl;
	
	cout << "Routing Table: " << endl;
	for(i = 0; i < routingTable.size(); i++) {
		cout << "(" << routingTable.at(i).at(0) << ", " << routingTable.at(i).at(1) << ", " << routingTable.at(i).at(2) << ")" << endl;
	}
	cout << endl;
	
}
