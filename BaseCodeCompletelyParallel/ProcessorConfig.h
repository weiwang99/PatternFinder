#pragma once
#include <vector>
#include "FileReader.h"
#include "dirent.h"

class ProcessorConfig
{
public:
	ProcessorConfig(void);
	~ProcessorConfig(void);

	static void CommandLineParser(int argc, char **argv); 
	static void FindFiles(string directory);
	static void DisplayHelpMessage();

	static unsigned int numThreads;
	static vector<FileReader*> files;
	static int f;
	static string patternToSearchFor;
	static vector<PListType> fileSizes;
	static unsigned int levelToOutput;
	static int history;
	static PListType minimum, maximum;
	static PListType memoryBandwidthMB;
	static PListType memoryPerThread;
	static bool displayEachLevelSearch;
	static bool findBestThreadNumber;
	static bool usingMemoryBandwidth;
	static unsigned int testIterations;
	static bool usingPureRAM;
	static bool usingPureHD;
	static bool outlierScans;
	static PListType minOccurrence;

private:
};

