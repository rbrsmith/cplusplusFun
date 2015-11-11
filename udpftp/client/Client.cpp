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

	if ((s2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	if ((s3 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
	{
		printf("socket() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	
	memset((char *)&si_in, 0, sizeof(si_in));
	si_in.sin_family = AF_INET;
	si_in.sin_port = htons(IN_PORT);
	si_in.sin_addr.S_un.S_addr = inet_addr(SERVER);


	memset((char *)&si_out, 0, sizeof(si_out));
	si_out.sin_family = AF_INET;
	si_out.sin_port = htons(OUT_PORT);
	si_out.sin_addr.S_un.S_addr = inet_addr(SERVER);


	//Bind the UDP port1
	if (bind(s, (LPSOCKADDR)&si_in, sizeof(si_in)) == SOCKET_ERROR) {
		logger.log("Client:  Error binding in port\n");
		cout << "Error binding";
		exit(-1);
	}



	logger.log("Client:  Client application started\n");

}

void Client::test() {
	int numberOfPackets = 10;
	int windowSize = 5;
//	int timeout = 2;

	vector<message> msgV(numberOfPackets);

	char buffer[BUFLEN];
	memset(buffer, '\0', BUFLEN);
	struct message *msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 0;
	msgV.at(0) = *msg;


	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 1;
	msgV.at(1) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 2;
	msgV.at(2) = *msg;
	
	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 3;
	msgV.at(3) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 4;
	msgV.at(4) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 5;
	msgV.at(5) = *msg;
	
	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 6;
	msgV.at(6) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 7;
	msgV.at(7) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 8;
	msgV.at(8) = *msg;

	memset(buffer, '\0', BUFLEN);
	msg = (struct message *) buffer;
	msg->testBit = 1;
	msg->testNum = 9;
	msgV.at(9) = *msg;

	sendPackets(msgV, numberOfPackets, windowSize, (TIMEOUT_SECS + TIMEOUT_MASECS));
}



void Client::sendPackets(vector<message> msgV, int numberOfPackets, int windowSize, int timeout) {
	vector<int> timeSentV(numberOfPackets);

	for (int i = 0; i < timeSentV.size(); ++i) {
		timeSentV.at(i) = -1;
	}

	time_t  timev;
	struct message *msg;
	char buffer[BUFLEN];
	for (int i = 0; i < windowSize; ++i) {
		struct message m = msgV.at(i);
		memset(buffer, '\0', BUFLEN);
		msg = (struct message *) buffer;
		msg->testBit = m.testBit;
		msg->testNum = m.testNum;
		cout << "Sending packet # " << msg->testNum << "\n";
		send(buffer);
		time(&timev);
		timeSentV.at(i) = timev;
		
	}



	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	time(&timev);
	
	int numPacketsEnRoute = windowSize;
	int packetsReceived = 0;
	while (1) {
		if (packetsReceived == numberOfPackets) {
			cout << "No packets are being sent, we are done\n";
			break;
		}
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			printf("%s", "Error in select\n");
			printf("%i", WSAGetLastError());
			printf("%s", "\n");
			exit(-1);
		}
		else if (!retval) {
			printf("%s", "Timeout event\n");
			// We have a timeout, we havent received a response in 5 seconds
			// who has failed
			int t = time(&timev);
			for (int i = 0; i < numberOfPackets; ++i) {
				int t_sent = timeSentV.at(i);
				if (t_sent < 0) {
					if (numPacketsEnRoute < windowSize) {
						// Send next packet
						struct message m = msgV.at(i);

						memset(buffer, '\0', BUFLEN);
						msg = (struct message *) buffer;
						msg->testBit = m.testBit;
						msg->testNum = m.testNum;
						cout << "Sending packet # " << msg->testNum << "\n";
						send(buffer);

						timeSentV.at(i) = t;
						numPacketsEnRoute += 1;
					}
					continue;
				}
				if (t_sent == 0) {
					continue;
				}
				if (t >= t_sent + (TIMEOUT_SECS)) {
					struct message m = msgV.at(i);
					memset(buffer, '\0', BUFLEN);
					msg = (struct message *) buffer;
					msg->testBit = m.testBit;
					msg->testNum = m.testNum;

					cout << "Re-Sending packet # " << msg->testNum << "\n";
					send(buffer);
					timeSentV.at(i) = t;
				}
				else {
				//	cout << "Pakcage " << i << " was not sent over 4 seconds ago\n";
				}


			}

		}
		else {
			if (FD_ISSET(s, &rfds)) {
				char nextBuffer[BUFLEN];
				if ((recv_len = recvfrom(s, nextBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
				{
					logger.log("Client:  recvfrom() failed\n");
					exit(EXIT_FAILURE);
				}
				else {
					struct message *res = (struct message *) nextBuffer;
					cout << "Message got response for package #" << res->testNum << "\n";
					timeSentV.at(res->testNum) = 0;
					numPacketsEnRoute -= 1;
					packetsReceived += 1;
					
					// Send next packet
					int t = time(&timev);
					for (int i = 0; i < numberOfPackets; ++i) {
						int t_sent = timeSentV.at(i);
						if (t_sent < 0) {
							if (numPacketsEnRoute < windowSize) {
								// Send next packet
								struct message m = msgV.at(i);
								memset(buffer, '\0', BUFLEN);
								msg = (struct message *) buffer;
								msg->testBit = m.testBit;
								msg->testNum = m.testNum;
								cout << "Sending packet # " << msg->testNum << "\n";
								send(buffer);

								timeSentV.at(i) = t;
								numPacketsEnRoute += 1;
							}
						}
					}
				}
			}

		}
	}


}


void Client::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_out, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		logger.log("Client:  Error in sending data");
		cout << "Sending data error";
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);
}

char * Client::reliableSend(char * buffer) {
	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	char buffer_bu[BUFLEN];
	memcpy(buffer_bu, buffer, BUFLEN);
	send(buffer);

	char resBuffer[BUFLEN];
	while (true) {
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			logger.log("Client:  Error in select\n");
			cout << "Error in select";
			exit(-1);
		}
		if (!retval) {
			logger.log("Client:  Timeout in reliable send (no sequence), resending\n");
			char buffer[BUFLEN];
			memcpy(buffer, buffer_bu, BUFLEN);
			send(buffer);
		}
		else {
			logger.log("Client:  Reliable send (no sequence) response received\n");
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			logger.log("Client:  Received a packet in reliable send (no sequence)\n");
			return resBuffer;
		}
	}
	return resBuffer;
}

char * Client::reliableSend(char * buffer, int sequence) {
	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	char buffer_bu[BUFLEN];
	memcpy(buffer_bu, buffer, BUFLEN);
	cout << "reliable send1 sending\n";
	send(buffer);

	char resBuffer[BUFLEN];
	while (true) {
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			logger.log("Client:  Error in select\n");
			cout << "Error in select";
			exit(-1);
		}
		if (!retval) {
			logger.log("Client:  Timeout in reliable send, resending\n");
			char buffer[BUFLEN];
			memcpy(buffer, buffer_bu, BUFLEN);
			send(buffer);
			cout << "reliable send1 resending\n";
		}
		else {
			logger.log("Client:  Reliable send response received\n");
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			} 
			struct message *resMsg = (struct message *) resBuffer;
			cout << resMsg->sequence << "\n";
			if (resMsg->sequence != sequence) {
				cout << sequence << "\n";
				logger.log("Client:  Wrong packet arrived in reliable send.  Resending\n"); char buffer[BUFLEN];
				memcpy(buffer, buffer_bu, BUFLEN);
				cout << "reliable send1 resending 2\n";
				send(buffer);

			}
			else {
				logger.log("Client:  Received correct packet in reliable send\n");
				return resBuffer;
			}
		}


	}
	return resBuffer;
}

int Client::handshake() {
	char buffer[BUFLEN];
	memset(buffer, '\0', BUFLEN);
	struct message *msg = (struct message *) buffer;
	int syn = getRandomNumber();
	msg->SYN = syn;
	msg->messageType = -1;
	logger.log("Client:  Sending handshake SYN: ");
	logger.log(msg->SYN);
	logger.log("\n");
	send(buffer);
	char nextBuffer[BUFLEN];

	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;

	FD_ZERO(&rfds);
	FD_SET(s, &rfds);
	retval = select(1, &rfds, NULL, NULL, &tv);
	if (retval == -1) {
		cout << "Error in select\n";
		logger.log("Client:  Error in select\n");
		printf("%s", "\n");
		exit(-1);
	}
	while (!retval) {
		msg->SYN = syn;
		msg->messageType = -1;
		logger.log("Client:  Timeout in handshake.  Resending\n");
		logger.log("Client:  Sending handshake SYN: ");
		logger.log(msg->SYN);
		logger.log("\n");
		send(buffer);
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
	}
	if (FD_ISSET(s, &rfds)) {
		if ((recv_len = recvfrom(s, nextBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Client:  recvfrom() failed\n");
			exit(EXIT_FAILURE);
		}
		else {
			struct message *res = (struct message *) nextBuffer;
			if (res->ACK != syn) {
				logger.log("Client:  Error in handshake, incorrect acknowledgement from Server.  Expected ");
				logger.log("Client:  Timeout in handshake.  Resending\n");
				logger.log("Client:  Sending handshake SYN: ");
				logger.log(msg->SYN);
				logger.log("\n");
				logger.log(syn);
				logger.log(" received ");
				logger.log(res->ACK);
				logger.log("\n");
				cout << "Error in hs";
				exit(EXIT_FAILURE);
			}
			else {
				logger.log("Client:  Handshake received correct acknowledgement of ");
				logger.log(res->ACK);
				logger.log(" from server\n");
				char finalBuffer[BUFLEN];
				struct message *msg = (struct message *) finalBuffer;
				msg->ACK = res->SYN;
				msg->messageType = -1;
				logger.log("Client:  Sending acknowledgement of ");
				logger.log(msg->ACK);
				logger.log(" to server\n");
				send(finalBuffer);
				
				int tries = 0;
				time_t  timev;
				time(&timev);
				while (true) {
					FD_ZERO(&rfds);
					FD_SET(s, &rfds);
					retval = select(1, &rfds, NULL, NULL, &tv);
					if (retval == -1) {
						cout << "Error in select\n";
						logger.log("Client:  Error in select\n");
						exit(-1);
					}
					if (!retval) {
						// resend
						time_t timev2;
						time(&timev2);
						if (timev2 > timev + MAX_WAIT) {
							logger.log("Client:  Multiple timeouts occured on final handshake step\nClient:  Handshake complete\n");
							cout << "Handshake Complete\n";
							Sleep(1000 * MAX_WAIT);
							return 0;
						}
						logger.log("Client:  Timeout in handshake third step.  Resending\n");
						struct message *msg = (struct message *) finalBuffer;
						msg->ACK = res->SYN;
						msg->messageType = -1;
						logger.log("Client:  Sending acknowledgement of ");
						logger.log(msg->ACK);
						logger.log(" to server\n");
						send(finalBuffer);
						tries += 1;
						continue;
					}
					if (FD_ISSET(s, &rfds)) {
						char buffer[BUFLEN];
						if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
						{
							logger.log("Client:  recvfrom() failed\n");
							exit(EXIT_FAILURE);
						}
						struct message *msg = (struct message *) buffer;
						if (msg->ACK == -1 && msg->SYN == -1) {
							cout << "Handshake completed\n";
							logger.log("Client:  Handshake completed successfully!\n");
							Sleep(1000 * MAX_WAIT);
							return 0;
						}
						else {
							logger.log("Client:  Incorrect packet arrival in handshake third step.  Resending\n");
							struct message *msg = (struct message *) finalBuffer;
							msg->ACK = res->SYN;
							msg->messageType = -1;
							logger.log("Client:  Sending acknowledgement of ");
							logger.log(msg->ACK);
							logger.log(" to server\n");
							send(finalBuffer);
							tries += 1;
						}
					}
				}
				
			}
		}
	}	
}

void Client::printRemoteList() {
	logger.log("Client:  Show remote files selected\n");
	handshake();

	char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	msg->messageType = 0; //This is a LIST operation
	msg->sequence = getRandomNumber();
	cout << "Sending packet with mtype " << msg->messageType << " and sequence " << msg->sequence << "\n";
	char * resBuf = reliableSend(buffer, msg->sequence);
	cout << "HERE";
	struct message * resMsg = (struct message *) resBuf;
	logger.log("Client:  Display remote listing to user\n");
	cout << "\n" << resMsg->body;


	// We got what we wanted, now tell Server to exit list mode
	memset(buffer, '\0', BUFLEN);
	struct message * m = (struct message *) buffer;
	m->messageType = -1;
	struct message * exitListMsg = (struct message *) buffer;
	memset(resBuf, '/0', BUFLEN);
	logger.log("Client:  Sending exit list message to server\n");
	resBuf = reliableSend(buffer);
	resMsg = (struct message *) resBuf;

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
	msg->messageType = 1; //This is a GET operation
	int seq = getRandomNumber();
	msg->sequence = seq;
	strcpy_s(msg->body, filename.c_str());
	cout << "SEQUENCE = " << msg->sequence << "\n";
	cout << "BODY = " << msg->body << "\n";



	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	send(buffer);
	char firstPacket[BUFLEN];
	char resBuffer[BUFLEN];
	while (true) {
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			logger.log("Client:  Error in select\n");
			cout << "Error in select";
			exit(-1);
		}
		if (!retval) {
			logger.log("Client:  Timeout in reliable send, resending\n");
			msg->messageType = 1;
			msg->sequence = seq;
			strcpy_s(msg->body, filename.c_str());
			send(buffer);
			cout << "resending filename\n";
		}
		else {
			cout << "Filename response received \n";
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			struct message *resMsg = (struct message *) resBuffer;
			cout << resMsg->sequence << "\n";
			if (resMsg->sequence == seq) {
				cout << "Got sequence back from server, ready to start receiving files\n";
				deliverFile();
				break;
			}
			else if (resMsg->sequence > seq) {
				cout << "OH SHIT, server is already sending us files, get ready to recceive and this is the first paccket\n";
				deliverFile(resMsg);
				break;
			}
			else {
				cout << "Weird = what packet is this?\n";
				msg->sequence = seq;
				strcpy_s(msg->body, filename.c_str());
				send(buffer);
				cout << "resending filename\n";
			}
			
		}	

	}








	

	/*char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	msg->messageType = 1;
	increaseSequence();
	strcpy_s(msg->body, filename.c_str());
	send(buffer);

	char resBuffer[BUFLEN];

	ofstream outFile(filename, ios::out | ios::binary);
	while (1) {
			
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			logger.log("Client:  Error in receiving data");
			exit(EXIT_FAILURE);
		}
		else {
			logger.log("Client:  Received file data from server\n");
			struct message *resMsg = (struct message *) resBuffer;
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
*/
}

void Client::deliverFile(struct message * firstMsg) {
	cout << "READY WITH FIRST PACKET SIR\n";
	cout << firstMsg->body;
}

void Client::deliverFile() {
	cout << "READY WITH NO PACKET SIR\n";

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