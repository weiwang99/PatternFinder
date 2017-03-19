#include "ProcessorStats.h"
#include "MemoryUtils.h"

mutex* ProcessorStats::countMutex = new mutex();
unsigned int ProcessorStats::globalLevel = 0;
vector<PListType> ProcessorStats::levelRecordings;
vector<PListType> ProcessorStats::currentLevelVector;
PListType ProcessorStats::eradicatedPatterns = 0;
bool ProcessorStats::processingFinished = false;
bool ProcessorStats::overMemoryCount = false;
double ProcessorStats::MemoryUsedPriorToThread = 0;
double ProcessorStats::MemoryUsageAtInception = 0;
PListType ProcessorStats::memoryCeiling = 0;
double ProcessorStats::mostMemoryOverflow = MemoryUtils::GetProgramMemoryConsumption();
double ProcessorStats::currMemoryOverflow = MemoryUtils::GetProgramMemoryConsumption();
vector<PListType> ProcessorStats::mostCommonPatternCount;
vector<PListType> ProcessorStats::mostCommonPatternIndex;
vector<bool> ProcessorStats::activeThreads;
vector<bool> ProcessorStats::usedRAM;
int ProcessorStats::threadsDispatched = 0;
int ProcessorStats::threadsDefuncted = 0;
bool ProcessorStats::firstLevelProcessedHD = false;

ProcessorStats::ProcessorStats(void)
{
}


ProcessorStats::~ProcessorStats(void)
{
}
