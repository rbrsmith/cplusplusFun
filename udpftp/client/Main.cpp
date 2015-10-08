#include "stdafx.h";
#include "Client.h";

using namespace std;

int main() {
	int choice = -1;
	Client client;
	while (choice != 0) {
		cout << "\n--------------------\n";
		cout << "Welcome to udpftp\n";
		cout << "1 - Get Remote File Listing\n0 - Quit";
		cout << "\n--------------------\nChoice: ";
		cin >> choice;
		switch (choice) {
			case 1:
				client.printRemoteList();
				break;
		}

		
	}

}