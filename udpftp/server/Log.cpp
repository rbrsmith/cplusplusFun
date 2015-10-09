#include "stdafx.h";
#include "Log.h"

using namespace std;



void Log::log(char * msg) {
	cout << msg;
	std::ofstream outfile;
	outfile.open("log.txt", std::ios_base::app);
	outfile << msg;
}

void Log::log(int msg) {
	cout << msg;
	std::ofstream outfile;
	outfile.open("log.txt", std::ios_base::app);
	outfile << msg;
}
