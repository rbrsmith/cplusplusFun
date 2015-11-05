#include "stdafx.h";
#include "Client.h";


Client::Client() {
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	memset((char *)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);
	si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
	logger.log("Client:  Client application started\n");

}

void Client::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		logger.log("Client:  Error in sending data");
		cout << "Sending data error";
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);

}

int Client::handshake() {
	char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	int syn = getRandomNumber();
	msg->SYN = syn;
	logger.log("Client:  Sending handshake SYN: ");
	logger.log(syn);
	logger.log("\n");
	send(buffer);

	char nextBuffer[BUFLEN];
	if ((recv_len = recvfrom(s, nextBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{	
		logger.log("Client:  recvfrom() failed\n");
		exit(EXIT_FAILURE);
	}
	else {
		struct message *res = (struct message *) nextBuffer;
		if (res->ACK != syn) {
			logger.log("Client:  Error in handshake, incorrect acknowledgement from Server.  Exepceted ");
			logger.log(syn);
			logger.log(" received ");
			logger.log(res->ACK);
			logger.log("\n");
			exit(EXIT_FAILURE);
		}
		else {
			logger.log("Client:  Handshake received correct acknowledgement of ");
			logger.log(res->ACK);
			logger.log(" from server\n");
			char finalBuffer[BUFLEN];
			struct message *msg = (struct message *) finalBuffer;
			msg->ACK = res->SYN;
			int bit = res->SYN & (1 << 0);
			logger.log("Client:  Sending acknowledgement of ");
			logger.log(msg->ACK);
			logger.log(" to server\n");
			logger.log("Client:  Setting sequence bit to ");
			logger.log(bit);
			logger.log("\n");
			send(finalBuffer);
			
			return bit;
		}

	}

}

void Client::printRemoteList() {
	logger.log("Client:  Show remote files selected\n");
	seq = handshake();
	char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	msg->messageType = 0; //This is a LIST operation
	msg->sequenceBit = seq;


	send(buffer);
	increaseSequence();

	char resBuffer[BUFLEN];
	if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		logger.log("Error receiving data from server");
		exit(EXIT_FAILURE);
	}
	else {
		struct message *resMsg = (struct message *) resBuffer;
		validateSequence(resMsg->sequenceBit);
		if (resMsg->errorBit == 1) {
			cout << "Unable to get remote directory as remote directory is too large\n";
			logger.log("Client:  Unable to get remote directory as remote directory is too large\n");
			return;
		}
		cout << "Server Files:\n";
		cout << "- - - - - - - \n";
		logger.log("Client:  List of file successfuly arrived\n");
		cout << resMsg->body;
	}
}

void Client::getFile(string filename) {
	logger.log("Client:  Get file selected\n");
	logger.log("Client:  Filename: ");
	char why[BODYLEN];
	strcpy_s(why, filename.c_str());
	logger.log(why);
	logger.log("\n");
	if (filename.length() > BODYLEN) {
		cout << "Unable to get file, filename is too long\n";
		logger.log("Client:  unable to get file, filename too long\n");
		return;
	}
	seq = handshake();
	char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	msg->messageType = 1;
	msg->sequenceBit = seq;
	increaseSequence();
	strcpy_s(msg->body, filename.c_str());
	send(buffer);

	char resBuffer[BUFLEN];

	ofstream outFile(filename, ios::out | ios::binary);
	while (1) {
			
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			logger.log("Client:  Error in receiving data");
			exit(EXIT_FAILURE);
		}
		else {
			logger.log("Client:  Received file data from server\n");
			struct message *resMsg = (struct message *) resBuffer;
			validateSequence(resMsg->sequenceBit);
			if (resMsg->errorBit == 1) {
				cout << "ERROR: " << resMsg->body;
				logger.log("Client:  Server responded with error, maybe file was not found\n");
				return;
			}
			else {
				outFile.write(resMsg->body, BODYLEN);
				if (resMsg->finalBit == 1) {
					logger.log("Client:  Final bit indicated file transfer has ended\n");
					cout << "End of file transfer\n";
					break;
				}
			}
		}
	}

}

void Client::printLocalList() {
	logger.log("Client:  Print local listing requested\n");
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
		cout << output << "\n";
		logger.log("Client:  Local files listed successfully\n");
	}
	else {
		/* could not open directory */
		logger.log("\nClient:  Error getting file listing");
		perror("Error getting file listing\n");
		exit(EXIT_FAILURE);
	}
}

void Client::sendFile(string filename) {
	logger.log("Client:  Put command selected\n");
	if (filename.length() > BODYLEN) {
		cout << "Unable to send file, filename is too long";
		logger.log("Client:  Unable to send file, filename is too long\n");
		return;
	}
	seq = handshake();
	
	logger.log("\nSending file ");
	char why[BODYLEN];
	strcpy_s(why, filename.c_str());
	logger.log(why);
	logger.log(" to server\n");
	cout << "Looking for file " << filename << "\n";

	ifstream fileToRead;
	fileToRead.open(filename, ios::in | ios::binary);
	if (fileToRead.is_open())
	{

		// First packet is just filename
		char buf[BUFLEN];
		struct message * sendMsg = (struct message *) buf;
		sendMsg->sequenceBit = seq;
		increaseSequence();
		strcpy_s(sendMsg->body, filename.c_str());
		sendMsg->messageType = 4;
		send(buf);

		// Can make this a bit better
		// Count how many packets there will be
		char fakeBuf[BODYLEN];
		int numPackets = 0;
		while (!fileToRead.eof()) {
			memset(fakeBuf, '\0', BODYLEN);
			numPackets += 1;
			fileToRead.read(fakeBuf, BODYLEN);
		}
		logger.log("Client:  Will send ");
		logger.log(numPackets);
		logger.log(" packets to server\n");


		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);
		int packetCount = 1;

		memset(buf, '\0', BUFLEN);

		while (!fileToRead.eof())
		{
			memset(buf, '\0', BODYLEN);


			struct message * sendMsg = (struct message *) buf;
			sendMsg->sequenceBit = seq;
			increaseSequence();

			/* Read the contents of file and write into the buffer for transmission */
			fileToRead.read(sendMsg->body, BODYLEN);

			if (packetCount >= numPackets) {
				logger.log("Client:  Final packet, setting final bit\n");
				sendMsg->finalBit = 1;
			}
			else {
				sendMsg->finalBit = 0;
			}
			logger.log("Client:  Sent file packet to server\n");
			send(buf);
			packetCount += 1;
		}
		logger.log("Client:  File transfer complete\n");
		cout << "File transfer complete\n";
	}
	else
	{
		logger.log("CLient:  Unable to find file\n");
		cout << "Unable to find file\n";
	}
	fileToRead.close();
}

	
int Client::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		logger.log("Client:  Sequence bits match up\n");
		increaseSequence();
		return 1;
	}
	else {
		logger.log("Client:  Sequence bits dont match up\n");
		exit(EXIT_FAILURE);
	}
}

int Client::getRandomNumber() {
	srand((unsigned)time(0));
	int randNum =  (rand() % 150) + 1;
	return randNum;
}

void Client::increaseSequence() {
	if (seq == 1) {
		seq = 0;
	}
	else {
		seq = 1;
	}
}