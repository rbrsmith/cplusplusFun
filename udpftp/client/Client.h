#pragma once
#include "stdafx.h";
#include <iostream>;

#include<stdio.h>
#include<winsock.h>
#include<iostream>
#include<string>
#include <fstream>
#include <ctime>

using namespace std;


#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SERVER "127.0.0.1"  //ip address of udp server
#define BODYLEN 256  //Max length of buffer
#define BUFLEN 300
#define PORT 8889   //The port on which to listen for incoming data

class Client {
		
	struct sockaddr_in si_other;
	int s, slen = sizeof(si_other);
	int recv_len;
	WSADATA wsa;

	int seq;

	struct message {
		int messageType;
		int SYN;
		int ACK;
		int sequenceBit;
		char body[BODYLEN];
	};

	public:
		Client();
		void printRemoteList();
	private:
		void send(char * buffer);
		int getRandomNumber();
		int handshake();
		int validateSequence(int remoteSeq);
		void increaseSequence();
};