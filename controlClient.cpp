#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <pthread.h>

#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <netdb.h>
#include <time.h> 
#include <errno.h>

#include "Node.h"

using namespace std;

vector<Node*> nodes;

void passCommand(string command_, int source_, int destination_) {
	struct temp {
		string command;
		int source;
		int destination;
	} combined;
	
	combined.command = command_;
	combined.source = source_;
	combined.destination = destination_;
	
	struct sockaddr_in myAddr, remoteAddr;
	socklen_t remoteAddrLen = sizeof(remoteAddr);
	
	struct hostent *hostInfo;
	struct sockaddr_in servAddr;
	char msg[1024];
	
	//Create the socket for the Node
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Cannot create socket");
        exit(1);
        //printf("Socket created");
    }
    
    memset((char *)&myAddr, 0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(0);
    
    if (bind(sd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
		perror("binding failed");
		exit(1);
	} 
	
	memset((char*)&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(nodes.at(source_-1)->controlPort);
	
// THIS DOESN'T WORK
	hostInfo = gethostbyname(nodes.at(source_-1)->hostName.c_str());
	if(!hostInfo) {
		fprintf(stderr, "address of %s could not be obtained", nodes.at(source_-1)->hostName);
		exit(1);
	}
	
	char str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(hostInfo->h_addr_list[0]), str, INET_ADDRSTRLEN);
	
	cout << str << endl;
	
	//original
	//memcpy((void*)&servAddr.sin_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
	
	if (inet_aton(str, &servAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
//


//THIS WORKS
	char *server = "127.0.0.1";
	if (inet_aton(server, &servAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
//

	sprintf(msg, "This is packet 1");
	if(sendto(sd, msg, strlen(msg), 0, (struct sockaddr*)&servAddr, remoteAddrLen) == -1) {
		perror("message sending failed");
	}
	close(sd);
}

int main(int argc, char *argv[]) {
	string fileName = "input.txt";

	ifstream inputFile(fileName);
	
	string command = argv[1];
	int source = stoi(argv[2]);
	int destination = stoi(argv[3]);
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
	
	passCommand(command, source, destination);
}

