#include "stdafx.h";
#include "Client.h";

/**
 *  Constructor - set up sockets 
 */
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


	// Set up in and out sockets
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


/**
 * Wrapper for winsock send function
 */
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

/**
 *  Will send a buffer to server and wait for any response back
 *  Upon timeout will resend
 */
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
			logger.log("Client:  Timeout sending packet, resending\n");
			char buffer[BUFLEN];
			memcpy(buffer, buffer_bu, BUFLEN);
			send(buffer);
		}
		else {
			logger.log("Client:  Received response for packet sent\n");
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
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
			logger.log("Client:  Timeout sending packet, resending\n");
			char buffer[BUFLEN];
			memcpy(buffer, buffer_bu, BUFLEN);
			send(buffer);
		}
		else {
			logger.log("Client:  Received response for packet sent\n");
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			struct message *resMsg = (struct message *) resBuffer;
			if (resMsg->sequence != sequence) {
				logger.log("Client:  Wrong packet arrived.  Resending packet\n"); char buffer[BUFLEN];
				memcpy(buffer, buffer_bu, BUFLEN);
				send(buffer);

			}
			else {
				logger.log("Client:  Received correct response for packet sent\n");
				return resBuffer;
			}
		}


	}
	return resBuffer;
}

/**
 *  Perform 3 way handshake
 */
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
				logger.log("Client:  Re-starting handshake\n");
				return handshake();
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

/**
 * Outputs the remote listing from the server
 */
void Client::printRemoteList() {
	logger.log("Client:  Show remote files selected\n");
	handshake();

	char buffer[BUFLEN];
	struct message *msg = (struct message *) buffer;
	msg->messageType = 0; //This is a LIST operation
	msg->sequence = getRandomNumber();
	char * resBuf = reliableSend(buffer, msg->sequence);
	struct message * resMsg = (struct message *) resBuf;
	logger.log("Client:  Received file listing from server\n");
	logger.log("Client:  Displaying remote listing to user\n");
	cout << resMsg->body << "\n";

	// We got what we wanted, now tell Server to exit list mode
	memset(buffer, '\0', BUFLEN);
	struct message * m = (struct message *) buffer;
	m->messageType = -1;
	struct message * exitListMsg = (struct message *) buffer;
	memset(resBuf, '/0', BUFLEN);
	logger.log("Client:  Sending exit list message to server\n");
	resBuf = reliableSend(buffer);
	resMsg = (struct message *) resBuf;
	logger.log("Client:  Server has acknowledged exit of list message\n");
}

/**
 * GET operation.  Given a file name let the server know about
 * the file and prepare client to receive file packets
 */
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

	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	logger.log("Client:  Sending GET filename to server with sequence ");
	logger.log(seq);
	logger.log("\n");
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
			logger.log("Client:  Timeout in sending filename to server, resending with sequence \n");
			logger.log(seq);
			logger.log("\n");
			msg->messageType = 1;
			msg->sequence = seq;
			strcpy_s(msg->body, filename.c_str());
			send(buffer);
		}
		else {
			logger.log("Client:  Server has acknowledge filename\n");
			memset(buffer, '\0', BUFLEN);
			if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			struct message *resMsg = (struct message *) resBuffer;
			if (resMsg->sequence == seq) {
				logger.log("Client:  Response from server on filename has correct sequence number of ");
				logger.log(seq);
				logger.log(" client is ready to file packets now\n");
				deliverFile(seq, filename);
				break;
			}
			else if (resMsg->sequence > seq) {
				logger.log("Client:  Response from server on filename has sequence greater than that sent of ");
				logger.log(seq);
				logger.log(" server is already sending packets.. client is ready to receive file packets now\n");
				// we drop that first packet cause we're naughty like that
				deliverFile(seq, filename);
				break;
			}
			else {
				logger.log("Client:  Received dealyed packet in client, resending filename packet\n");
				msg->sequence = seq;
				strcpy_s(msg->body, filename.c_str());
				send(buffer);
			}

		}

	}
}

/**
 * Accept all file packets from server, and acknowledge them as they
 * arrive.  Saves all packets into a vector then writes this vector to a file
 */
void Client::deliverFile(int sequence, string filename) {
	vector<string> receivedPackets(0);
	vector<char *> receivedPackets2(0);
	vector<int> sequenceNums(0);
	while (true) {
		char buffer[BUFLEN];
		if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Client:  recvfrom() failed\n");
			exit(EXIT_FAILURE);
		}
		cout << ".";
		// get message
		struct message * msg = (struct message *) buffer;
		if (msg->finalBit == 1) {
			cout << "\n";
			logger.log("Client:  Received file closing packet from server, letting server know client has received it\n");
			char finalBuf[BUFLEN];
			struct message * finalMsg = (struct message *) finalBuf;
			finalMsg->sequence = msg->sequence;
			memset(buffer, '/0', BUFLEN);
			logger.log("Client:  Sending acknowledgment of final packet to server with sequence of ");
			logger.log(msg->sequence);
			logger.log("\n");
			char * buffer = reliableSend(finalBuf);
			logger.log("Client:  Received acknowledgment from server that it received our acknowledgment\n");
			logger.log("Client:  Sending empy packet to server to ensure it is back in ready mode\n");
			reliableSend(buffer);
			break;
		}
		if (msg->sequence < sequence) {
			if (msg->sequence < 0) {
				// OK, server is clearly sending us something from handshake
				logger.log("Client:  Received delayed packet from server with no sequence number, restarting GET\n");
				cout << "x";
				return getFile(filename);
			}
		}
		if (msg->numPackets < 0) {
			logger.log("Client:  Received delayed packet from server\n");
			// Somethings wrong, if numpackets aren't sent, we're not in right mode
			// tell server to go into get mode damnit
			char errorBuf[BUFLEN];
			struct message * errorMsg = (struct message *) errorBuf;
			errorMsg->messageType = -1;
			logger.log("Client:  Sending reset packets to server\n");
			reliableSend(errorBuf);
			memset(errorBuf, '/0', BUFLEN);
			errorMsg = (struct message *) errorBuf;
			errorMsg->messageType = 1;
			errorMsg->sequence = sequence;
			strcpy_s(errorMsg->body, filename.c_str());
			reliableSend(errorBuf);
			logger.log("Client:  Server should now be reset\n");
			cout << "x";
			continue;
		}
		if (receivedPackets.size() < msg->numPackets) {
			logger.log("Client:  Received file packet from server - setting client number packets to match server");
			logger.log("Client: There will be ");
			logger.log(msg->numPackets);
			logger.log("\n");
			receivedPackets.resize(msg->numPackets);
			receivedPackets2.resize(msg->numPackets);
			sequenceNums.resize(msg->numPackets);
			for (int i = 0; i < receivedPackets.size(); i++) {
				receivedPackets.at(i) = -1;
				sequenceNums.at(i) = -1;
			}
		}
		char resBuffer[BUFLEN];
		struct message * resMsg = (struct message *) resBuffer;
		resMsg->sequence = msg->sequence;
		int actualSequence = msg->sequence - sequence - 1;
		logger.log("Client:  Received file packet from server with sequence ");
		logger.log(msg->sequence);
		logger.log(" and relative sequence of ");
		logger.log(actualSequence);
		logger.log("\n");
		if (actualSequence < 0) {
			cout << "x";
			logger.log("Client:  Relative sequence is out of range, sending reset packets to server\n");
			// looks like client sent us a delayed packet from something else - reset
			char errorBuf[BUFLEN];
			struct message * errorMsg = (struct message *)  errorBuf;
			errorMsg->messageType = -1;
			reliableSend(errorBuf);
			memset(errorBuf, '/0', BUFLEN);
			errorMsg = (struct message *) errorBuf;
			errorMsg->messageType = 1;
			errorMsg->sequence = sequence;
			strcpy_s(errorMsg->body, filename.c_str());
			reliableSend(errorBuf);
			logger.log("Client:  Server should now be reset\n");
			continue;
		}
		string data = msg->body;
		receivedPackets.at(actualSequence) = data;
		char * newBuf = new char[BODYLEN];
		memcpy(newBuf, msg->body, BODYLEN);
		receivedPackets2.at(actualSequence) = newBuf;
		if (sequenceNums.at(actualSequence) != -1) {
			cout << "x";
			sequenceNums.at(actualSequence) = msg->sequence;
		}
		logger.log("Client:  Received a correct file packet of sequence ");
		logger.log(msg->sequence);
		logger.log("\n");
		logger.log("Client:  Adding to received packets buffer\n");
		logger.log("Client:  Sending acknowledgment for file packet with sequence ");
		logger.log(resMsg->sequence);
		logger.log("\n");
		send(resBuffer);
	}
	logger.log("Client:  Saving ");
	char fb[BODYLEN];
	strcpy_s(fb, filename.c_str());
	logger.log(fb);
	logger.log(" to file\n");
	ofstream outFile(filename, ios::out | ios::binary);
	for (int i = 0; i < receivedPackets.size(); ++i)
	{
		char b[BODYLEN];
		memcpy(b, receivedPackets2.at(i), BODYLEN - 1);
		outFile.write(b, BODYLEN - 1);
		memset(b, '/0', BODYLEN);
	}
	logger.log("Client:  File saving done\n");
	cout << "File transferred!\n";
	outFile.close();
}

/**
 * Get local file directory and print files to screen
 */
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
		cout << output;
		closedir(dir);
		logger.log("Client:  Local files listed successfully\n");
	}
	else {
		/* could not open directory */
		logger.log("\nClient:  Error getting file listing");
		perror("Error getting file listing\n");
		exit(EXIT_FAILURE);
	}
}

/**
 * Given a file name, read the file to a vector of packets and send
 * packets to server, if no acknowledgment is recieved before a timeout, resend
 */
void Client::sendFile(string filename) {
	logger.log("Client:  Put command selected\n");
	if (filename.length() > BODYLEN) {
		cout << "Unable to send file, filename is too long";
		logger.log("Client:  Unable to send file, filename is too long\n");
		return;
	}
	seq = handshake();

	logger.log("Client:  Sending file ");
	char why[BODYLEN];
	strcpy_s(why, filename.c_str());
	logger.log(why);
	logger.log(" to server\n");
	
	// Get number packets to be sent
	int numPackets = 0;

	ifstream fileToRead;
	fileToRead.open(filename, ios::in | ios::binary);
	vector<char *> packets(0);
	if (fileToRead.is_open())
	{
		char fakeBuf[BODYLEN];
		while (!fileToRead.eof()) {
			memset(fakeBuf, '\0', BODYLEN);
			numPackets += 1;
			fileToRead.read(fakeBuf, BODYLEN - 1);
		}
		packets.resize(numPackets);
		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);
		char buffer[BODYLEN];
		int i = 0;
		while (!fileToRead.eof()) {
			memset(buffer, '\0', BODYLEN);
			fileToRead.read(buffer, BODYLEN - 1);
			char * newBuf = new char[BODYLEN];
			memcpy(newBuf, buffer, BODYLEN - 1);
			packets.at(i) = newBuf;
			i += 1;
			
		}
	}
	else {
		cout << "Unknown file\n";
		logger.log("Client:  Unable to find file\n");
		return;
	}


	char buffer[BUFLEN];
	memset(buffer, '/0', BUFLEN);
	struct message * filenameMsg = (struct message *) buffer;
	char bdy[BODYLEN];
	strcpy_s(filenameMsg->body, filename.c_str());
	filenameMsg->messageType = 4;
	int sequence = getRandomNumber();
	filenameMsg->sequence = sequence;
	cout << filenameMsg->body << "\n";
	reliableSend(buffer, sequence);

	vector<int> timeSend(numPackets);
	vector<int> sequenceNumbers(numPackets);
	for (int i = 0; i < numPackets; ++i) {
		timeSend.at(i) = -1;
		sequenceNumbers.at(i) = -1;
	}

	// We can send file now
	int window_size = WINDOW_SIZE;
	int packetsEnRoute = 0;

	time_t timev;
	for (int i = 0; i < window_size; ++i) {
		if (i >= numPackets) {
			logger.log("Client:  Number of packets is less than window size\n");
			logger.log("CLient:  Done sending packets\n");
			break;
		}
		
		char buffer[BUFLEN];
		struct message * m = (struct message *) buffer;
		sequence += 1;
		m->sequence = sequence;
		m->numPackets = numPackets;
		m->body[BODYLEN];
		
		memcpy(m->body, packets.at(i), BODYLEN - 1);

		time(&timev);
		timeSend.at(i) = timev;
		sequenceNumbers.at(i) = sequence;
		send(buffer);
		logger.log("Client:  Sending packet number ");
		logger.log(i);
		logger.log(" with sequence number ");
		logger.log(sequence);
		logger.log("\n");
		logger.log("Client:  Updating time sent of packet number ");
		logger.log(i);
		logger.log("\n");
		packetsEnRoute += 1;
	}

	fd_set rfds;
	struct timeval tv;

	tv.tv_sec = TIMEOUT_SECS;
	tv.tv_usec = TIMEOUT_MASECS;
	int retval;
	while (1) {
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			cout << "Error in select\n";
			logger.log("Client:  Error in select\n");
			exit(-1);
		}
		if (!retval) {
			logger.log("Client:  Timeout in PUT\n");
			time_t time_now;
			time(&time_now);
			for (int i = 0; i < numPackets; ++i) {
				if (timeSend.at(i) != -1 && timeSend.at(i) != 0 && timeSend.at(i) < time_now) {
					// resend
					char resendBuf[BUFLEN];
					struct message * resendMsg = (struct message *) resendBuf;
					int s = sequenceNumbers.at(i);
					resendMsg->sequence = s;

					memcpy(resendMsg->body, packets.at(i), BODYLEN - 1);

					resendMsg->numPackets = numPackets;
					logger.log("Client:  Resent packet ");
					logger.log(i);
					logger.log(" with sequence ");
					logger.log(resendMsg->sequence);
					logger.log("\n");
					time(&timev);
					timeSend.at(i) = timev;
					logger.log("Client:  Updating time sent of packet ");
					logger.log(i);
					logger.log("\n");
					cout << "x";
					send(resendBuf);
				}
			}

		}
		else {
			char successBuf[BUFLEN];
			if ((recv_len = recvfrom(s, successBuf, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			struct message * sucMsg = (struct message *) successBuf;
			logger.log("Client:  Received acknowlegment for file packet ");
			logger.log(sucMsg->sequence);
			logger.log("\n");
			// find who we are getting res for
			for (int i = 0; i < numPackets; ++i) {
				if (sequenceNumbers.at(i) == sucMsg->sequence) {
					if (timeSend.at(i) != 0) {
						cout << ".";
						logger.log("Client:  First acknowledgment for packet ");
						logger.log(i);
						logger.log(", updating packet status to done\n");
						timeSend.at(i) = 0;
						packetsEnRoute -= 1;
						break;
					}
					else {
						cout << "x";
					}
				}
			}

			// find next guy to send
			int sent = 0;
			// see if we can even send any
			logger.log("Client:  Looking to see if any more file packets need to be sent\n");
			if (packetsEnRoute < window_size) {
				for (int i = 0; i < numPackets; ++i) {
					if (timeSend.at(i) == -1) {
						// found our chap
						char nextSeq[BUFLEN];
						struct message * nextMsg = (struct message *) nextSeq;
						sequence += 1;
						nextMsg->sequence = sequence;
						sequenceNumbers.at(i) = sequence;

						memcpy(nextMsg->body, packets.at(i), BODYLEN - 1);

						nextMsg->numPackets = numPackets;

						time_t t;
						time(&t);
						timeSend.at(i) = t;
						logger.log("Client:  Sending next packet of ");
						logger.log(i);
						logger.log(" with sequence ");
						logger.log(sequence);
						logger.log("\n");

						logger.log("Client:  Updating time sent of packet ");
						logger.log(i);
						logger.log("\n");
						send(nextSeq);

						packetsEnRoute += 1;
						break;
					}
					else if (timeSend.at(i) == 0) {
						sent += 1;
					}
				}
			}
			if (sent == numPackets) {
				// We found nobody
				logger.log("Client:  All file packets sent\n");
				break;
			}
		}



	}
	cout << "\n";
	// Send final packet
	seq = sequence += 100;

	char finalBuf[BUFLEN];
	struct message * finalMsg = (struct message *) finalBuf;

	while (true) {
		finalMsg->finalBit = 1;
		finalMsg->sequence = seq;
		logger.log("Client:  Sending GET closing packet with sequence ");
		logger.log(seq);
		logger.log("to client indicating file transfer is done\n");
		send(finalBuf);
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			cout << "error in select";
			exit(-1);
		}
		if (!retval) {
			logger.log("Client:  Timeout in PUT closing packet\n");
			continue;
		}
		else {
			char finRecBuf[BUFLEN];
			if ((recv_len = recvfrom(s, finRecBuf, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Client:  recvfrom() failed\n");
				exit(EXIT_FAILURE);
			}
			struct message * finalRecMsg = (struct message *) finRecBuf;
			if (finalRecMsg->sequence == seq) {
				logger.log("Client:  Server has acknowledged PUT close.  Sending empty packet to reset everything\n");
				char fBuf[BUFLEN];
				reliableSend(fBuf);
				cout << "File Transfer is done!\n";
				return;
			}
			else {
				continue;
			}
		}
	}
	logger.log("Server:  File transfer is done\n");
	cout << "File Transfer done!\n";

}

/**
 * Return a random number
 */
int Client::getRandomNumber() {
	srand((unsigned)time(0));
	int randNum = (rand() % 150) + 1;
	return randNum;
}
