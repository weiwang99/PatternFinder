#pragma once
#include <string.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

using namespace std;
#define ARCHIVE_FOLDER LOGGERPATH
#if defined(_WIN64) || defined(_WIN32)
	#define READMEPATH "../../ReadMe.txt"
#elif defined(__linux__)
	#define READMEPATH "../ReadMe.txt"
#endif

#define GREATERTHAN4GB 0
//#define INTEGERS 1
#define BYTES 1
using namespace std;

#if GREATERTHAN4GB
typedef unsigned long long PListType;
#else
typedef unsigned long PListType ;
#endif

#if INTEGERS
typedef unsigned long long PatternType;
#endif

#if BYTES
typedef string PatternType;
#endif