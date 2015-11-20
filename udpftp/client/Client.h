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
#include <vector>
#include "Log.h";

using namespace std;


#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SERVER "127.0.0.1"  //ip address of udp server
#define BODYLEN 256  
#define BUFLEN 2048 //Max length of buffer
#define IN_PORT 5000 
#define OUT_PORT 7000
#define TIMEOUT_SECS 0
#define TIMEOUT_MASECS 500000
#define MAX_WAIT 2
#define WINDOW_SIZE 5;

class Client {

	Log logger;
	struct sockaddr_in si_in;
	struct sockaddr_in si_out;
	int s, slen = sizeof(si_out);
	int recv_len;
	int s2 = sizeof(si_out);
	int s3 = sizeof(si_out);
	WSADATA wsa;

	int seq;

	struct message {
		int messageType;
		int SYN;
		int ACK;
		int sequence;
		int finalBit;
		int errorBit;
		int testBit;
		int testNum;
		int numPackets;
		char body[BODYLEN];
	};

public:
	Client();
	void printRemoteList();
	void getFile(string filename);
	void printLocalList();
	void sendFile(string filename);
	void sendPackets(vector<message> msgV, int numberOfPackets, int windowSize, int timeout);
private:
	void send(char * buffer);
	void deliverFile(int sequence, string filename);
	char * reliableSend(char * buffer, int sequence);
	char * reliableSend(char * buffer);
	int getRandomNumber();
	int handshake();
};