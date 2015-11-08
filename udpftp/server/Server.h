#include "stdafx.h"
#include <iostream>
#include<winsock.h>
#include <fstream>
#include <iostream>;
#include <string>
#include <stdio.h>;
#include <ctime>;
#include "dirent\dirent.h"
#include "Log.h";

#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define BUFLEN 2048  //Max length of buffer
#define BODYLEN 256
#define IN_PORT 5001 
#define OUT_PORT 7001
#define SERVER "127.0.0.1"  //ip address of udp server
#define TIMEOUT 1

class Server {
	SOCKET s;
	struct sockaddr_in si_in;
	struct sockaddr_in si_out;
	int slen = sizeof(si_out);
	int recv_len;
	WSADATA wsa;

	int seq;
	Log logger;

	struct message {
		int messageType;
		int SYN;
		int ACK;
		int sequenceBit;
		int finalBit;
		int errorBit;
		int testBit;
		int testNum;
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
		void get(struct message * msg);
		void put(struct message * msg);
};