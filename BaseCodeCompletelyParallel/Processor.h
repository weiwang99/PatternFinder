#pragma once
#include <vector>
#include <future>
#include "StopWatch.h"
#include "ProcessorConfig.h"
#include "ProcessorStats.h"

using namespace std;

//Struct used to pass around current level data
struct LevelPackage
{
	unsigned int currLevel;
	unsigned int threadIndex;
	unsigned int inceptionLevelLOL;
	bool useRAM;
	unsigned int coreIndex;
};


template<class DataType>
class Processor
{
public:
	Processor<DataType>(void){};
	~Processor<DataType>(void){};

	virtual void PlantSeed(DataType positionInFile, DataType startPatternIndex, DataType numPatternsToSearch, DataType threadIndex) = 0;
	
	void WaitForThreads(vector<unsigned int> localWorkingThreads, vector<future<void>> *localThreadPool, bool recursive = false, unsigned int level = 0)
	{
		DataType threadsFinished = 0;
		StopWatch oneSecondTimer;
		while (threadsFinished != localThreadPool->size())
		{
			vector<unsigned int> currentThreads;
			for (DataType k = 0; k < localWorkingThreads.size(); k++)
			{
				if (localThreadPool != NULL && (*localThreadPool)[localWorkingThreads[k]].valid())
				{
					if(recursive)
					{
						(*localThreadPool)[localWorkingThreads[k]].get();

						threadsFinished++;
					}
					else
					{
						(*localThreadPool)[localWorkingThreads[k]].get();
						threadsFinished++;

						ProcessorStats::countMutex->lock();
						ProcessorStats::activeThreads[k] = false;
						ProcessorStats::countMutex->unlock();

						if(level != 0)
						{
							stringstream buff;
							buff << "Thread " << localWorkingThreads[k] << " finished all processing" << endl;
							Logger::WriteLog(buff.str());
						}
					}
				}
				else
				{
					//std::this_thread::sleep_for(std::chrono::milliseconds(1));
					currentThreads.push_back(localWorkingThreads[k]);
				}
			}
			localWorkingThreads.clear();
			for(unsigned int i = 0; i < currentThreads.size(); i++)
			{
				localWorkingThreads.push_back(currentThreads[i]);
			}
		}
	}

protected:

private:

};


