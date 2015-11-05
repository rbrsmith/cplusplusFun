#pragma once
#include "stdafx.h";
#include <iostream>;
#include "dirent\dirent.h";

#include<stdio.h>
#include<winsock.h>
#include<iostream>
#include<string>
#include <fstream>
#include <ctime>
#include "Log.h";

using namespace std;


#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SERVER "127.0.0.1"  //ip address of udp server
#define BODYLEN 256  
#define BUFLEN 300 //Max length of buffer
#define PORT 8889   //The port on which to listen for incoming data

class Client {

	Log logger;
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
		int finalBit;
		int errorBit;
		char body[BODYLEN];
	};
		
	public:
		Client();
		void printRemoteList();
		void getFile(string filename);
		void printLocalList();
		void sendFile(string filename);
	private:
		void send(char * buffer);
		int getRandomNumber();
		int handshake();
		int validateSequence(int remoteSeq);
		void increaseSequence();
};