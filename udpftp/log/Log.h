#pragma once

#include <iostream>;

#include <fstream>;
#include <string>;

#define logPath  "C:\\Users\\Ross\\Documents\\Visual Studio 2015\\Projects\\log.txt"
class Log {
public:
	void log(char * msg);

	void log(int msg);

};