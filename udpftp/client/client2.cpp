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
	cout << "1 - Remote List\n2 - Get\n3-Local List\n4 - Put";
	int choice;
	cin >> choice;
	cin.get();
	cin.clear();
	if (choice == 1) {
		client.displayList();
	}
	else if (choice == 2) {
		cout << "GET";
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
	else if (choice == 3) {
		cout << "LOCAL LIST";
		client.showLocalList();
	}
	else if (choice == 4) {
		cout << "PUT";
		cout << "Enter file name: ";
		char filename[200];
		gets_s(filename);
		cout << filename;
		client.sendFile(filename);
	}

}