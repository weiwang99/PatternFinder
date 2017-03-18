#include "MemoryWatchDog.h"
#include "ProcessorStats.h"
#include "ProcessorConfig.h"
#include "MemoryUtils.h"


MemoryWatchDog::MemoryWatchDog(StopWatch beginningOfTime)
{
	initTime = beginningOfTime;
}


MemoryWatchDog::~MemoryWatchDog(void)
{
	delete memQ;
}

void MemoryWatchDog::MonitorMemory()
{
	memQ = new thread(&MemoryWatchDog::QueryMemory, this);
}

void MemoryWatchDog::QueryMemory()
{
	StopWatch swTimer;
	stringstream loggingIt;
	swTimer.Start();
	while(!ProcessorStats::processingFinished)
	{
		this_thread::sleep_for(std::chrono::milliseconds(50));
		double memoryOverflow = 0;
		ProcessorStats::overMemoryCount = MemoryUtils::IsOverMemoryCount(ProcessorStats::MemoryUsedPriorToThread, (double)ProcessorConfig::memoryBandwidthMB, memoryOverflow);
		ProcessorStats::currMemoryOverflow = memoryOverflow;
		if(ProcessorStats::mostMemoryOverflow < memoryOverflow)
		{
			ProcessorStats::mostMemoryOverflow = memoryOverflow;
		}
		//Abort mission and did not exit gracefully ie dump this shit cause we will be pageing soon
		if(ProcessorStats::currMemoryOverflow + ProcessorConfig::memoryBandwidthMB > ProcessorStats::memoryCeiling)
		{
			Logger::WriteLog("Have to bail because you are using too much memory for your system!");
			exit(0);
		}

		if(swTimer.GetTime() > 10000.0f)
		{
			loggingIt.str("");
			swTimer.Start();
			loggingIt << "Thread level status...\n";
			for(int j = 0; j < ProcessorStats::currentLevelVector.size(); j++)
			{
				loggingIt << "Thread " << j << " is at level: " << ProcessorStats::currentLevelVector[j] << endl;
			}
			cout << loggingIt.str();
			Logger::WriteLog(loggingIt.str());
			loggingIt.str("");
			loggingIt << "Percentage of file processed is: " << (((double)ProcessorStats::eradicatedPatterns)/((double)ProcessorConfig::files[ProcessorConfig::f]->fileStringSize))*100.0f << "%\n";
			cout << loggingIt.str();
			Logger::WriteLog(loggingIt.str());
			loggingIt.str("");
			loggingIt << "Percentage of cpu usage: " << MemoryUtils::CPULoad() << "%\n";
			cout << loggingIt.str();
			Logger::WriteLog(loggingIt.str());
			//cout << "Memory Maps in service: " << PListArchive::mappedList.size() << endl;
			initTime.DisplayNow();
		}
	}
}
