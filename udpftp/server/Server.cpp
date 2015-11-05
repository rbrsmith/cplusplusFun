#include "stdafx.h"
#include "Server.h"

using namespace std;



void Server::start() {

	logger.log("\nServer:  New Server instance started...\n");
	

	slen = sizeof(si_other);
	//Initialise winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		cout << "Error in winsock";
		logger.log("Server:  Unable to start winsock\n");
		exit(EXIT_FAILURE);
	}
	
	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		cout << "Error in creating socket";
		logger.log("Server:  Unable to create socket\n");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(s, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR)
	{
		cout << "Error in binding";
		logger.log("Server:  Unable to bind\n");
		exit(EXIT_FAILURE);
	}
	logger.log("Server:  Server started and waiting...\n");
	cout << "Server ready and waiting...\n";
	//keep listening for data
	char serverBuf[BUFLEN];
	while (1)
	{
		struct message * msg = getDataFromClient(serverBuf);
		switch (msg->messageType) {
			case 0:
				// List operation
				logger.log("Server:  List operation called...\n");
				cout << "LIST\n";
				list(msg->sequenceBit);
				break;
			case 1:
				// Get operation
				logger.log("Server:  Get operation called...\n");
				cout << "GET\n";
				get(msg);
				break;
			case 4:
				// Put operation
				logger.log("Server:  Put operation called...\n");
				cout << "PUT\n";
				put(msg);
				break;
		}
		logger.log("Server:  Server:  Waiting for request...\n");
	}	


}

int Server::handshake() {
	// Get SYN
	char buffer[BUFLEN];
	if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		logger.log("Server:  Error in recieving packet\n");
		exit(EXIT_FAILURE);
	}
	else {
		struct message *msg = (struct message *) buffer;
		logger.log("Server:  Received request from client with SYN: ");
		logger.log(msg->SYN);
		logger.log("\n");
		
		short syn = getRandomNumber();
		logger.log("Server:  Replying with ACK: ");
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
			logger.log("Server:  Eror receiving from client");
			exit(EXIT_FAILURE);
		}
		else {
			struct message *msg = (struct message *) finalBuffer;
			logger.log("Server:  Received ");
			logger.log(msg->ACK);
			logger.log(" from client\n");
			if (msg->ACK != syn) {
				logger.log("Server:  ACK's dont align in handshake\n");
				exit(EXIT_FAILURE);
			}
			else {
				// Extract least significant bit
				int bit = syn & (1 << 0);
				logger.log("Server:  Handshake successful!\n");
				logger.log("Server:  Setting sequence bit to ");
				logger.log(bit);
				logger.log("\n");
				return bit;
			}
		}
	}
}

void Server::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		logger.log("Server:  Sending error!");
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);
}


Server::message * Server::getDataFromClient(char *serverBuf) {
	seq = handshake();
	if ((recv_len = recvfrom(s, serverBuf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		logger.log("Server:  Failed to recieve data from client");
		exit(EXIT_FAILURE);
	}
	else {
		logger.log("Server:  Incoming data from client...\n");
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
		logger.log("Server:  Error getting file listing\n");
		exit(EXIT_FAILURE);
	}

	// TODO we assume 1 packet size for list
	char sendBuffer[BUFLEN];
	struct message * sendMsg = (struct message *) sendBuffer;
	sendMsg->sequenceBit = seq;
	increaseSequence();
	if (output.length() >= BODYLEN) {
		logger.log("Server:  Unable to send file listing, listing is too large");
		sendMsg->errorBit = 1;
		send(sendBuffer);
		exit(EXIT_FAILURE);
	}
	strcpy_s(sendMsg->body, output.c_str());
	send(sendBuffer);
	logger.log("Server:  List of files send to client\n");
}

void Server::get(struct message * msg) {
	validateSequence(msg->sequenceBit);
	string filename = msg->body;


	logger.log("Server:  Looking for file: ");
	logger.log("Filename: ");
	char why[BODYLEN];
	strcpy_s(why, filename.c_str());
	logger.log(why);
	logger.log("\n");

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
		logger.log("Server:  Sending ");
		logger.log(numPackets);
		logger.log(" packets to server\n");

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
				logger.log("Server:  Last packet, setting final bit\n");
				sendMsg->finalBit = 1;
			}
			else {
				sendMsg->finalBit = 0;
			}

			send(buf);
			logger.log("Server:  Sending file packet\n");
			packetCount += 1;
		}
		cout << "File transfer complete\n";
		logger.log("Server:  File transfer complete\n");
	}
	else
	{
		char buf[BUFLEN];
		struct message * sendMsg = (struct message *) buf;
		sendMsg->sequenceBit = seq;
		increaseSequence();
		sendMsg->errorBit = 1;
		logger.log("Server:  Unable to find file\n");
		cout << "Unable to find file\n";
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
			logger.log("Server:  Error recieving data from client");
			cout << "Error receiving data";
			exit(EXIT_FAILURE);
		}
		else {
			struct message *resMsg = (struct message *) resBuffer;
			validateSequence(resMsg->sequenceBit);
			
			outFile.write(resMsg->body, BODYLEN);
			if (resMsg->finalBit == 1) {
				cout << "End of file transfer\n";
				logger.log("End of file transfer\n");
				break;
			}
		}
	}

}

int Server::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		logger.log("Server:  Sequence bits match up\n");
		increaseSequence();
		return 1;
	}
	else {
		logger.log("Server:  Sequence bits don't match\n");
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
