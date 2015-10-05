#pragma once
#include<stdio.h>
#include<winsock.h>
#include <fstream>
#include <iostream>;
#include <string>
#include <stdio.h>;

using namespace std;
#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define BUFLEN 256  //Max length of buffer
#define PORT 8889   //The port on which to listen for incoming data


class Server {

	SOCKET s;
	struct sockaddr_in server, si_other;
	int slen, recv_len;
	char buf[BUFLEN];
	WSADATA wsa;


	public:
		int start();
	private:
		int sendFile(string file);
};