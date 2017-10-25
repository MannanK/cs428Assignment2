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
		string hostname;
		int controlPort;
		int dataPort;
		vector<int> neighbors;
		
		void addNeighbor(int neighborID);
		void removeNeighbor(int neighborID);
};
