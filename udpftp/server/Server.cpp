#include "stdafx.h"
#include "Server.h"

using namespace std;



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
	
	while(1) {
		cout << "Server ready and waiting\n";
		struct message * msg = getDataFromClient(serverBuf);
		cout << "SEQ?>>? " << msg->sequence << "\n";
		cout << "MESSAGE TYPE IS " << msg->messageType << "\n";
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
				cout << "BODY : " << filename << "\n";
				logger.log("Server:  Get operation called...\n");
				cout << "GET\n";
				get(filename, seq);
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
						cout << "Handdshake completed 1\n";
						return 0;
					}
					if (retval) {
						char buffer[BUFLEN];
						recv_len = recvfrom(s, buffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen);
						struct message * m = (struct message *) buffer;
						if (m->messageType >= 0) {
							cout << "Handshake completed 2\n";
							memcpy(fillMe, buffer, BUFLEN);
							logger.log("Server:  Client has send a message with a message type, indicating the client moved on from handshake. Handshae success\n");
							return 1;
						}
						cout << "Recevied unexpeccted packet, wait for a timeout\n";
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
			// I dont even know who you are
			cout << "Unknown packet in handshake, has no syn or ack and its sequence is " << rec_msg->sequence << " and message type is " << rec_msg->messageType << "\n";
			logger.log("Server:  Received unknown message in handshake.  Sending back empty response\n");
			
		//	if (rec_msg->sequence >= 0 && rec_msg->sequence >= 0) {
		//		cout << "THere - weird, a sequence is set - lets send upwards\n";
		//		memcpy(fillMe, buffer, BUFLEN);
		//		return 1;
		//	}

			send(failBuffer);
			return -1;
		}

	}

}

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
		cout << "HELP ME\n";
		struct message * m = (struct message *) fillMe;
		cout << "SEQ: " << m->sequence << "\n";
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
			cout << "Received data";
			cout << resmsg->messageType << "\n";
			messageType = resmsg->messageType;
		}
	}
	cout << "Incoming data\n";
	logger.log("Server:  Incoming data from client...\n");

	return resmsg;
	
}

void Server::list(int sequence) {
	cout << "LIST SEQ " << sequence << "\n";
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
	sendMsg->sequence = sequence;
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
		cout << "Sending list\n";
		cout << sequence << "\n";
		send(sendBuffer);
		logger.log("Server:  List of files sent to client\n");

		char resBuffer[BUFLEN];
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Server:  Error recieving data from client");
			cout << "Error receiving data";
			exit(EXIT_FAILURE);
		}
		struct message * resMsg = (struct message *) resBuffer;
		if (resMsg->messageType < 0) {
			// Client has given us permission to exit list
			cout << "List complete\n";
			logger.log("Server:  Received end packet from client.  Exiting list loop\n");
			break;
		}
		else {
			logger.log("Server:  Received non-end packet from client.  Resending list\n");
		}
	}
}

void Server::get(std::string filename, int sequence) {
	cout << "Get called with sequence " << sequence << "\n";
	cout << "BODY 2" << filename << "\n";
	// let client know we are about to send a file
	char buffer[BUFLEN];
	struct message * resMsg = (struct message *) buffer;

	resMsg->sequence = sequence;
	send(buffer);
	cout << "I sent pack the acknoledgment that i know what file ur looking for\n";

	// now send the file
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
		char fileBuf[BODYLEN];
		int numPackets = 0;
		while (!fileToRead.eof()) {
			memset(fileBuf, '\0', BODYLEN);
			numPackets += 1;
			fileToRead.read(fileBuf, BODYLEN);
		}

		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);

		vector<string> msgV(numPackets);
		int i = 0;
		while (!fileToRead.eof()) {
			string data;
			char buf[BODYLEN];
			memset(buf, '\0', BODYLEN);
			fileToRead.read(buf, BODYLEN);
			data = buf;
			char b[BODYLEN];
//			strcpy_s(b, data.c_str());

			msgV.at(i) = data;

			i += 1;
		}


		logger.log("Server:  Sending ");
		logger.log(numPackets);
		logger.log(" packets to server\n");
		for (i = 0; i < msgV.size(); ++i) {
			string data = msgV.at(i);

			char b[BODYLEN];
			strncpy_s(b, data.c_str(), _TRUNCATE);
			cout << b << "__END__";
			
			
			char buffer[BUFLEN];
			struct message * m = (struct message *) buffer;
			sequence += 1;
			m->sequence = sequence;
			m->body[BODYLEN];
			strncpy_s(m->body, data.c_str(), _TRUNCATE);
			send(buffer);
		}
		cout << "DONE FILE\n";

	}
	



	/*fd_set rfds;
	struct timeval tv;
	int retval;
	tv.tv_sec = TIMEOUT_SECS + MAX_WAIT;
	tv.tv_usec = TIMEOUT_MASECS * 2;
	while (true) {

		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		retval = select(1, &rfds, NULL, NULL, &tv);
	*/
	
	
	//string filename = msg->body;


	//	int packetCount = 1;

	//	char buf[BUFLEN];
	//	
	//	while (!fileToRead.eof())
	//	{
	//		memset(buf, '\0', BODYLEN);


	//		struct message * sendMsg = (struct message *) buf;
	//		increaseSequence();

	//		/* Read the contents of file and write into the buffer for transmission */
	//		fileToRead.read(sendMsg->body, BODYLEN);

	//		if (packetCount == numPackets) {
	//			logger.log("Server:  Last packet, setting final bit\n");
	//			sendMsg->finalBit = 1;
	//		}
	//		else {
	//			sendMsg->finalBit = 0;
	//		}

	//		send(buf);
	//		logger.log("Server:  Sending file packet\n");
	//		packetCount += 1;
	//	}
	//	cout << "File transfer complete\n";
	//	logger.log("Server:  File transfer complete\n");
	//}
	//else
	//{
	//	char buf[BUFLEN];
	//	struct message * sendMsg = (struct message *) buf;
	//	increaseSequence();
	//	sendMsg->errorBit = 1;
	//	logger.log("Server:  Unable to find file\n");
	//	cout << "Unable to find file\n";
	//	strcpy_s(sendMsg->body, "File was not found");
	//	send(buf);
	//}
	//fileToRead.close();
}

void Server::put(struct message * msg) {
	
	string filename = msg->body;

	ofstream outFile(filename, ios::out | ios::binary);

	// First package is filename
	char resBuffer[BUFLEN];
	while (1) {
		if ((recv_len = recvfrom(s, resBuffer, BUFLEN, 0, (struct sockaddr *) &si_in, &slen)) == SOCKET_ERROR)
		{
			logger.log("Server:  Error recieving data from client");
			cout << "Error receiving data";
			exit(EXIT_FAILURE);
		}
		else {
			struct message *resMsg = (struct message *) resBuffer;
			
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
	int randNum = (rand() % 300) + 1	;
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
