#include "stdafx.h"
#include "Server.h"

using namespace std;


/**
 * Star the server in a forever loop listening for incoming packets
 */
void Server::start() {

	logger.log("\nServer:  New Server instance started...\n");

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

	memset((char *)&si_in, 0, sizeof(si_in));
	si_in.sin_family = AF_INET;
	si_in.sin_port = htons(IN_PORT);
	si_in.sin_addr.s_addr = INADDR_ANY;

	//Bind the UDP port1
	if (bind(s, (LPSOCKADDR)&si_in, sizeof(si_in)) == SOCKET_ERROR) {
		logger.log("Server:  Error binding in port\n");
		cout << "Error binding";
		exit(-1);
	}


	memset((char *)&si_out, 0, sizeof(si_out));
	si_out.sin_family = AF_INET;
	si_out.sin_port = htons(OUT_PORT);
	si_out.sin_addr.S_un.S_addr = inet_addr(SERVER);

	logger.log("Server:  Server started and waiting...\n");
	//keep listening for data
	char serverBuf[BUFLEN];
	int count = 1;
	cout << "Server started\n";
	while (1) {
		logger.log("Server:  Ready state\n");
		struct message * msg = getDataFromClient(serverBuf);
		int seq = msg->sequence;
		string filename;
		switch (msg->messageType) {
		case 0:
			// List operation
			logger.log("Server:  List operation called...\n");
			cout << "LIST\n";
			list(seq);
			break;
		case 1:
			// Get operation
			filename = msg->body;
			logger.log("Server:  Get operation called...\n");
			cout << "GET\n";
			get(filename, seq);
			break;
		case 4:
			// Put operation
			filename = msg->body;
			logger.log("Server:  Put operation called...\n");
			cout << "PUT\n";
			put(filename, seq);
			break;
		}
		logger.log("Server:  Server:  Waiting for request...\n");
	}

}

/**
 * Perform handshake
 * Take a packet to be filled with handshake response
 */
int Server::handshake(char * fillMe) {
	// Get SYN
	char buffer[BUFLEN];
	int syn = -1;
	int ack = -1;
	while (1) {
		recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen);
		if (recv_len == SOCKET_ERROR) {
			logger.log("Server:  Error in recvfrom in handshake");
			exit(-1);
		}

		struct message *rec_msg = (struct message *) buffer;
		if (rec_msg->SYN > 0) {
			logger.log("Server:  Received SYN of ");
			logger.log(rec_msg->SYN);
			logger.log(" from client\n");
			// Send a response
			char nextBuffer[BUFLEN];
			struct message *res = (struct message *) nextBuffer;
			res->ACK = rec_msg->SYN;
			res->SYN = getRandomNumber();
			ack = res->SYN;
			logger.log("Server:  Sending response with ACK of ");
			logger.log(res->ACK);
			logger.log(" and SYN of ");
			logger.log(res->SYN);
			logger.log("\n");
			send(nextBuffer);
		}
		else if (rec_msg->ACK > 0) {
			if (ack == -1) {
				logger.log("Server:  Error in handshake, exiting\n");
				exit(-1);
			}
			if (rec_msg->ACK == ack) {
				logger.log("Server:  Received correct ACK of ");
				logger.log(rec_msg->ACK);
				logger.log(" from client\n");
				char nextBuffer[BUFLEN];
				struct message *res = (struct message *) nextBuffer;
				// Send empty buffer to close loop
				res->SYN = -1;
				res->ACK = -1;
				logger.log("Server:  Sending last packet of empty SYN and ACK\n");
				send(nextBuffer);

				fd_set rfds;
				struct timeval tv;
				int retval;
				tv.tv_sec = TIMEOUT_SECS + MAX_WAIT;
				tv.tv_usec = TIMEOUT_MASECS * 2;


				while (true) {

					FD_ZERO(&rfds);
					FD_SET(s, &rfds);
					retval = select(1, &rfds, NULL, NULL, &tv);
					if (retval == -1) {
						cout << "Error in select\n";
						logger.log("Server:  Error in select, exiting\n");
						exit(-1);
					}

					if (!retval) {
						logger.log("Server:  No more handshakes arriving from client. Handshake success!\n");
						cout << "Handdshake completed\n";
						return 0;
					}
					if (retval) {
						char buffer[BUFLEN];
						recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen);
						struct message * m = (struct message *) buffer;
						if (m->messageType >= 0) {
							cout << "Handshake completed\n";
							memcpy(fillMe, buffer, BUFLEN);
							logger.log("Server:  Client has send a message with a message type, indicating the client moved on from handshake. Handshae success\n");
							return 1;
						}
						logger.log("Server:  Received unexpected packet, client is still sending, wait until a timeout\n");
						char nextBuffer[BUFLEN];
						struct message *res = (struct message *) nextBuffer;
						// Send empty buffer to close loop
						res->SYN = -1;
						res->ACK = -1;
						send(nextBuffer);
					}
				}

			}
		}
		else {
			char failBuffer[BUFLEN];
			logger.log("Server:  Received unknown message in handshake.  Sending back empty response\n");

			send(failBuffer);
			return -1;
		}

	}

}

/**
 * Send a packet buffer
 */
void Server::send(char * buffer) {
	if (sendto(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_out, slen) == SOCKET_ERROR)
	{
		cout << "error sending";
		cout << WSAGetLastError();
		logger.log("Server:  Sending error!");
		exit(EXIT_FAILURE);
	}
	// Clear buffer
	memset(buffer, '\0', BUFLEN);
}


/** 
 * First function called - blocks until a response comes in
 * Performs handshake first
 */
Server::message * Server::getDataFromClient(char *serverBuf) {
	char fillMe[BUFLEN];
	int handshakeRes = handshake(fillMe);
	if (handshakeRes == -1) {
		logger.log("Server:  No response from handshake, going back into wait mode\n");
		struct message * empty = (struct message *) serverBuf;
		empty->messageType = -1;
		return empty;
	}
	else if (handshakeRes == 1) {
		struct message * m = (struct message *) fillMe;
		if (m->sequence >= 0) {
			return m;
		}
	}

	int messageType = -1;
	struct message *resmsg;
	while (messageType < 0) {
		if ((recv_len = recvfrom(s, serverBuf, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			cout << "Error in recvform";
			logger.log("Server:  Failed to recieve data from client");
			exit(EXIT_FAILURE);
		}
		else {
			resmsg = (struct message *) serverBuf;
			messageType = resmsg->messageType;

			logger.log("Server:  Received message packet, message type is ");
			logger.log(messageType);
			logger.log("\n");
		}
	}
	cout << "Incoming request\n";
	logger.log("Server:  Recceived valid message packet\n");
	return resmsg;
}

/**
 * List action:  Return packet of all files on server
 */
void Server::list(int sequence) {
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
	logger.log("Server:  Read server files without problem\n");
	// TODO we assume 1 packet size for list
	char sendBuffer[BUFLEN];
	struct message * sendMsg = (struct message *) sendBuffer;
	sendMsg->sequence = sequence;
	logger.log("Server:  Sending server files to client with sequence ");
	logger.log(sequence);
	logger.log("\n");
	if (output.length() >= BODYLEN) {
		logger.log("Server:  Unable to send file listing, listing is too large");
		sendMsg->errorBit = 1;
		send(sendBuffer);
		exit(EXIT_FAILURE);
	}
	strcpy_s(sendMsg->body, output.c_str());

	while (1) {
		struct message * sendMsg = (struct message *) sendBuffer;
		sendMsg->sequence = sequence;
		if (output.length() >= BODYLEN) {
			logger.log("Server:  Unable to send file listing, listing is too large");
			sendMsg->errorBit = 1;
			send(sendBuffer);
			exit(EXIT_FAILURE);
		}
		strcpy_s(sendMsg->body, output.c_str());
		logger.log("Server:  Sending file listing to client using sequence ");
		logger.log(sequence);
		logger.log("\n");
		send(sendBuffer);

		char resBuffer[BUFLEN];
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Server:  Error recieving data from client");
			cout << "Error receiving data";
			exit(EXIT_FAILURE);
		}
		logger.log("Server:  Received packet from client in list sending\n");
		struct message * resMsg = (struct message *) resBuffer;
		if (resMsg->messageType < 0) {
			// Client has given us permission to exit list
			logger.log("Server:  Packet from client indicates listing server can return to ready mode\n");
			cout << "List complete\n";
			break;
		}
		else {
			logger.log("Server:  Received non-end packet from client.  Resending list\n");
		}
	}
}

/** 
 * Get function:  Given a filename and a sequence number start sending file packets to client
 */
void Server::get(std::string filename, int sequence) {
	int original_sequence = sequence;
	logger.log("Server:  GET called with intial sequence of ");
	logger.log(sequence);
	logger.log("\n");
	// let client know we are about to send a file
	char buffer[BUFLEN];
	struct message * resMsg = (struct message *) buffer;

	resMsg->sequence = sequence;
	logger.log("Server:  Packet sent to client to prepare client for file packets\n");
	send(buffer);

	// now send the file
	logger.log("Server:  Lookinging for file: ");
	logger.log("Filename: ");
	char why[BODYLEN];
	strcpy_s(why, filename.c_str());
	logger.log(why);
	logger.log("\n");

	ifstream fileToRead;
	fileToRead.open(filename, ios::in | ios::binary);

	vector<string> msgV(0);
	vector<char *>msgV2(0);
	int numPackets = 0;
	if (fileToRead.is_open())
	{
		// Can make this a bit better
		// Count how many packets there will be
		char fileBuf[BODYLEN];
		while (!fileToRead.eof()) {
			memset(fileBuf, '\0', BODYLEN);
			numPackets += 1;
			fileToRead.read(fileBuf, BODYLEN - 1);
		}
		int ws = WINDOW_SIZE;
		logger.log("Server:  File found, total packets to be send is ");
		logger.log(numPackets);
		logger.log("\n");
		logger.log("Server:  Window size is ");
		logger.log(ws);
		logger.log("\n");

		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);

		msgV.resize(numPackets);
		msgV2.resize(numPackets);
		int i = 0;
		char buf[BODYLEN];
		while (!fileToRead.eof()) {
			string data;
			memset(buf, '\0', BODYLEN);
			// minus one for the null character
			fileToRead.read(buf, BODYLEN - 1);
			data = buf;
			msgV.at(i) = data;

			char * newBuf = new char[BODYLEN];
			memcpy(newBuf, buf, BODYLEN - 1);
			msgV2.at(i) = newBuf;

			i += 1;
		}
		logger.log("Server:  File has been read to memory\n");
		
	}
	fileToRead.close();



	time_t  timev;
	// We wait for response
	vector<int> timeSend(numPackets);
	vector<int> sequenceNumbers(numPackets);
	logger.log("Server:  Setting initial sequence numbers and time sents\n");
	for (int i = 0; i < numPackets; ++i) {
		timeSend.at(i) = -1;
		sequenceNumbers.at(i) = -1;
	}
	int window_size = WINDOW_SIZE;
	int packetsEnRoute = 0;
	logger.log("Server:  Sending first ");
	logger.log(window_size);
	logger.log(" packets\n");
	for (int i = 0; i < window_size; ++i) {
		if (i >= numPackets) {
			logger.log("Server:  Number of packets is less than window size\n");
			logger.log("Server:  Done sending packets\n");
			break;
		}
		string data = msgV.at(i);
		char b[BODYLEN];
		strncpy_s(b, data.c_str(), BODYLEN);

		char buffer[BUFLEN];
		struct message * m = (struct message *) buffer;
		sequence += 1;
		m->sequence = sequence;
		m->numPackets = numPackets;
		m->body[BODYLEN];
		memcpy(m->body, msgV2.at(i), BODYLEN - 1);


		time(&timev);
		timeSend.at(i) = timev;
		sequenceNumbers.at(i) = sequence;
		send(buffer);
		logger.log("Server:  Sending packet number ");
		logger.log(i);
		logger.log(" with sequence number ");
		logger.log(sequence);
		logger.log("\n");
		logger.log("Server:  Updating time sent of packet number ");
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
			exit(-1);
		}
		if (!retval) {
			logger.log("Server:  Timeout in GET\n");
			time_t time_now;
			time(&time_now);
			for (int i = 0; i < numPackets; ++i) {
				if (timeSend.at(i) != -1 && timeSend.at(i) != 0 && timeSend.at(i) < time_now) {
					// resend
					char resendBuf[BUFLEN];
					struct message * resendMsg = (struct message *) resendBuf;
					int s = sequenceNumbers.at(i);
					resendMsg->sequence = s;
					string data = msgV.at(i);

					memcpy(resendMsg->body, msgV2.at(i), BODYLEN - 1);

					resendMsg->numPackets = numPackets;
					logger.log("Server:  Resent packet ");
					logger.log(i);
					logger.log(" with sequence ");
					logger.log(resendMsg->sequence);
					logger.log("\n");
					time(&timev);
					timeSend.at(i) = timev;
					logger.log("Server:  Updating time sent of packet ");
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
			logger.log("Server:  Received acknowlegment for file packet ");
			logger.log(sucMsg->sequence);
			logger.log("\n");
			// find who we are getting res for
			for (int i = 0; i < numPackets; ++i) {
				if (sequenceNumbers.at(i) == sucMsg->sequence) {
					if (timeSend.at(i) != 0) {
						cout << ".";
						logger.log("Server:  First acknowledgment for packet ");
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
			logger.log("Server:  Looking to see if any more file packets need to be sent\n");
			if (packetsEnRoute < window_size) {
				for (int i = 0; i < numPackets; ++i) {
					if (timeSend.at(i) == -1) {
						// found our chap
						char nextSeq[BUFLEN];
						struct message * nextMsg = (struct message *) nextSeq;
						sequence += 1;
						nextMsg->sequence = sequence;
						sequenceNumbers.at(i) = sequence;
						string data = msgV.at(i);

						memcpy(nextMsg->body, msgV2.at(i), BODYLEN - 1);

						nextMsg->numPackets = numPackets;

						time_t t;
						time(&t);
						timeSend.at(i) = t;
						logger.log("Server:  Sending next packet of ");
						logger.log(i);
						logger.log(" with sequence ");
						logger.log(sequence);
						logger.log("\n");

						logger.log("Server:  Updating time sent of packet ");
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
				logger.log("Server:  All file packets sent\n");
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
		logger.log("Server:  Sending GET closing packet with sequence ");
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
			logger.log("Server:  Timeout in GET closing packet\n");
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
				logger.log("Server:  Client has acknowledge GET close\n");
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
 * Put option:  Given a filename and sequence, get ready for file packets arriving from clients
 */
void Server::put(string filename, int sequence) {
	
	char buffer[BUFLEN];
	fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS + MAX_WAIT;
	tv.tv_usec = TIMEOUT_MASECS * 2;

	while (1) {
		struct message * msg = (struct message *) buffer;
		msg->sequence = sequence;
		send(buffer);
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			cout << "Error in select";
			logger.log("Server:  Error in select\n");
			exit(-1);
		}
		if (!retval) {
			cout << "timeout\n";
			continue;
		}
		else {
			char inBuffer[BUFLEN];
			if ((recv_len = recvfrom(s, inBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
			{
				logger.log("Server:  Error recieving data from client");
				cout << "Error receiving data";
				exit(EXIT_FAILURE);
			}
			else {
				struct message * inMsg = (struct message *) inBuffer;
				if (inMsg->sequence < sequence) {
					cout << "Invalid sequence";
					continue;
				}
				else {
					return deliver(filename, sequence);
				}
			}
		}

	}

}

/**
 * Once a PUT has been identified this function receives the packet and saves it to disk
 */
void Server::deliver(string filename, int sequence) {
	vector<string> receivedPackets(0);
	vector<char *> receivedPackets2(0);
	vector<int> sequenceNums(0);
	while (true) {
		char buffer[BUFLEN];
		if ((recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Server:  recvfrom() failed\n");
			exit(EXIT_FAILURE);
		}
		cout << ".";
		// get message
		struct message * msg = (struct message *) buffer;
		if (msg->finalBit == 1) {
			cout << "\n";
			logger.log("Server:  Received file closing packet from Client, letting Client know Server has received it\n");
			char finalBuf[BUFLEN];
			struct message * finalMsg = (struct message *) finalBuf;
			finalMsg->sequence = msg->sequence;
			memset(buffer, '/0', BUFLEN);
			logger.log("Server:  Sending acknowledgment of final packet to Client with sequence of ");
			logger.log(msg->sequence);
			logger.log("\n");
			char * buffer = reliableSend(finalBuf);
			logger.log("Server:  Received acknowledgment from server that it received our acknowledgment\n");
			break;
		}
		if (msg->sequence < sequence) {
			if (msg->sequence < 0) {
				// OK, server is clearly sending us something from handshake
				logger.log("Server:  Received delayed packet from server with no sequence number, restarting GET\n");
				cout << "x";
				return put(filename, sequence);
			}
		}
		if (msg->numPackets < 0) {
			logger.log("Server:  Received delayed packet from server\n");
			// Somethings wrong, if numpackets aren't sent, we're not in right mode
			// tell server to go into get mode damnit
			char errorBuf[BUFLEN];
			struct message * errorMsg = (struct message *) errorBuf;
			errorMsg->messageType = -1;
			logger.log("Server:  Sending reset packets to server\n");
			reliableSend(errorBuf);
			memset(errorBuf, '/0', BUFLEN);
			errorMsg = (struct message *) errorBuf;
			errorMsg->messageType = 1;
			errorMsg->sequence = sequence;
			strcpy_s(errorMsg->body, filename.c_str());
			reliableSend(errorBuf);
			logger.log("Server:  Server should now be reset\n");
			cout << "x";
			continue;
		}
		if (msg->numPackets > 10000) {
			cout << "x";
			logger.log("Server:  Received a delayed packet\n");
			continue;
		}
		if (receivedPackets.size() < msg->numPackets) {
			logger.log("Server:  Received file packet from server - setting client number packets to match server");
			logger.log("Server: There will be ");
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
		logger.log("Server:  Received file packet from server with sequence ");
		logger.log(msg->sequence);
		logger.log(" and relative sequence of ");
		logger.log(actualSequence);
		logger.log("\n");
		if (actualSequence < 0) {
			cout << "x";
			logger.log("Server:  Relative sequence is out of range, sending reset packets to server\n");
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
		char * newBuf = new char[BODYLEN];
		memcpy(newBuf, msg->body, BODYLEN);
		receivedPackets2.at(actualSequence) = newBuf;
		if (sequenceNums.at(actualSequence) != -1) {
			cout << "x";
			sequenceNums.at(actualSequence) = msg->sequence;
		}
		logger.log("Server:  Received a correct file packet of sequence ");
		logger.log(msg->sequence);
		logger.log("\n");
		logger.log("Server:  Adding to received packets buffer\n");
		logger.log("Server:  Sending acknowledgment for file packet with sequence ");
		logger.log(resMsg->sequence);
		logger.log("\n");
		send(resBuffer);
	}


	logger.log("Server:  Saving ");
	char fb[BODYLEN];
	strcpy_s(fb, filename.c_str());
	logger.log(fb);
	logger.log(" to file\n");
	ofstream outFile(filename, ios::out | ios::binary);
	for (int i = 0; i < receivedPackets2.size(); ++i)
	{
		char b[BODYLEN];
		memcpy(b, receivedPackets2.at(i), BODYLEN - 1);
		outFile.write(b, BODYLEN - 1);
		memset(b, '/0', BODYLEN);
	}
	logger.log("Server:  File saving done\n");
	cout << "File transferred!\n";
	outFile.close();
}

/**
 * Return random number
 */
int Server::getRandomNumber() {
	srand((unsigned)time(0));
	int randNum = (rand() % 300) + 1;
	return randNum;
}


/**
 *  Send packet and wait for any response, if timeout - resend
 */
char * Server::reliableSend(char * buffer) {
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

/**
 * Send packet to client and wait for response with same sequence - if timeout - resend
 */
char * Server::reliableSend(char * buffer, int sequence) {
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
