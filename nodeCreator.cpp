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

//The Node for this process
Node* thisNode;

//generate-packet command, send packet to destination node
void generate(int destination) {

}

void addLink(int destination) {
	thisNode->addNeighbor(destination);
}

void removeLink(int destination) {
	thisNode->removeNeighbor(destination);
}

void *controlThread(void *dummy) {

	//Output the node
	thisNode->outputNode();
	
	struct sockaddr_in myAddr;
	struct sockaddr_in remoteAddr;
	socklen_t addrLen = sizeof(remoteAddr);
	int bytesReceived;
	unsigned char buffer[1024];
	
	fd_set rfds;
	
	//Create the socket for the Node
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }
    
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
	FD_SET(sd, &rfds);
    
    memset((char*)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(thisNode->controlPort);
    
    //Bind socket to control port specified in input file
    if(bind(sd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
        perror("bind failure");
        exit(1);
    }
    
    while(true) {
        cout << "Waiting on port " << thisNode->controlPort << endl;
        
        fd_set tempfdset;
		FD_ZERO(&tempfdset);
		tempfdset = rfds;
		
		if (select(sd+1, &tempfdset, NULL, NULL, NULL) == -1) {
            perror("select: ");
            exit(1);
        }
        
        if (FD_ISSET(sd, &tempfdset)) {
			bytesReceived = recvfrom(sd, buffer, 1024, 0, (struct sockaddr*)&remoteAddr, &addrLen);
		    cout << "Received " << bytesReceived << " bytes." << endl;
		    if(bytesReceived > 0) {
		        buffer[bytesReceived] = 0;
		        cout << "Message: " << buffer << endl;
		    }
		}
    }
}

void *dataThread(void *dummy) {



    while(true) {
    
    }
}

int main(int argc, char* argv[]) {

    //Hardcoded file name
    string fileName = "input.txt";
    ifstream inputFile(fileName);
    
    //Take in node number
    int nodeNum = stoi(argv[1], NULL);
    
    //Only read the line of the file relevant to that specific node
    if (inputFile.is_open()) {
		string newString;
		int nodeID, controlPort, dataPort;
		string hostName;
		
		int counter = 1;
		while (getline(inputFile, newString)) {
		    if(counter == nodeNum){
			    std::istringstream ss1;
			    ss1.str(newString);
			    ss1 >> nodeID >> hostName >> controlPort >> dataPort;
			
			    thisNode = new Node(nodeID, hostName, controlPort, dataPort);
			
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
					    thisNode->addNeighbor(stoi(token));
				    }
			    }
			
			    thisNode->addNeighbor(stoi(newString));
			    break;
            }
            else{counter++;}
			
		}
		
		inputFile.close();
			
	} else cout << "\nUnable to open file." << endl;

    //Create threads for the data and  control ports of the node
    int i=0;
	pthread_t myThread;
	pthread_create(&myThread, NULL, controlThread, &i);
	//pthread_join(myThread, NULL);
	pthread_create(&myThread, NULL, dataThread, &i);
	//pthread_join(myThread, NULL);		
	
	pthread_join(myThread, NULL);
}
