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

}

void Client::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
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
	send(buffer);

	char nextBuffer[BUFLEN];
	if ((recv_len = recvfrom(s, nextBuffer, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
	{	
		printf("recvfrom() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	else {
		struct message *res = (struct message *) nextBuffer;
		if (res->ACK != syn) {
			cout << "Error in handshake, incorrect acknowledgment.  Expected " << syn << " recevied " << res->ACK << "\n";
			exit(EXIT_FAILURE);
		}
		else {
			char finalBuffer[BUFLEN];
			struct message *msg = (struct message *) finalBuffer;
			msg->ACK = res->SYN;
			send(finalBuffer);
			int bit = res->SYN & (1 << 0);
			return bit;
		}

	}

}

void Client::printRemoteList() {
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
		exit(EXIT_FAILURE);
	}
	else {
		struct message *resMsg = (struct message *) resBuffer;
		validateSequence(resMsg->sequenceBit);
		if (resMsg->errorBit == 1) {
			cout << "Unable to get remote directory as remote directory is too large\n";
			return;
		}
		cout << "Server Files:\n";
		cout << "- - - - - - - \n";
		cout << resMsg->body;
	}
}

void Client::getFile(string filename) {
	if (filename.length() > BODYLEN) {
		cout << "Unable to get file, filename is too long\n";
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
			exit(EXIT_FAILURE);
		}
		else {
			struct message *resMsg = (struct message *) resBuffer;
			validateSequence(resMsg->sequenceBit);
			if (resMsg->errorBit == 1) {
				cout << "ERROR: " << resMsg->body;
				return;
			}
			else {
				outFile.write(resMsg->body, BODYLEN);
				if (resMsg->finalBit == 1) {
					cout << "End of file transfer\n";
					break;
				}
			}
		}
	}

}

void Client::printLocalList() {
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
	}
	else {
		/* could not open directory */
		perror("Error getting file listing\n");
		exit(EXIT_FAILURE);
	}
}

void Client::sendFile(string filename) {
	if (filename.length() > BODYLEN) {
		cout << "Unable to send file, filename is too long";
		return;
	}
	seq = handshake();
	

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
				sendMsg->finalBit = 1;
			}
			else {
				sendMsg->finalBit = 0;
			}

			send(buf);
			packetCount += 1;
		}
		cout << "File transfer complete\n";
	}
	else
	{
		cout << "Unable to find file\n";
	}
	fileToRead.close();
}


int Client::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		increaseSequence();
		return 1;
	}
	else {
		cout << "Sequence bits dont mathch\n";
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