#pragma once
#include <vector>
#include "TypeDefines.h"
#include "ProcessorConfig.h"
#include "ProcessorStats.h"
#include "PListArchive.h"
#include "Logger.h"
#include "HardDiskProcessor.h"
#include "DRAMProcessor.h"

using namespace std;
class ProcessorManager
{
public:
	ProcessorManager(void);
	~ProcessorManager(void);

	void CleanUp();
	
	void ProcessDater();

	void ProcessFirstLevelDater(PListType patternCount, PListType overallFilePosition, LevelPackage &levelInfo);

	void PrepDataFirstLevel(bool prediction, vector<vector<string>>& fileList, vector<vector<PListType>*>* prevLocalPListArray, vector<vector<PListType>*>* nextLocalPListArray);
	
	void PrepData(bool prediction, LevelPackage& levelInfo, vector<string>& fileList, vector<vector<PListType>*>* prevLocalPListArray);

	static bool PredictHardDiskOrRAMProcessing(LevelPackage levelInfo, PListType sizeOfPrevPatternCount);

	DRAMProcessor<PListType> *dramProc;
	HardDiskProcessor<PListType> *hdProc;
};

