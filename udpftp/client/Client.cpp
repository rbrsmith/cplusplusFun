
#include "stdafx.h";

#include "Client.h";
#include "dirent\dirent.h"

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
};

int Client::getFile(char * filename) {
	strcpy_s(message, "1");
	int i;
	for (i = 0; filename[i] != '\0'; ++i) {
		message[i + 1] = filename[i];
	}
	message[i + 1] = '\0';
	
	if (sendto(s, message, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	memset(buf, '\0', BUFLEN);


	string output = "";
	int count = 0;
	while (1) {
		count += 1;
		int numBytes = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen);
		if (numBytes == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}


		string file(filename);
		appendToFile(buf, 1, file);
		if (buf[0] - '0' == 1) {
			break;
		}
		memset(buf, 0, sizeof(buf));

	}

	closesocket(s);
	WSACleanup();

	return 0;
}

void Client::displayList() {
	strcpy_s(message, "0");
	message[1] = '\0';

	if (sendto(s, message, BUFLEN, 0, (struct sockaddr *) &si_other, slen) == SOCKET_ERROR)
	{
		printf("sendto() failed with error code : %d", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	memset(buf, '\0', BUFLEN);
	puts(buf);


	int count = 0;
	while (1) {
		count += 1;
		int numBytes = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen);
		if (numBytes == SOCKET_ERROR)
		{
			printf("recvfrom() failed with error code : %d", WSAGetLastError());
			exit(EXIT_FAILURE);
		}

		puts(buf);

		if (buf[0] - '0' == 1) {
			break;
		}
		memset(buf, 0, sizeof(buf));

	}

	closesocket(s);
	WSACleanup();



}

int Client::appendToFile(char * buffer, int headerBits, string filename) {
	ofstream fout(filename, ofstream::out | ofstream::app);
	if (fout.is_open())
	{
		for (int i = headerBits; i != BUFLEN; i++)
		{
			fout << buffer[i];
		}
		return 0;
	}
	else
	{
		cout << "File could not be opened." << endl;
		return -1;
	}
	
}

int Client::binToDec(int * bin, int size) {
	int val = 0;
	int i;

	for (i = 1; i <= size; ++i) {
		if (bin[i - 1] == 1) {
			if (i == 1) {
				val += 1;
			}
			int tmp = 0;
			for (int j = 0; j < i; ++j) {
				tmp = 2 * j;
			}
			val += tmp;
		}
	}
	return val;
}

void Client::showLocalList() {
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
	}
	else {
		/* could not open directory */
		perror("");
		exit(EXIT_FAILURE);
	}
}


int Client::sendFile(char * filename) {
	string file(filename);

	ifstream fileToRead;
	fileToRead.open(file, ios::in | ios::binary);
	if (fileToRead.is_open())
	{
		// Can make this a bit better
		// Count how many packets there will be
		char fakeBuf[BUFLEN - 1];
		int numPackets = 0;
		while (!fileToRead.eof()) {
			memset(fakeBuf, '\0', BUFLEN - 1);
			numPackets += 1;
			fileToRead.read(buf, BUFLEN - 1);
		}

		fileToRead.clear();
		fileToRead.seekg(0, ios::beg);
		int packetCount = 1;

		// SEND FIRST REQ
		/* Transfer the content to requested client */
		buf[0] = '2';
		if (sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
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
		// END SEND


		char newbuf[BUFLEN - 1];
		while (!fileToRead.eof())
		{
			memset(newbuf, '\0', BUFLEN - 1);
			/* Read the contents of file and write into the buffer for transmission */
			fileToRead.read(newbuf, BUFLEN-1);
			char finalBuf[BUFLEN];
			// Indicate ur sending a file
			if (packetCount == numPackets) {
				finalBuf[0] = '1';
			}
			else {
				finalBuf[0] = '0';
			}
			for (int i = 0; i < BUFLEN - 1; i++) {
				finalBuf[i + 1] = newbuf[i];
			}



			/* Transfer the content to requested client */
			if (sendto(s, finalBuf, BUFLEN, 0, (struct sockaddr*) &si_other, slen) == SOCKET_ERROR)
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