#pragma once
#include <mutex>
#include <vector>
#include "TypeDefines.h"

using namespace std;

class ProcessorStats
{
public:
	ProcessorStats(void);
	~ProcessorStats(void);

	static unsigned int globalLevel;
	static mutex *countMutex;
	static PListType eradicatedPatterns;
	static bool processingFinished;
	static bool overMemoryCount;
	
	static vector<PListType> levelRecordings;
	static vector<PListType> currentLevelVector;

	static double MemoryUsedPriorToThread;
	static double MemoryUsageAtInception;

	static PListType memoryCeiling;
	static double mostMemoryOverflow;
	static double currMemoryOverflow;
	static vector<PListType> mostCommonPatternCount;
	static vector<PListType> mostCommonPatternIndex;
	static vector<bool> activeThreads;
	static vector<bool> usedRAM;

	static int threadsDispatched;
	static int threadsDefuncted;
	static bool firstLevelProcessedHD;
};

