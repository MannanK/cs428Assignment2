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
#include <math.h>

#include "Node.h"

using namespace std;

//The Node for this process
Node* thisNode = new Node();

//Convert a number to an 8-bit binary string
string toBinary(int num) {
	string binary = "";
	for(int i = 7; i >= 0; i--) {
		if(num >= pow(2, i)) {
			binary.append("1");
			num = num - pow(2, i);
		}
		else {
			binary.append("0");
		}
	}
	return binary;
}

//Convert an 8-bit binary string to a number
int toInt(string binary) {
	int num =0;
	for(int i = 0; i < 8; i++) {
		if(binary[7-i] == '1') {
			num = num + pow(2, i);
		}
	}
	return num;
	
}

void updateTable() {
    //Reset routing table
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
    	if(thisNode->routingTable.at(i).at(0) == thisNode->nodeID) {
        	thisNode->routingTable.at(i).at(0) = i+1;
        	thisNode->routingTable.at(i).at(1) = -1;
        	thisNode->routingTable.at(i).at(2) = 0;
    	}
		else {
        	thisNode->routingTable.at(i).at(0) = i+1;
        	thisNode->routingTable.at(i).at(1) = -1;
        	thisNode->routingTable.at(i).at(2) = -1;
		}
    }
    
    //Ensure all neighbors now 1 hop away
    for(int i = 0; i < thisNode->neighborInfo.size(); i++) {
        thisNode->routingTable.at(thisNode->neighborInfo.at(i)->nodeID-1).at(1) = thisNode->neighborInfo.at(i)->nodeID;
        thisNode->routingTable.at(thisNode->neighborInfo.at(i)->nodeID-1).at(2) = 1;
    }
    
    thisNode->outputNode();
}

//Output the packet header info
void printPacket(string packet){
	cout << "Header" << endl;
	cout << "Source ID: " << toInt(packet.substr(0, 8)) << endl;
	cout << "Destination ID: " << toInt(packet.substr(8, 16)) << endl;
	cout << "Packet ID: " << toInt(packet.substr(16, 24)) << endl;
	cout << "TTL: " << toInt(packet.substr(24, 32)) << endl;
	
	cout << "Data" << endl;
	packet.erase(0, 32);
	for(int i = 0; i < packet.length(); i=i+8) {
		cout << toInt(packet.substr(i, i+8)) << " -> ";
	}
	
	cout << endl << endl;
}

//Recieve packet from a node and send it again
void sendPacket(string dataPacket) {
	//Update ttl
	int ttl = toInt(dataPacket.substr(24,32));
	ttl--;
	
	//Drop Packet if time to live expires
	if(ttl == 0) {
		cout << "Packet dropped" << endl;
	}
	
	else {
		//Replace ttl in packet
		dataPacket.replace(24, 8, toBinary(ttl));
		
		//Add self to data forwarding path
		dataPacket.append(toBinary(thisNode->nodeID));
		printPacket(dataPacket);
		
	}
	
}

 //generate-packet command, send packet to destination node
void generate(int destination) {
	string dataPacket = "";
	
	//Source Node ID
	dataPacket.append(toBinary(thisNode->nodeID));
	
	//Destination Node ID
	dataPacket.append(toBinary(destination));
	
	//PacketID
	thisNode->packetsSent ++;
	dataPacket.append(toBinary(thisNode->packetsSent));
	
	//TTL
	dataPacket.append(toBinary(15));
	
	//Add self to data forwarding path
	dataPacket.append(toBinary(thisNode->nodeID));
	
	printPacket(dataPacket);
	sendPacket(dataPacket);

}

//Add a link to a Node, need to fetch information about newly connected node by exchanging distance vectors
void createLink(int destination) {
	Node* newDest = new Node();
	newDest->nodeID = destination;
	thisNode->neighborInfo.push_back(newDest);
	
	thisNode->outputNode();
}

//Remove a link to a node, need to update the distance vectors
void removeLink(int destination) {
	for(int i=0; i < thisNode->neighborInfo.size(); i++) {
		if(destination == thisNode->neighborInfo.at(i)->nodeID) {
			thisNode->neighborInfo.erase(thisNode->neighborInfo.begin()+i);
			break;
		}
	}
	
	thisNode->outputNode();
	//update routing table
}

void *controlThread(void *dummy) {
	
	string command;
	int destination;
	
	struct sockaddr_in myAddr;
	struct sockaddr_in remoteAddr;
	socklen_t addrLen = sizeof(remoteAddr);
	int bytesReceived;
	char buffer[1024];
	
	fd_set rfds;
	
	//Create the socket for the Node
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }
    
    FD_ZERO(&rfds);
	FD_SET(sd, &rfds);
    
    memset((char*)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(thisNode->controlPort);
    
    //Bind socket to control port specified in input file and to all IP addresses so that it can listen to everyone
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
		        
		        string fullString = buffer;
		        istringstream iss(fullString);
		        string command;
		        int destination;
		        
		        iss >> command >> destination;
		        
		        if(command == "generate-packet") {
		        	struct sockaddr_in servAddr2;
		        	socklen_t remoteAddrLen = sizeof(servAddr2);
	
					memset((char*)&servAddr2, 0, sizeof(servAddr2));
					servAddr2.sin_family = AF_INET;
					servAddr2.sin_port = htons(thisNode->dataPort);
		
					struct hostent *tempStruct2;
					if ((tempStruct2 = gethostbyname(thisNode->hostName.c_str())) == NULL) {
						fprintf(stderr, "Error while getting host name\n");
						exit(1);
					}
	
					struct in_addr **ipAddress2;
					ipAddress2 = (struct in_addr **) tempStruct2->h_addr_list;
	
					servAddr2.sin_addr.s_addr = inet_addr(inet_ntoa(*ipAddress2[0]));
		        
		        	//send message to data port
		        	string temp = to_string(destination);	
					strcpy(buffer, temp.c_str());
		        	
		        	if(sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr*)&servAddr2, remoteAddrLen) == -1) {
						perror("message sending failed");
					}
		        }
		        
		        else if(command == "create-link") {
		        	createLink(destination);
		        }
		        
		        else if(command == "remove-link") {
		        	removeLink(destination);
		        }
		    }
		}
    }
}

void *dataThread(void *dummy) {
	
	int destination;
	
	struct sockaddr_in myAddr;
	struct sockaddr_in remoteAddr;
	socklen_t addrLen = sizeof(remoteAddr);
	int bytesReceived;
	char buffer[1024];
	
	fd_set rfds;
	
	//Create the socket for the Node
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }
    
    FD_ZERO(&rfds);
	FD_SET(sd, &rfds);
    
    memset((char*)&myAddr, 0, sizeof(myAddr));
    myAddr.sin_family = AF_INET;
    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myAddr.sin_port = htons(thisNode->dataPort);
    
    //Bind socket to control port specified in input file and to all IP addresses so that it can listen to everyone
    if(bind(sd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
        perror("bind failure");
        exit(1);
    }

    while(true) {
    	cout << "Waiting on port " << thisNode->dataPort << endl;
        
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
		        
		        string fullString = buffer;
		        istringstream iss(fullString);
		        int destination;
		        
		        iss >> destination;
		        
		        generate(destination);
		    }
		}
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
			    
			    thisNode->nodeID = nodeID;
                thisNode->hostName = hostName;
                thisNode->controlPort = controlPort;
                thisNode->dataPort = dataPort;
			
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
				        Node* neighbor = new Node();
				        neighbor->nodeID = stoi(token);
				        thisNode->neighborInfo.push_back(neighbor);
					    //thisNode->addNeighbor(stoi(token));
				    }
			    }
			    Node* neighbor = new Node();
			    neighbor->nodeID = stoi(newString);
			    thisNode->neighborInfo.push_back(neighbor);
			    //thisNode->addNeighbor(stoi(newString));
			    
            }
                
            //Routing table is a 2D Vector, initially sets next hop and distance to -1
			vector<int>data;
            data.push_back(counter);
            data.push_back(-1);
            data.push_back(-1);
			thisNode->routingTable.push_back(data);
			
			//Update counter
            counter++;
			
		}
		
		inputFile.close();
		
			
	} else cout << "\nUnable to open file." << endl;


	ifstream inputFile2(fileName);
	if(inputFile2.is_open()) {
	
	    string newString;
	    int nodeID, controlPort, dataPort;
	    string hostName;
	
	    int counter = 1;
	    //Read line by line
	    while (getline(inputFile2, newString)) {

            //If line matches one of the neighbors, retrieve that neighbors info and save it
	        for(int i = 0; i < thisNode->neighborInfo.size(); i++) {
	            if(counter == thisNode->neighborInfo.at(i)->nodeID) {
	                
					std::istringstream ss1;
			        ss1.str(newString);
			        ss1 >> nodeID >> hostName >> controlPort >> dataPort;
			        
			        thisNode->neighborInfo.at(i)->hostName = hostName;
			        thisNode->neighborInfo.at(i)->controlPort = controlPort;
			        thisNode->neighborInfo.at(i)->dataPort = dataPort;
	                break;
	            }         
	        }
	        counter++;
	    }
	}
	
	thisNode -> outputNode();
	updateTable();
	
    //Create threads for the data and  control ports of the node
    int i=0;
	pthread_t myThread;
	pthread_create(&myThread, NULL, controlThread, &i);
	pthread_create(&myThread, NULL, dataThread, &i);	
	
	pthread_join(myThread, NULL);
}
