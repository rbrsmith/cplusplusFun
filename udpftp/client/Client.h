#pragma once

#include <iostream>;

#include<stdio.h>
#include<winsock.h>
#include<iostream>
#include<string>
#include <fstream>

using namespace std;


#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define SERVER "127.0.0.1"  //ip address of udp server
#define BUFLEN 256  //Max length of buffer
#define PORT 8889   //The port on which to listen for incoming data

class Client {
	public:
		Client();
		int getFile(char * filename);
		void Client::displayList();
	private:
		char message[BUFLEN];
		struct sockaddr_in si_other;
		int s, slen = sizeof(si_other);
		char buf[BUFLEN];
		WSADATA wsa;

		int appendToFile(char * buffer, int headerBits, string filename);
		int Client::binToDec(int * bin, int size);
		


};