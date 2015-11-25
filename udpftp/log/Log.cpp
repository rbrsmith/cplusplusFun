#include "stdafx.h";
#include "Log.h"

using namespace std;


void Log::log(char * msg) {

	std::ofstream outfile;
	outfile.open(logPath, std::ios_base::app);
	outfile << msg;
}

void Log::log(int msg) {

	std::ofstream outfile;
	outfile.open(logPath, std::ios_base::app);
	outfile << msg;
}
