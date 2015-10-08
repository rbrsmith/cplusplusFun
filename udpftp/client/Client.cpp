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
//	char tmp[BUFLEN];
//	strcpy_s(tmp, "test");
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
		if (validateSequence(resMsg->sequenceBit) != 1) {
			cout << "Error in sequence bit\n";
		}
		else {
			cout << "Server Files:\n";
			cout << "- - - - - - - \n";
			cout << resMsg->body;
		}
	}
}


int Client::validateSequence(int remoteSeq) {
	if (remoteSeq == seq) {
		increaseSequence();
		return 1;
	}
	else {
		return 0;
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