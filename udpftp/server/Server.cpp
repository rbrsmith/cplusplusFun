#include "stdafx.h"
#include "Server.h"

using namespace std;

void Server::start() {
	slen = sizeof(si_other);

	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	
	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	cout << "Server started and waiting...\n";
	//keep listening for data
	char serverBuf[BUFLEN];
	while (1)
	{
		struct message * msg = getDataFromClient(serverBuf);
		switch (msg->messageType) {
			case 0:
				// List operation
				cout << "List operation called...\n";
				list(msg->sequenceBit);
				break;
		}
	}	


}

int Server::handshake() {
	// Get SYN
	char buffer[BUFLEN];
	if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	else {
		struct message *msg = (struct message *) buffer;
		cout << "Received request from client with SYN: " << msg->SYN << "\n";
		short syn = getRandomNumber();
		cout << "Replying with ACK: " << syn << "\n";
		char sendBuffer[BUFLEN];
		struct message *response = (struct message *) sendBuffer;
		response->SYN = syn;
		response->ACK = msg->SYN;
		Server::send(sendBuffer);
		char finalBuffer[BUFLEN];
		if ((recv_len = recvfrom(s, finalBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		else {
			struct message *msg = (struct message *) finalBuffer;
			cout << "Received " << msg->ACK << " from client\n";
			if (msg->ACK != syn) {
				cout << "ACK's don't align in handshake\n";
				exit(EXIT_FAILURE);
			}
			else {
				cout << "Handshake Successfull!";
				// Extract least significant bit
				int bit = syn & (1 << 0);
				return bit;
			}
		}
	}
}

void Server::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);
}


Server::message * Server::getDataFromClient(char *serverBuf) {
	seq = handshake();
	if ((recv_len = recvfrom(s, serverBuf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	else {
		cout << "Incoming packet from client...\n";
		struct message *resmsg = (struct message *) serverBuf;
		return resmsg;
	}
	
}

void Server::list(int incomingSeq) {
	if (validateSequence(incomingSeq) != 1) {
		cout << "Error in sequence values\n";
		exit(EXIT_FAILURE);
	}
	else {
		string output = "";
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir(".")) != NULL) {
			/* print all the files and directories within directory */
			while ((ent = readdir(dir)) != NULL) {
				string fname = ent->d_name;
				if (fname.compare("..") == 0 || fname.compare(".") == 0) {
					continue;
				}
				output += ent->d_name;
				output += "\n";
			}
			closedir(dir);
		}
		else {
			/* could not open directory */
			perror("Error getting file listing\n");
			exit(EXIT_FAILURE);
		}

		// TODO we assume 1 packet size for list
		char sendBuffer[BUFLEN];
		struct message * sendMsg = (struct message *) sendBuffer;
		sendMsg->sequenceBit = seq;
		increaseSequence();
		strcpy_s(sendMsg->body, output.c_str());
		send(sendBuffer);
	}

}

int Server::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		increaseSequence();
		return 1;
	}
	else {
		return 0;
	}
}

int Server::getRandomNumber() {
	srand((unsigned)time(0));
	int randNum = (rand() % 300) + 150;
	return randNum;
}

void Server::increaseSequence() {
	if (seq == 1) {
		seq = 0;
	}
	else {
		seq = 1;
	}
}