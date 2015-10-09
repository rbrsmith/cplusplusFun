#include "stdafx.h"
#include "Server.h"

using namespace std;



void Server::start() {

	logger.log("\nNew Server instance started...\n");

	slen = sizeof(si_other);
	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		logger.log("Unable to start winsock\n");
		exit(EXIT_FAILURE);
	}
	
	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		logger.log("Unable to create socket\n");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		logger.log("Unable to bind\n");
		exit(EXIT_FAILURE);
	}
	logger.log("Server started and waiting...\n");
	//keep listening for data
	char serverBuf[BUFLEN];
	while (1)
	{
		struct message * msg = getDataFromClient(serverBuf);
		switch (msg->messageType) {
			case 0:
				// List operation
				logger.log("List operation called...\n");
				list(msg->sequenceBit);
				break;
			case 1:
				// Get operation
				logger.log("Get operation called...\n");
				get(msg);
				break;
			case 4:
				// Put operation
				logger.log("Put operation called...\n");
				put(msg);
				break;
		}
		logger.log("Waiting for request...\n");
	}	


}

int Server::handshake() {
	// Get SYN
	char buffer[BUFLEN];
	if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		logger.log("Error in recieving packet\n");
		exit(EXIT_FAILURE);
	}
	else {
		struct message *msg = (struct message *) buffer;
		logger.log("Received request from client with SYN: ");
		logger.log(msg->SYN);
		logger.log("\n");
		
		short syn = getRandomNumber();
		logger.log("Replying with ACK: ");
		logger.log(syn);
		logger.log("\n");
		char sendBuffer[BUFLEN];
		struct message *response = (struct message *) sendBuffer;
		response->SYN = syn;
		response->ACK = msg->SYN;
		Server::send(sendBuffer);
		char finalBuffer[BUFLEN];
		if ((recv_len = recvfrom(s, finalBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			logger.log("Eror receiving from client");
			exit(EXIT_FAILURE);
		}
		else {
			struct message *msg = (struct message *) finalBuffer;
			logger.log("Received ");
			logger.log(msg->ACK);
			logger.log("\n");
			if (msg->ACK != syn) {
				logger.log("ACK's dont align in handshake\n");
				exit(EXIT_FAILURE);
			}
			else {
				logger.log("Handshake successful!\n\n");
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
		logger.log("Sending error!");
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);
}


Server::message * Server::getDataFromClient(char *serverBuf) {
	seq = handshake();
	if ((recv_len = recvfrom(s, serverBuf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		logger.log("Failed to recieve data from client");
		exit(EXIT_FAILURE);
	}
	else {
		logger.log("Incoming data from client...\n");
		struct message *resmsg = (struct message *) serverBuf;
		return resmsg;
	}
	
}

void Server::list(int incomingSeq) {
	validateSequence(incomingSeq);
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
		logger.log("Error getting file listing\n");
		exit(EXIT_FAILURE);
	}

	// TODO we assume 1 packet size for list
	char sendBuffer[BUFLEN];
	struct message * sendMsg = (struct message *) sendBuffer;
	sendMsg->sequenceBit = seq;
	increaseSequence();
	if (output.length() >= BODYLEN) {
		logger.log("Unable to send file listing, listing is too large");
		sendMsg->errorBit = 1;
		send(sendBuffer);
		exit(EXIT_FAILURE);
	}
	strcpy_s(sendMsg->body, output.c_str());
	send(sendBuffer);
	logger.log("List of files end to client\n");
}

void Server::get(struct message * msg) {
	validateSequence(msg->sequenceBit);
	string filename = msg->body;


	logger.log("Looking for file...\n");

	ifstream fileToRead;
	fileToRead.open(filename, ios::in | ios::binary);
	if (fileToRead.is_open())
	{
		// Can make this a bit better
		// Count how many packets there will be
		char fakeBuf[BODYLEN];
		int numPackets = 0;
		while (!fileToRead.eof()) {
			memset(fakeBuf, '\0', BODYLEN);
			numPackets += 1;
			fileToRead.read(fakeBuf, BODYLEN);
		}

		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);
		int packetCount = 1;

		char buf[BUFLEN];
		
		while (!fileToRead.eof())
		{
			memset(buf, '\0', BODYLEN);


			struct message * sendMsg = (struct message *) buf;
			sendMsg->sequenceBit = seq;
			increaseSequence();

			/* Read the contents of file and write into the buffer for transmission */
			fileToRead.read(sendMsg->body, BODYLEN);

			if (packetCount == numPackets) {
				sendMsg->finalBit = 1;
			}
			else {
				sendMsg->finalBit = 0;
			}

			send(buf);
			packetCount += 1;
		}
		logger.log("File transfer complete\n");
	}
	else
	{
		char buf[BUFLEN];
		struct message * sendMsg = (struct message *) buf;
		sendMsg->sequenceBit = seq;
		increaseSequence();
		sendMsg->errorBit = 1;
		logger.log("Unable to find file\n");
		strcpy_s(sendMsg->body, "File was not found");
		send(buf);
	}
	fileToRead.close();
}

void Server::put(struct message * msg) {
	validateSequence(msg->sequenceBit);
	
	string filename = msg->body;

	ofstream outFile(filename, ios::out | ios::binary);

	// First package is filename
	char resBuffer[BUFLEN];
	while (1) {
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			logger.log("Error recieving data from client");
			exit(EXIT_FAILURE);
		}
		else {
			struct message *resMsg = (struct message *) resBuffer;
			validateSequence(resMsg->sequenceBit);
			
			outFile.write(resMsg->body, BODYLEN);
			if (resMsg->finalBit == 1) {
				logger.log("End of file transfer\n");
				break;
			}
		}
	}

}

int Server::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		logger.log("Sequence bits match up\n");
		increaseSequence();
		return 1;
	}
	else {
		logger.log("Sequence bits don't match\n");
		exit(EXIT_FAILURE);
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
