/*
Simple udp client
Silver Moon (m00n.silv3r@gmail.com)
*/
#include "stdafx.h"


#include "Client.h";


using namespace std;


int main(void)
{



	Client client;

	cout << "Enter file name: ";
	char filename[200];
	gets_s(filename);
	int res = client.getFile(filename);
	if (res == 0) {
		cout << "Success";
	}
	else {
		cout << "Error";
	}
}