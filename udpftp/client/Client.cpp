
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

int Client::appendToFile(char * buffer, int headerBits, string filename) {
	ofstream fout(filename, ofstream::out | ofstream::app);
	if (fout.is_open())
	{
		for (int i = headerBits; i != BUFLEN; i++)
		{
			fout << buffer[i]; //appending ith character of array in the file
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
