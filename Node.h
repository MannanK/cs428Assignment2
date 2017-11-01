#include <iostream>
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;

class Node {
	public:
		Node();
		Node(int nodeID_, string hostname_, int controlPort_, int dataPort_);
		int nodeID;
		string hostName;
		int controlPort;
		int dataPort;
		vector<int> neighbors;
		
		vector<Node*> neighborInfo;
		vector<vector<int>> routingTable;
		
		
		void addNeighbor(int neighborID);
		void removeNeighbor(int neighborID);
		void outputNode();
};
