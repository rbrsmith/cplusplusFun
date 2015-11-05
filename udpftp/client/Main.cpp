#include "stdafx.h";
#include "Client.h";

using namespace std;

int main() {
	int choice = -1;
	Client client;
	while (choice != 0) {
		cout << "\n--------------------\n";
		cout << "Welcome to udpftp\n";
		cout << "1 - Get Remote File Listing\n2 - Get File\n3 - Get Local File Listing\n4 - Put File\n0 - Quit";
		cout << "\n--------------------\nChoice: ";
		cin >> choice;
		switch (choice) {
			case 1:
			{
				client.printRemoteList();
				break;
			}
			case 2:
			{	
				client.printRemoteList();
				cout << "\nEnter filename to retreive: ";
				string filename;
				cin >> filename;
				cin.clear();
				client.getFile(filename);
				break;
			}
			case 3:
			{
				client.printLocalList();
				break;
			}
			case 4:
				client.printLocalList();
				cout << "\nEnter filename to send: ";
				string filename;
				cin >> filename;
				cin.clear();
				client.sendFile(filename);
				break;

		}

		
	}

}