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

void passCommand(string command, int source, int destination) {

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
	servAddr.sin_port = htons(nodes.at(source-1)->controlPort);
	
/* THIS DOESN'T WORK
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
*/

// ----------------------------------------------

//get the IP address of servhost
	struct hostent *tempStruct;
	if ((tempStruct = gethostbyname(nodes.at(source-1)->hostName.c_str())) == NULL) {
		fprintf(stderr, "Error while getting host name\n");
		exit(1);
	}
	
	struct in_addr **ipAddress;
    ipAddress = (struct in_addr **) tempStruct->h_addr_list;
	
	servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*ipAddress[0]));
	
// ----------------------------------------------

/*	THIS WORKS
	char *server = "127.0.0.1";
	if (inet_aton(server, &servAddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
*/

	string temp = command + " " + to_string(destination);	
	strcpy(msg, temp.c_str());

	if(sendto(sd, msg, strlen(msg), 0, (struct sockaddr*)&servAddr, remoteAddrLen) == -1) {
		perror("message sending failed");
	}
	
	if(command != "generate-packet") {
		struct sockaddr_in servAddr2;
	
		memset((char*)&servAddr2, 0, sizeof(servAddr2));
		servAddr2.sin_family = AF_INET;
		servAddr2.sin_port = htons(nodes.at(destination-1)->controlPort);
		
		struct hostent *tempStruct2;
		if ((tempStruct2 = gethostbyname(nodes.at(destination-1)->hostName.c_str())) == NULL) {
			fprintf(stderr, "Error while getting host name\n");
			exit(1);
		}
	
		struct in_addr **ipAddress2;
		ipAddress2 = (struct in_addr **) tempStruct2->h_addr_list;
	
		servAddr2.sin_addr.s_addr = inet_addr(inet_ntoa(*ipAddress2[0]));
	
		string temp = command + " " + to_string(source);	
		strcpy(msg, temp.c_str());
	
		if(sendto(sd, msg, strlen(msg), 0, (struct sockaddr*)&servAddr2, remoteAddrLen) == -1) {
			perror("message sending failed");
		}
	}

/*	sprintf(msg, "This is packet 1");
	if(sendto(sd, msg, strlen(msg), 0, (struct sockaddr*)&servAddr, remoteAddrLen) == -1) {
		perror("message sending failed");
	}
*/
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

