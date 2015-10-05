#include "stdafx.h";
#include "Server.h";

int Server::start() {

	slen = sizeof(si_other);

	//Initialise winsock
	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}
	printf("Socket created.\n");

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
	puts("Bind done");
	//keep listening for data
	while (1)
	{
		printf("Waiting for data...");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);
		int typeBit = buf[0] - '0';
		if (typeBit == 1) {
			cout << "Send file";
			
			char newBuf[BUFLEN];
			int i;
			for (i= 0; buf[i] != '\0'; ++i) {
				newBuf[i] = buf[i+1];
			}
			newBuf[i + 1] = '\0';
			string file(newBuf);
			sendFile(file);
		}
		else {
			cout << "List";
		}
	}

	closesocket(s);
	WSACleanup();
	return 0;

}

int Server::sendFile(string file) {
	
	ifstream fileToRead;
	fileToRead.open(file, ios::in | ios::binary);
	if (fileToRead.is_open())
	{
		// Can make this a bit better
		// Count how many packets there will be
		char fakeBuf[BUFLEN-1];
		int numPackets = 0;
		while (!fileToRead.eof()) {
			memset(fakeBuf, '\0', BUFLEN - 1);
			numPackets += 1;
			fileToRead.read(buf, BUFLEN - 1);
		}

		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);
		int packetCount = 1;
		while (!fileToRead.eof())
		{
			memset(buf, '\0', BUFLEN-1);
			/* Read the contents of file and write into the buffer for transmission */
			fileToRead.read(buf, BUFLEN);
			char finalBuf[BUFLEN];
			if (packetCount == numPackets) {
				finalBuf[0] = '1';
			}
			else {
				finalBuf[0] = '0';
			}
			for (int i = 0; i < BUFLEN - 1; ++i) {
				finalBuf[i + 1] = buf[i];
			}
			
			/* Transfer the content to requested client */
			if (sendto(s, finalBuf, recv_len, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
			{
				printf("sendto() failed with error code : %d", WSAGetLastError());
				/* Close the connection and unlock the mutex if there is a Socket Error */
				closesocket(s);

				return -1;
			}
			else
			{
				/* Reset the buffer and use the buffer for next transmission */
				memset(buf, '\0', sizeof(buf));
			}
			packetCount += 1;
		}
		printf("File transferred");
	}
	else
	{
		printf("FILE ERROR");
	}
	fileToRead.close();

	
	return 0;

}
