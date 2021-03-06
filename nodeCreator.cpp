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

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;

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

//Convert this Node's routing table to a string to be sent to other nodes
string toRoutingString() {
    string myString = "";
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
        for(int j = 0; j < 3; j++) {
            if(thisNode->routingTable.at(i).at(j) == -1) {
                myString.append("--------");
            }
            else {
                myString.append(toBinary(thisNode->routingTable.at(i).at(j)));
            }
        }
    }

    return myString;
}

//Convert the routing string to a 2D vector representing the routingTable
vector <vector<int>> toRoutingVector(string routingString) {
    vector <vector<int>> routingTable;
    int counter = 0;
    int i = 0;
    
    while(i < routingString.length()) {
        vector<int> temp;
        
        for(int j = 0; j < 3; j++) {
            if(routingString.substr(i, 8) == "--------") {
                temp.push_back(-1);
            }
            else {
                temp.push_back(toInt(routingString.substr(i, 8)));
            }
            i = i+8;
        }
        
        routingTable.push_back(temp);
        counter++;
    }    
    
	return routingTable;
}

//Output the packet info
void printPacket(string packet){
    cout << "-------------PACKET RECEIVED-------------" << endl;
	cout << "Header" << endl;
	cout << "Source ID: " << toInt(packet.substr(0, 8)) << endl;
	cout << "Destination ID: " << toInt(packet.substr(8, 8)) << endl;
	cout << "Packet ID: " << toInt(packet.substr(16, 8)) << endl;
	cout << "TTL: " << toInt(packet.substr(24, 8)) << endl;
	
	cout << "Data" << endl;
	cout << toInt(packet.substr(32, 8));
	packet.erase(0, 40);
	for(int i = 0; i < packet.length(); i=i+8) {
		cout << " -> " << toInt(packet.substr(i, 8));
	}
	
	cout << endl << "-----------------------------------------" << endl << endl;
}

//Initial version of a Node's table. Adds info about neighbors, but keeps everything else blank
void createTable() {
    //Reset routing table
    //Initially creates the routing table with information about this node's neighbors
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
    
}

//Send this node's routing table to all of its neighbors
void sendTable() {
    pthread_mutex_lock( &mutex1 );
    char buffer[1024]; 

    struct sockaddr_in myAddr;
    
    //Create the socket for the Node
    int sd;
    if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Cannot create socket");
        exit(1);
    }
    
    memset((char *)&myAddr, 0, sizeof(myAddr));
	myAddr.sin_family = AF_INET;
	myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myAddr.sin_port = htons(0);
    
    if (bind(sd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
		perror("binding failed");
		exit(1);
	}
	
	//Loop through the neighbors vector, and using the respective port and host name info, send own routing table to their control port
    for(int i=0; i < thisNode->neighborInfo.size(); i++) {
        struct sockaddr_in servAddr;
        socklen_t remoteAddrLen = sizeof(servAddr);

        memset((char*)&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(thisNode->neighborInfo.at(i)->controlPort);

        struct hostent *tempStruct;
        if ((tempStruct = gethostbyname(thisNode->neighborInfo.at(i)->hostName.c_str())) == NULL) {
	        fprintf(stderr, "Error while getting host name\n");
	        exit(1);
        }

        struct in_addr **ipAddress;
        ipAddress = (struct in_addr **) tempStruct->h_addr_list;

        servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*ipAddress[0]));

        //send message to control port
        string temp = "table " + toRoutingString();
        strcpy(buffer, temp.c_str());

        if(sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr*)&servAddr, remoteAddrLen) == -1) {
	        perror("message sending failed");
        }
    }
    
    close(sd);
    pthread_mutex_unlock( &mutex1 );
}

//Update our current routing table based on the routing table we just received
void updateTable(vector<vector<int>> routingTable) {
    pthread_mutex_lock( &mutex2 );
    
    //Keep record of routing table before update, to compare later to determine if table should be printed
    vector<vector<int>> oldTable = thisNode->routingTable;
    
    //Determine where the routing table came from
    int sourceNode;
    for(sourceNode = 1; sourceNode <= routingTable.size(); sourceNode++) {
    	if(routingTable.at(sourceNode-1).at(2) == 0) {
            break;
        }
    }
    
    //Ensures that all nodes currently 1 hop away in our routing table are still actually neighbors
    //If not, invalidates the distance vector entry, indicating a new path has to be found
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
    	if(thisNode->routingTable.at(i).at(2) == 1) {
    		int found = 0;
    		for(int j = 0; j < thisNode->neighborInfo.size(); j++) {
    			if(thisNode->neighborInfo.at(j)->nodeID == i+1) {
    				found = 1;
    				break;
    			}
    		}
    		if(found==0) {
    		    thisNode->routingTable.at(i).at(1) = -1;
            	thisNode->routingTable.at(i).at(2) = -1; 
    		}
    	}
    }
    
	//If any node in our routing table > 1 hop away, check the routing table of that nextHop, and make sure the distance is correct
	//Basically fact checks the distances
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
    	if(thisNode->routingTable.at(i).at(1) == sourceNode && thisNode->routingTable.at(i).at(2) > 1) {
    		if(routingTable.at(i).at(2) != thisNode->routingTable.at(i).at(2)-1) {
    			thisNode->routingTable.at(i).at(1) = -1;
            	thisNode->routingTable.at(i).at(2) = -1; 
    		}
    	}
    }

    //Loop through the received routing table
    for(int i = 0; i < routingTable.size(); i++) {
	    //If the distance to a node in the current routing table is greater than that in the received routing table, or if the distance is unknown, update your table
	    //If the received routing table has a distance to a node of -1 never update
	    if((thisNode->routingTable.at(i).at(2) > routingTable.at(i).at(2) || thisNode->routingTable.at(i).at(2) == -1) && routingTable.at(i).at(2) > -1) {
	        //Use this to determine the node that passed the distance vector for next hop
	        
	        if(thisNode->routingTable.at(i).at(2) != routingTable.at(i).at(2) + 1) {
	         	//Update info
	        	thisNode->routingTable.at(i).at(1) = sourceNode;
	        	thisNode->routingTable.at(i).at(2) = routingTable.at(i).at(2) + 1; 
	        }
	     
	    }

    }
    
    //If distance to a node exceeds the number of nodes, the node is unreachable, set to -1
    for(int i = 0; i < routingTable.size(); i++) {
    	if(thisNode->routingTable.at(i).at(2) > thisNode->routingTable.size()) {
			thisNode->routingTable.at(i).at(1) = -1;
	      	thisNode->routingTable.at(i).at(2) = -1;
		}
	}
	
	//compare the routing table to how it was prior to the update
	//If it changed, print it
    int same = 1;
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
    	if(thisNode->routingTable.at(i).at(1) != oldTable.at(i).at(1) || thisNode->routingTable.at(i).at(2) != oldTable.at(i).at(2)) {
    		same = 0;
    		break;
		}
    
    }
    if(same == 0) {
    	cout << "----------ROUTING TABLE UPDATED----------"<< endl;
    	for(int i = 0; i < thisNode->routingTable.size(); i++) {
			cout << "(" << thisNode->routingTable.at(i).at(0) << ", " << thisNode->routingTable.at(i).at(1) << ", " << thisNode->routingTable.at(i).at(2) << ")" << endl;
		}
		cout << "-----------------------------------------"<< endl;
    }

    pthread_mutex_unlock( &mutex2 );
}

//Recieve packet from a node and send it again
void sendPacket(string dataPacket) {
	//Update time to live (ttl)
	int ttl = toInt(dataPacket.substr(24,8));
	
	int properLength = 32 + 8*(15-ttl);
	dataPacket.erase(properLength, dataPacket.length());
	
	ttl--;
	
	//Drop packet if time to live expires (meaning either a cycle or route to get to node is too long)
	if(ttl == 0) {
		cout << "Packet dropped" << endl;
	}
	
	else {
		//Replace ttl in packet
		dataPacket.replace(24, 8, toBinary(ttl));
		//Add self to data forwarding path
		dataPacket.append(toBinary(thisNode->nodeID));
		
		int destination = toInt(dataPacket.substr(8, 8));
		int nextHop = thisNode->routingTable.at(destination-1).at(1);
		
		//Don't know any way to get to the node, so drop the packet
		if(nextHop == -1) {
			cout << "Packet dropped" << endl;
			return;
		}
		
		//Parse the port and host name info for the node we want to send to
		int nextPort;
		string nextHost;
		for(int i = 0; i < thisNode->neighborInfo.size(); i++) {
		    if(nextHop == thisNode->neighborInfo.at(i)->nodeID) {
		        nextPort = thisNode->neighborInfo.at(i)->dataPort;
		        nextHost = thisNode->neighborInfo.at(i)->hostName;
		        break;
		    }
		}
		
		cout << "Node " << thisNode->nodeID << " forwarding to " << nextHop << " to get to node " << destination << "." << endl << endl;
        
		char buffer[1024];
        struct sockaddr_in myAddr;
    
        //Create the socket for the Node
        int sd;
        if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Cannot create socket");
            exit(1);
        }
        
        memset((char *)&myAddr, 0, sizeof(myAddr));
	    myAddr.sin_family = AF_INET;
	    myAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    myAddr.sin_port = htons(0);
        
        if (bind(sd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
		    perror("binding failed");
		    exit(1);
	    }
		
		struct sockaddr_in servAddr;
        socklen_t remoteAddrLen = sizeof(servAddr);

        memset((char*)&servAddr, 0, sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_port = htons(nextPort);

        struct hostent *tempStruct;
        if ((tempStruct = gethostbyname(nextHost.c_str())) == NULL) {
	        fprintf(stderr, "Error while getting host name\n");
	        exit(1);
        }

        struct in_addr **ipAddress;
        ipAddress = (struct in_addr **) tempStruct->h_addr_list;

        servAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*ipAddress[0]));

        //send message to data port
        strcpy(buffer, dataPacket.c_str());

        if(sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr*)&servAddr, remoteAddrLen) == -1) {
	        perror("message sending failed");
        }
        
        close(sd);
	}
}

 //generate-packet command, send packet to destination node's data port
void generate(int destination) {

    cout << "Generating packet in source " << thisNode->nodeID << endl;
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
	
	sendPacket(dataPacket);
}

//Read the input file to get new information about a neighbor that was added in createLink
void readNewNeighbor(int destination) {
	ifstream inputFile("input.txt");
	if(inputFile.is_open()) {
	    string newString;
	    int nodeID, controlPort, dataPort;
	    string hostName;

	    //Read line by line
	    while (getline(inputFile, newString)) {
	    	std::istringstream ss;
		    ss.str(newString);
		    ss >> nodeID >> hostName >> controlPort >> dataPort;
	    
            if(nodeID == destination) {
            	Node* temp = new Node();
            	temp->nodeID = nodeID;
            	temp->hostName = hostName;
		        temp->controlPort = controlPort;
		        temp->dataPort = dataPort;
            	thisNode->neighborInfo.push_back(temp);
                break;
            }         
	    }
	}
}

//Add a link to a Node
void createLink(int destination) {
	pthread_mutex_lock(&mutex2);
	
	//Update routing table, change distance to this node to be 1 since it is now a neighbor
	thisNode->routingTable.at(destination-1).at(1) = destination;
	thisNode->routingTable.at(destination-1).at(2) = 1;
	
	readNewNeighbor(destination);
	
	//sleep(1);
	
	cout << endl << "CREATING LINK BETWEEN " << thisNode->nodeID << " AND " << destination << endl << endl;
	
	cout << "----------ROUTING TABLE UPDATED----------"<< endl;
    	for(int i = 0; i < thisNode->routingTable.size(); i++) {
			cout << "(" << thisNode->routingTable.at(i).at(0) << ", " << thisNode->routingTable.at(i).at(1) << ", " << thisNode->routingTable.at(i).at(2) << ")" << endl;
		}
	cout << "-----------------------------------------"<< endl;
	
	pthread_mutex_unlock(&mutex2);
}

//Remove a link to a node, need to update the distance vectors
void removeLink(int destination) {
	pthread_mutex_lock(&mutex2);
	
	//Remove this node from our own neighbors list
	for(int i=0; i < thisNode->neighborInfo.size(); i++) {
		if(destination == thisNode->neighborInfo.at(i)->nodeID) {
			thisNode->neighborInfo.erase(thisNode->neighborInfo.begin()+i);
			break;
		}
	}
	
	//Update routing table, nullify the link using -1, should get properly updated by next update of routing table
	thisNode->routingTable.at(destination-1).at(1) = -1;
	thisNode->routingTable.at(destination-1).at(2) = -1;
	
	//Ensure any node in our own routing table that uses this node as a nextHop is nullified, indicating a new path must be found
	for(int i = 0; i < thisNode->routingTable.size(); i++) {
		if(thisNode->routingTable.at(i).at(1) == destination) {
			thisNode->routingTable.at(i).at(1) = -1;
			thisNode->routingTable.at(i).at(2) = -1;
		}
	}
	
	sleep(1);
	
	cout << endl << "REMOVING LINK BETWEEN " << thisNode->nodeID << " AND " << destination << endl << endl;
	
	cout << "----------ROUTING TABLE UPDATED----------"<< endl;
    for(int i = 0; i < thisNode->routingTable.size(); i++) {
		cout << "(" << thisNode->routingTable.at(i).at(0) << ", " << thisNode->routingTable.at(i).at(1) << ", " << thisNode->routingTable.at(i).at(2) << ")" << endl;
	}
	cout << "-----------------------------------------"<< endl;
	pthread_mutex_unlock(&mutex2);
}

//Function that is continuously listening for messages from the control client, or vectors from other nodes
//Forwards the commands to respective functions
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
    
    int counter = 0;
    while(true) {
        fd_set tempfdset;
		FD_ZERO(&tempfdset);
		tempfdset = rfds;
		
		struct timeval tv;
		tv.tv_sec = 1;
		
		if (select(sd+1, &tempfdset, NULL, NULL, &tv) == -1) {
            perror("select: ");
            exit(1);
        }
        
        //Ensures routing table is sent every 5 seconds to all neighbors
        if(counter == 5) {
            counter=0;
            sendTable();
        } 
        
        
        if (FD_ISSET(sd, &tempfdset)) {
			bytesReceived = recvfrom(sd, buffer, 1024, 0, (struct sockaddr*)&remoteAddr, &addrLen);
			
		    if(bytesReceived > 0) {
		        string fullString = buffer;
		        istringstream iss(fullString);
		        
		        string command;
		        iss >> command;

				//Received message was a routing table from another node, calls updateTable() to update our own routing table
		        if(command == "table") {
		        	string routingString;
		        	iss >> routingString;
		        	
		        	updateTable(toRoutingVector(routingString));
		        }
		        
		        //Received message was a command from the control client
		        else {
		        	int destination;
				    iss >> destination;
				    
				    //send message to the data port, telling it to generate a packet to the destination nodeID
				    //this goes to *dataThread()
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
				    
				    	//send message to our own data port
				    	string temp = "initial " + to_string(destination) + " ";	
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
		
		counter++;
    }
    
    close(sd);
}

//Function that is continuously listening for messages from the controlThread(), telling us to generate and send a new data packet
//Forwards the commands to respective functions
void *dataThread(void *dummy) {
	int destination;
	
	struct sockaddr_in myAddr;
	struct sockaddr_in remoteAddr;
	socklen_t addrLen = sizeof(remoteAddr);
	int bytesReceived;
	
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
        char buffer[1024];
        
        fd_set tempfdset;
		FD_ZERO(&tempfdset);
		tempfdset = rfds;
		
		if (select(sd+1, &tempfdset, NULL, NULL, NULL) == -1) {
            perror("select: ");
            exit(1);
        }
        
        if (FD_ISSET(sd, &tempfdset)) {
			bytesReceived = recvfrom(sd, buffer, 1024, 0, (struct sockaddr*)&remoteAddr, &addrLen);
			
		    if(bytesReceived > 0) {
		        string fullString = buffer;
		        
		        //Received a data packet from another node's data port
		        if(fullString[0] != 'i') {
		        	int destination = toInt(fullString.substr(8,8));
		        	
		        	//this is the node that should receive the packets
		        	if(destination == thisNode->nodeID) {
	        	    	int ttl = toInt(fullString.substr(24,8));
                        int properLength = 32 + 8*(15-ttl);
                        fullString.erase(properLength, fullString.length());
                        fullString.append(toBinary(destination));
                        
		        	    printPacket(fullString);
		        	}
		        	
		        	//we need to forward to some intermediate node to get to the final destination
		        	else {
		        	    sendPacket(fullString);
		        	}
		        }
		        
		        //Received command from control thread to generate a new packet to the given destination
		        else {
		        	istringstream iss(fullString);
		        	string command;
		        	int destination;
		        	
		        	iss >> command >> destination;
		        	
				    generate(destination);
                }
		    }
		}
    }
    
    close(sd);
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
				
				//Skip the first 4 sections of the line since they are nodeID, host name, etc.
				//Then start adding neighbors to neighbors list
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
				    }
			    }
			    
			    //Make sure the node has at least one neighbor
			    if(pos != string::npos) {
			    	Node* neighbor = new Node();
			    	neighbor->nodeID = stoi(newString);
			    	thisNode->neighborInfo.push_back(neighbor);
			    }
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

	//Reread the file to get information about the neighbors, such as their host name and ports
	ifstream inputFile2(fileName);
	
	if(inputFile2.is_open()) {
	    string newString;
	    int nodeID, controlPort, dataPort;
	    string hostName;
	
	    int counter = 1;

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
	
	//Create our initial routing table
	createTable();
	
	cout << "----------------NODE INFO----------------" << endl;
	thisNode->outputNode();
	cout << "-----------------------------------------" << endl << endl;
	
    //Create threads for the data and control ports of the node
    int i=0;
	pthread_t myThread;
	pthread_create(&myThread, NULL, controlThread, &i);
	pthread_create(&myThread, NULL, dataThread, &i);	
	
	pthread_join(myThread, NULL);
}

