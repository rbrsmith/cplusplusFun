#include "stdafx.h"
#include <iostream>
#include<winsock.h>
#include <fstream>
#include <iostream>;
#include <string>
#include <stdio.h>;
#include <ctime>;
#include "dirent\dirent.h"

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define BUFLEN 300  //Max length of buffer
#define BODYLEN 256
#define PORT 8889   //The port on which to listen for incoming data

class Server {
	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	//char buf[BUFLEN];
	WSADATA wsa;

	int seq;


	struct message {
		int messageType;
		int SYN;
		int ACK;
		int sequenceBit;
		char body[BODYLEN];
	};
	int bufLen = sizeof(message);


	public:
		void start();
	private:
		struct message * getDataFromClient(char *serverBuf);
		int handshake();
		int getRandomNumber();
		void send(char * buffer);
		void list(int sequence);
		int validateSequence(int remoteSeq);
		void increaseSequence();
};