#pragma once
#include "StopWatch.h"
#include <thread>
class MemoryWatchDog
{
public:
	MemoryWatchDog(StopWatch beginningOfTime);
	~MemoryWatchDog(void);

	void MonitorMemory();
	void QueryMemory();

private:
	StopWatch initTime;
	thread *memQ;
};

