#pragma once
#include "Processor.h"

template<class DataType>
class DRAMProcessor : Processor<DataType>
{
public:
	DRAMProcessor<DataType>(void)
	{
		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		prevPListArray = new vector<vector<DataType>*>();
		globalPListArray = new vector<vector<DataType>*>();

		//Initialize all possible values for the first list to NULL
		prevPListArray->resize(256*threadsToDispatch);
		for(int i = 0; i < 256*threadsToDispatch; i++)
		{
			(*prevPListArray)[i] = NULL;
		}
	}
	~DRAMProcessor<DataType>(void)
	{
		Cleanup();

		delete prevPListArray;
		delete globalPListArray;
	}

	void Cleanup()
	{
		for (int i = 0; i < prevPListArray->size(); i++)
		{
			if((*prevPListArray)[i] != NULL)
			{
				delete (*prevPListArray)[i];
			}
		}
		prevPListArray->clear();

		for (int i = 0; i < globalPListArray->size(); i++)
		{
			if((*globalPListArray)[i] != NULL)
			{
				delete (*globalPListArray)[i];
			}
		}
		globalPListArray->clear();
	}

	vector<vector<PListType>> BalanceLoad(int threadsToDispatch, vector<vector<PListType>*>* patterns)
	{
		vector<vector<PListType>> balancedList(threadsToDispatch);
		vector<PListType> balancedSizeList;
		for(PListType i = 0; i < threadsToDispatch; i++)
		{
			balancedSizeList.push_back(0);
		}

		if(patterns->size() == threadsToDispatch*256)
		{
			vector<PListType> patternTotals(256, 0);
			for(PListType i = 0; i < 256; i++)
			{
				for(PListType z = 0; z < threadsToDispatch; z++)
				{
					patternTotals[i] += (*patterns)[i*threadsToDispatch + z]->size();
				}
			}

			for(PListType i = 0; i < patternTotals.size(); i++)
			{
				bool found = false;
				PListType smallestIndex = 0;
				PListType smallestAmount = -1;
				for(PListType j = 0; j < threadsToDispatch; j++)
				{
					if((*patterns)[(i*threadsToDispatch) + j] != NULL)
					{
						for(PListType z = 0; z < threadsToDispatch; z++)
						{
							if(balancedSizeList[z] < smallestAmount)
							{
								smallestAmount = balancedSizeList[z];
								smallestIndex = z;
								found = true;
							}
						}

					}
				}
				if(found)
				{
					balancedSizeList[smallestIndex] += patternTotals[i];
					for(PListType j = 0; j < threadsToDispatch; j++)
					{
						balancedList[smallestIndex].push_back((i*threadsToDispatch) + j);
					}
				}
			}
		}
		else
		{
			vector<PListType> patternTotals;
			for(PListType i = 0; i < patterns->size(); i++)
			{
				patternTotals.push_back((*patterns)[i]->size());
			}

			for(PListType i = 0; i < patternTotals.size(); i++)
			{
				bool found = false;
				PListType smallestIndex = 0;
				PListType smallestAmount = -1;
			
				if((*patterns)[i] != NULL)
				{
					for(PListType z = 0; z < threadsToDispatch; z++)
					{
						if(balancedSizeList[z] < smallestAmount)
						{
							smallestAmount = balancedSizeList[z];
							smallestIndex = z;
							found = true;
						}
					}

				}
			
				if(found)
				{
					balancedSizeList[smallestIndex] += patternTotals[i];
					balancedList[smallestIndex].push_back(i);
				}
			}
		}

		PListType sizeToTruncate = 0;
		for(unsigned int i = 0; i < threadsToDispatch; i++)
		{
			if(balancedList[i].size() > 0)
			{
				sizeToTruncate++;
			}
		}

		vector<vector<PListType>> balancedTruncList(sizeToTruncate);
		PListType internalCount = 0;
		for(unsigned int i = 0; i < threadsToDispatch; i++)
		{
			if(balancedList[i].size() > 0)
			{
				balancedTruncList[internalCount] = balancedList[i];
				internalCount++;
			}
		}

		return balancedTruncList;
	}

	void ThreadBranch(vector<vector<PListType>*>* patterns, vector<PListType> patternIndexList, vector<string> fileList, LevelPackage levelInfo)
	{
		PListType numPatternsToSearch = patternIndexList.size();
		bool isThreadDefuncted = false;
		//cout << "Threads dispatched: " << threadsDispatched << " Threads deported: " << threadsDefuncted << " Threads running: " << threadsDispatched - threadsDefuncted << endl;

		int tempCurrentLevel = levelInfo.currLevel;
		int threadsToDispatch = ProcessorConfig::numThreads - 1;

		if(ProcessorStats::threadsDispatched - ProcessorStats::threadsDefuncted > threadsToDispatch)
		{
			cout << "WENT OVER THREADS ALLOCATION SIZE!" << endl;
		}

		vector<vector<PListType>*>* prevLocalPListArray = new vector<vector<PListType>*>();
		vector<vector<PListType>*>*	globalLocalPListArray = new vector<vector<PListType>*>();

		if(levelInfo.useRAM)
		{
			for(PListType i = 0; i < numPatternsToSearch; i++)
			{
				if((*patterns)[patternIndexList[i]] != NULL)
				{
					prevLocalPListArray->push_back((*patterns)[patternIndexList[i]]);
				}
			}
		}

		bool continueSearching = true;

		while(continueSearching)
		{
			bool useRAMBRO = true;
			if(levelInfo.currLevel != 2)
			{
				//useRAMBRO = !NextLevelTreeSearchRecursion(prevLocalPListArray, globalLocalPListArray, fileList, levelInfo);
			}
			else
			{
				useRAMBRO = levelInfo.useRAM;
			}

			if(useRAMBRO)
			{
				continueSearching = Process(prevLocalPListArray, globalLocalPListArray, levelInfo, isThreadDefuncted);
			}
			else
			{
				//continueSearching = ProcessHD(levelInfo, fileList, isThreadDefuncted);
			}
		}

		if(prevLocalPListArray != NULL)
		{
			delete prevLocalPListArray;
		}

		if(globalLocalPListArray != NULL)
		{
			delete globalLocalPListArray;
		}

		ProcessorStats::countMutex->lock();
		if(ProcessorStats::currentLevelVector[levelInfo.threadIndex] > ProcessorStats::globalLevel)
		{
			ProcessorStats::globalLevel = ProcessorStats::currentLevelVector[levelInfo.threadIndex];
		}
		if(!isThreadDefuncted)
		{
			ProcessorStats::threadsDefuncted++;
		}
		ProcessorStats::countMutex->unlock();

		return;
	}

	void BuildThreadJobs(DataType level)
	{
		vector<future<void>> *threadPool = new vector<future<void>>();
		//use that one prediction
		if(ProcessorStats::usedRAM[0])
		{
			vector<vector<PListType>> balancedTruncList = BalanceLoad(ProcessorConfig::numThreads - 1, prevPListArray);
			vector<unsigned int> localWorkingThreads;
			for(unsigned int i = 0; i < balancedTruncList.size(); i++)
			{
				ProcessorStats::activeThreads[i] = true;
				localWorkingThreads.push_back(i);
			}

			ProcessorStats::countMutex->lock();
			for (unsigned int i = 0; i < localWorkingThreads.size(); i++)
			{
				LevelPackage levelInfo;
				levelInfo.currLevel = level;
				levelInfo.threadIndex = i;
				levelInfo.inceptionLevelLOL = 0;
				levelInfo.useRAM = true;
				levelInfo.coreIndex = i;
				ProcessorStats::threadsDispatched++;
				vector<string> temp2;

				threadPool->push_back(std::async(std::launch::async, &DRAMProcessor::ThreadBranch, this, prevPListArray, balancedTruncList[i], temp2, levelInfo));
	
			}
			ProcessorStats::countMutex->unlock();

			Processor<DataType>::WaitForThreads(localWorkingThreads, threadPool);
		}
	}

	bool Process(vector<vector<DataType>*>* prevLocalPListArray, vector<vector<DataType>*>* globalLocalPListArray, LevelPackage& levelInfo, bool& isThreadDefuncted)
	{
		//Keeps track of all pLists in one contiguous block of memory
		vector<PListType> linearList;
		//Keeps track of the length of each pList
		vector<PListType> pListLengths;

		//Keeps track of all pLists in one contiguous block of memory
		vector<PListType> prevLinearList;
		//Keeps track of the length of each pList
		vector<PListType> prevPListLengths;

		bool continueSearching = true;
		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		PListType totalTallyRemovedPatterns = 0;
		PListType newPatterns = 0;
		string globalStringConstruct;
		vector<PListType> pListTracker;
		PListType fileSize = ProcessorConfig::files[ProcessorConfig::f]->fileStringSize;
		PListType totalCount = 0;
		if(levelInfo.currLevel == 2)
		{
			int threadCountage = 0;

			for (PListType i = 0; i < prevLocalPListArray->size(); i++)
			{
				PListType pListLength = (*prevLocalPListArray)[i]->size();
				if(pListLength > 0)
				{
					totalCount += pListLength;
				}
			}
			prevLinearList.reserve(totalCount);

			for (PListType i = 0; i < prevLocalPListArray->size(); i++)
			{
				PListType pListLength = (*prevLocalPListArray)[i]->size();
				if(pListLength > 0)
				{
					threadCountage += pListLength;
					prevLinearList.insert(prevLinearList.end(), (*prevLocalPListArray)[i]->begin(), (*prevLocalPListArray)[i]->end());
					delete  (*prevLocalPListArray)[i];
				}
				else
				{
					delete (*prevLocalPListArray)[i];
				}

				if(/*!firstLevelProcessedHD*/false)
				{
					if(i % threadsToDispatch == (threadsToDispatch - 1) && threadCountage > 1)
					{
						prevPListLengths.push_back(threadCountage);
						threadCountage = 0;
					}
				}
				else
				{
					prevPListLengths.push_back(threadCountage);
					threadCountage = 0;
				}
			}
			//delete prevLocalPListArray;
		}
		else
		{

			for (PListType i = 0; i < prevLocalPListArray->size(); i++)
			{
				PListType pListLength = (*prevLocalPListArray)[i]->size();
				if(pListLength > 0)
				{
					totalCount += pListLength;
				}
			}

			prevLinearList.reserve(totalCount);

			for (PListType i = 0; i < prevLocalPListArray->size(); i++)
			{
				PListType pListLength = (*prevLocalPListArray)[i]->size();
				if(pListLength > 1)
				{
					prevLinearList.insert(prevLinearList.end(), (*prevLocalPListArray)[i]->begin(), (*prevLocalPListArray)[i]->end());
					prevPListLengths.push_back(pListLength);
					delete (*prevLocalPListArray)[i];
				}
				else
				{
					delete (*prevLocalPListArray)[i];
				}
				//delete (*prevLocalPListArray)[i];
			}
			//delete prevLocalPListArray;
		}
		globalStringConstruct.resize(totalCount);
		linearList.reserve(totalCount);

		PListType linearListIndex = 0;

		PListType globalStride = 0;

		//We have nothing to process!
		if(totalCount == 0)
			return false;

		while(continueSearching)
		{

			PListType stride = 0;
			PListType strideCount = 0;
			PListType prevStride = 0;

			totalTallyRemovedPatterns = 0;
			PListType minIndex = -1;
			PListType maxIndex = 0;
			PListType stringIndexer = 0;

			if(levelInfo.currLevel == 2)
			{
				PListType linearListSize = prevLinearList.size();
				if(linearListSize > 0)
				{
					for (PListType i = 0; i < linearListSize; i++)
					{
						if (prevLinearList[i] < fileSize)
						{
							globalStringConstruct[stringIndexer++] = ProcessorConfig::files[ProcessorConfig::f]->fileString[prevLinearList[i]];

							stride += abs((long long)(prevLinearList[i] - prevStride));
							strideCount++;
							prevStride = prevLinearList[i];
						}
					}
				}
			}
			else
			{
				PListType linearListSize = prevLinearList.size();
				if(linearListSize > 1)
				{
					for (PListType i = 0; i < linearListSize; i++)
					{
						if (prevLinearList[i] < fileSize)
						{
							globalStringConstruct[stringIndexer++] = ProcessorConfig::files[ProcessorConfig::f]->fileString[prevLinearList[i]];

							stride += abs((long long)(prevLinearList[i] - prevStride));
							strideCount++;
							prevStride = prevLinearList[i];
						}
					}
				}
			}
		
			if(globalStride == 0)
			{
				globalStride = (stride / strideCount);
			}
			else
			{
				globalStride += (stride / strideCount);
				globalStride /= 2.0f;
			}

			if(prevPListLengths.size() == 0)
			{
				continueSearching = false;
				break;
			}

			globalStringConstruct.resize(stringIndexer);
			stringIndexer = 0;
			vector<PListType> newPList[256];

			if(levelInfo.currLevel == 2)
			{
				PListType prevPListSize = prevLinearList.size();
				PListType indexes[256] = {0};
				PListType indexesToPush[256] = {0};
				//Banking off very random patterns
				PListType firstPatternIndex[256] = {0};
				PListType prevSize = 0;
				int listLength = 0;


				int currList = 0;
				int currPosition = 0;
				int limit = prevPListLengths[currList];
				for (PListType i = 0; i < prevPListSize; i++)
				{
					//If pattern is past end of string stream then stop counting this pattern
					PListType index = prevLinearList[i];
					if (index < fileSize)
					{
						uint_fast8_t indexIntoFile = (uint8_t)globalStringConstruct[stringIndexer++];
						if(firstPatternIndex[indexIntoFile])
						{
							if(newPList[indexIntoFile].empty())
							{
								newPList[indexIntoFile].push_back(firstPatternIndex[indexIntoFile]);
							}
							newPList[indexIntoFile].push_back(index + 1);	
							indexes[indexIntoFile]++;
						}
						else
						{
							firstPatternIndex[indexIntoFile] = index + 1;
							indexes[indexIntoFile]++;
							indexesToPush[listLength++] = indexIntoFile;
						}
					}
					else
					{
						totalTallyRemovedPatterns++;
					}

					if(i + 1 == currPosition + limit)
					{
						for (PListType k = 0; k < listLength; k++)
						{
							int insert = indexes[indexesToPush[k]];
							if (insert >= ProcessorConfig::minOccurrence)
							{
								pListLengths.push_back(newPList[indexesToPush[k]].size());
								linearList.insert(linearList.end(), newPList[indexesToPush[k]].begin(), newPList[indexesToPush[k]].end());

								indexes[indexesToPush[k]] = 0;
								firstPatternIndex[indexesToPush[k]] = 0;
								newPList[indexesToPush[k]].clear();
							}
							else if(insert == 1)
							{
								totalTallyRemovedPatterns++;
								indexes[indexesToPush[k]] = 0;
								firstPatternIndex[indexesToPush[k]] = 0;
								newPList[indexesToPush[k]].clear();
							}

						}
						if(currList + 1 < prevPListLengths.size())
						{
							currPosition = i + 1;
							currList++;
							limit = prevPListLengths[currList];
							listLength = 0;
						}
					}
				}
			}
			else
			{
				PListType prevPListSize = prevLinearList.size();
				PListType indexes[256] = {0};
				PListType indexesToPush[256] = {0};
				//Banking off very random patterns
				PListType firstPatternIndex[256] = {0};
				PListType prevSize = 0;
				int listLength = 0;


				int currList = 0;
				int currPosition = 0;
				int limit = prevPListLengths[currList];
				for (PListType i = 0; i < prevPListSize; i++)
				{
					//If pattern is past end of string stream then stop counting this pattern
					PListType index = prevLinearList[i];
					if (index < fileSize)
					{
						uint_fast8_t indexIntoFile = (uint8_t)globalStringConstruct[stringIndexer++];
						if(firstPatternIndex[indexIntoFile])
						{
							if(newPList[indexIntoFile].empty())
							{
								newPList[indexIntoFile].push_back(firstPatternIndex[indexIntoFile]);
							}
							newPList[indexIntoFile].push_back(index + 1);	
							indexes[indexIntoFile]++;
						}
						else
						{
							firstPatternIndex[indexIntoFile] = index + 1;
							indexes[indexIntoFile]++;
							indexesToPush[listLength++] = indexIntoFile;
						}
					}
					else
					{
						totalTallyRemovedPatterns++;
					}

					if(i + 1 == currPosition + limit)
					{
						for (PListType k = 0; k < listLength; k++)
						{
							int insert = indexes[indexesToPush[k]];
							if (insert >= ProcessorConfig::minOccurrence)
							{
								pListLengths.push_back(newPList[indexesToPush[k]].size());
								linearList.insert(linearList.end(), newPList[indexesToPush[k]].begin(), newPList[indexesToPush[k]].end());

								indexes[indexesToPush[k]] = 0;
								firstPatternIndex[indexesToPush[k]] = 0;
								newPList[indexesToPush[k]].clear();
							}
							else if(insert == 1)
							{
								totalTallyRemovedPatterns++;
								indexes[indexesToPush[k]] = 0;
								firstPatternIndex[indexesToPush[k]] = 0;
								newPList[indexesToPush[k]].clear();
							}

						}
						if(currList + 1 < prevPListLengths.size())
						{
							currPosition = i + 1;
							currList++;
							limit = prevPListLengths[currList];
							listLength = 0;
						}
					}
				}
			}

			ProcessorStats::countMutex->lock();

			ProcessorStats::eradicatedPatterns += totalTallyRemovedPatterns;

			if(ProcessorStats::levelRecordings.size() < levelInfo.currLevel)
			{
				ProcessorStats::levelRecordings.resize(levelInfo.currLevel);
			}
			ProcessorStats::levelRecordings[levelInfo.currLevel - 1] += pListLengths.size();

			if(ProcessorStats::mostCommonPatternIndex.size() < levelInfo.currLevel)
			{
				ProcessorStats::mostCommonPatternIndex.resize(levelInfo.currLevel);
				ProcessorStats::mostCommonPatternCount.resize(levelInfo.currLevel);
			}
					
			PListType countage = 0;
			for (PListType i = 0; i < pListLengths.size(); i++)
			{
				if(pListLengths[i] > 0)
				{
					if( pListLengths[i] > ProcessorStats::mostCommonPatternCount[levelInfo.currLevel - 1])
					{
						ProcessorStats::mostCommonPatternCount[levelInfo.currLevel - 1] = pListLengths[i];
						ProcessorStats::mostCommonPatternIndex[levelInfo.currLevel - 1] = linearList[countage] - levelInfo.currLevel;
					}
				}
				countage += pListLengths[i];
			}

			levelInfo.currLevel++;

			if(levelInfo.currLevel > ProcessorStats::currentLevelVector[levelInfo.threadIndex])
			{
				ProcessorStats::currentLevelVector[levelInfo.threadIndex] = levelInfo.currLevel;
			}

			ProcessorStats::countMutex->unlock();

			if(linearList.size() == 0 || levelInfo.currLevel - 1 >= ProcessorConfig::maximum)
			{
				prevLinearList.clear();
				prevLinearList.reserve(linearList.size());
				prevLinearList.swap((linearList));

				prevPListLengths.clear();
				prevPListLengths.reserve(pListLengths.size());
				prevPListLengths.swap((pListLengths));
				pListLengths.reserve(0);

				continueSearching = false;
			}
			else
			{
				//Have to add prediction here 
				bool prediction = false;//PredictHardDiskOrRAMProcessing(levelInfo, ProcessorStats::levelRecordings[levelInfo.currLevel - 2]);
				if(prediction)
				{

					PListType indexing = 0;
					prevLocalPListArray->clear();
					for(int piss = 0; piss < pListLengths.size(); piss++)
					{
						prevLocalPListArray->push_back(new vector<PListType>(linearList.begin() + indexing, linearList.begin() + indexing + pListLengths[piss]));
						indexing += pListLengths[piss];
					}

					linearList.clear();
					linearList.reserve(0);
					pListLengths.clear();
					pListLengths.reserve(0);
					break;
				}
				else
				{
					continueSearching = true;
				
					//DispatchNewThreadsRAM(0, continueSearching, linearList, pListLengths, levelInfo, isThreadDefuncted);

					prevLinearList.clear();
					prevLinearList.reserve(linearList.size());
					prevLinearList.swap((linearList));

					prevPListLengths.clear();
					prevPListLengths.reserve(pListLengths.size());
					prevPListLengths.swap((pListLengths));
					pListLengths.reserve(0);
				}

			}
		}
		return continueSearching;
	}

	void PlantSeedProcessing(DataType patternCount, DataType overallFilePosition, LevelPackage &levelInfo)
	{
		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		PListType cycles = patternCount/threadsToDispatch;
		PListType lastCycle = patternCount - (cycles*threadsToDispatch);
		PListType span = cycles;
		PListType position = 0;

		vector<future<void>> *threadPlantSeedPool = new vector<future<void>>();
				
		for(int i = 0; i < threadsToDispatch; i++)
		{
			if(!(i < threadsToDispatch - 1))
			{
				span = span + lastCycle;
			}	
						
			threadPlantSeedPool->push_back(std::async(std::launch::async, &DRAMProcessor<PListType>::PlantSeed, this, overallFilePosition, position, span, i));
					
			position += span;
		}

		vector<unsigned int> localWorkingThreads;
		for(unsigned int i = 0; i < threadsToDispatch; i++)
		{
			localWorkingThreads.push_back(i);
		}

		Processor<DataType>::WaitForThreads(localWorkingThreads, threadPlantSeedPool); 

		PlantSeedPostProcessing(levelInfo);

		threadPlantSeedPool->erase(threadPlantSeedPool->begin(), threadPlantSeedPool->end());
		(*threadPlantSeedPool).clear();
		delete threadPlantSeedPool;
	}

	//First level post processing 
	void PlantSeedPostProcessing(LevelPackage &levelInfo)
	{
		if(ProcessorStats::levelRecordings.size() < levelInfo.currLevel)
		{
			ProcessorStats::levelRecordings.resize(levelInfo.currLevel);
		}
		ProcessorStats::levelRecordings[0] = 256;
		if(ProcessorStats::mostCommonPatternIndex.size() < levelInfo.currLevel)
		{
			ProcessorStats::mostCommonPatternIndex.resize(levelInfo.currLevel);
			ProcessorStats::mostCommonPatternCount.resize(levelInfo.currLevel);
		}
					
		std::map<string, PListType> countMap;
		std::map<string, vector<PListType>> indexMap;
		for (PListType i = 0; i < prevPListArray->size(); i++)
		{
			if((*prevPListArray)[i] != nullptr && (*prevPListArray)[i]->size() > 0)
			{
				countMap[ProcessorConfig::files[ProcessorConfig::f]->fileString.substr((*(*prevPListArray)[i])[0] - (levelInfo.currLevel), levelInfo.currLevel)] += (*prevPListArray)[i]->size();
							
				if( countMap[ProcessorConfig::files[ProcessorConfig::f]->fileString.substr((*(*prevPListArray)[i])[0] - (levelInfo.currLevel), levelInfo.currLevel)] > ProcessorStats::mostCommonPatternCount[levelInfo.currLevel - 1])
				{
					ProcessorStats::mostCommonPatternCount[levelInfo.currLevel - 1] = countMap[ProcessorConfig::files[ProcessorConfig::f]->fileString.substr((*(*prevPListArray)[i])[0] - (levelInfo.currLevel), levelInfo.currLevel)];
					ProcessorStats::mostCommonPatternIndex[levelInfo.currLevel - 1] = (*(*prevPListArray)[i])[0] - (levelInfo.currLevel);
				}

				if((*prevPListArray)[i]->size() >= 1)
				{
					indexMap[ProcessorConfig::files[ProcessorConfig::f]->fileString.substr((*(*prevPListArray)[i])[0] - (levelInfo.currLevel), levelInfo.currLevel)].push_back(i);
				}
			}
		}
		ProcessorStats::levelRecordings[levelInfo.currLevel - 1] = countMap.size();
		for(map<string, vector<PListType>>::iterator it = indexMap.begin(); it != indexMap.end(); it++)
		{
			if(it->second.size() == 1 && (*prevPListArray)[it->second[0]]->size() == 1)
			{
				(*prevPListArray)[it->second[0]]->clear();
				ProcessorStats::levelRecordings[levelInfo.currLevel - 1]--;
				ProcessorStats::eradicatedPatterns++;
			}
		}
		//
		//if(coverage.size() < levelInfo.currLevel)
		//{
		//	coverage.resize(levelInfo.currLevel);
		//}
		//coverage[0] = ((float)(files[f]->fileStringSize - (256 - levelRecordings[0])))/((float)files[f]->fileStringSize);
	}

	//First level processing 
	void PlantSeed(DataType positionInFile, DataType startPatternIndex, DataType numPatternsToSearch, DataType threadIndex)
	{
		int fileIndex = ProcessorConfig::f;

		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		DataType earlyApproximation = ProcessorConfig::files[fileIndex]->fileString.size()/(256*(ProcessorConfig::numThreads - 1));
		vector<DataType>* leaves[256];
		for(int i = 0; i < 256; i++)
		{
			leaves[i] = new vector<DataType>();
			leaves[i]->reserve(earlyApproximation);
		}
		DataType endPatternIndex = numPatternsToSearch + startPatternIndex;

		for (DataType i = startPatternIndex; i < endPatternIndex; i++)
		{
			int temp = i + positionInFile + 1;
			uint8_t tempIndex = (uint8_t)ProcessorConfig::files[fileIndex]->fileString[i];
			if(ProcessorConfig::patternToSearchFor.size() == 0 || ProcessorConfig::files[fileIndex]->fileString[i] == ProcessorConfig::patternToSearchFor[0])
			{
				leaves[tempIndex]->push_back(temp);
			}
		}

		for(int i = 0; i < 256; i++)
		{
			(*prevPListArray)[threadIndex + i*threadsToDispatch] = leaves[i];
		}
	
		ProcessorStats::currentLevelVector[threadIndex] = 2;
	}

	void DRAMProcessor::PostPlantDataPrep(bool prediction, vector<vector<string>>& fileList)
	{
		PListType threadsToDispatch = ProcessorConfig::numThreads - 1;
		vector<vector<string>> tempFileList = fileList;
		for(int a = 0; a < fileList.size(); a++)
		{
			fileList[a].clear();
		}

		if(prediction)
		{
			if(!ProcessorStats::usedRAM[0])
			{
				//chunk files
				vector<PListArchive*> threadFiles;
				stringstream threadFilesNames;
				unsigned int threadNumber = 0;

				for(int a = 0; a < tempFileList.size(); a++)
				{
					for(int b = 0; b < tempFileList[a].size(); b++)
					{
						fileList[threadNumber].push_back(tempFileList[a][b]);

						//Increment chunk
						threadNumber++;
						if(threadNumber >= threadsToDispatch)
						{
							threadNumber = 0;
						}
					}
				}
			}
			else if(ProcessorStats::usedRAM[0])
			{
				//chunk files
				vector<PListArchive*> threadFiles;
				stringstream threadFilesNames;
				unsigned int threadNumber = 0;

				for(int a = 0; a < threadsToDispatch; a++)
				{
					threadFilesNames.str("");
					ProcessorStats::fileIDMutex->lock();
					ProcessorStats::fileID++;
					threadFilesNames << "PListChunks" << ProcessorStats::fileID;
					ProcessorStats::fileIDMutex->unlock();

					threadFiles.push_back(new PListArchive(threadFilesNames.str(), true));
					fileList[a].push_back(threadFilesNames.str());
				}
				for(PListType prevIndex = 0; prevIndex < prevPListArray->size(); )
				{
					list<PListType> *sorting = new list<PListType>();

					for(int threadCount = 0; threadCount < threadsToDispatch; threadCount++)
					{
						if((*prevPListArray)[prevIndex] != NULL)
						{
							copy( (*prevPListArray)[prevIndex]->begin(), (*prevPListArray)[prevIndex]->end(), std::back_inserter(*sorting));
							((*prevPListArray)[prevIndex])->erase(((*prevPListArray)[prevIndex])->begin(), ((*prevPListArray)[prevIndex])->end());
							delete (*prevPListArray)[prevIndex];
							prevIndex++;
						}
					}
					vector<PListType> finalVector;
					sorting->sort();
					std::copy( sorting->begin(), sorting->end(), std::back_inserter(finalVector));
					sorting->clear();
					delete sorting;

					threadFiles[threadNumber]->WriteArchiveMapMMAP(finalVector);
					threadFiles[threadNumber]->WriteArchiveMapMMAP(vector<PListType>(), "", true);
				

					//Increment chunk
					threadNumber++;
					if(threadNumber >= threadsToDispatch)
					{
						threadNumber = 0;
					}
				}
				//Clear out the array also after deletion
				prevPListArray->clear();

				for(int a = 0; a < threadsToDispatch; a++)
				{
					threadFiles[a]->WriteArchiveMapMMAP(vector<PListType>(), "", true);
					threadFiles[a]->CloseArchiveMMAP();
					delete threadFiles[a];
				}
			}
		}
		else if(!prediction)
		{
			if(!ProcessorStats::usedRAM[0])
			{
				prevPListArray->clear();
				for(PListType i = 0; i < tempFileList.size(); i++)
				{
					for(PListType prevChunkCount = 0; prevChunkCount < tempFileList[i].size(); prevChunkCount++)
					{
						PListArchive archive(tempFileList[i][prevChunkCount]);
						while(archive.Exists() && !archive.IsEndOfFile())
						{
							//Just use 100 GB to say we want the whole file for now
							vector<vector<PListType>*> packedPListArray;
							archive.GetPListArchiveMMAP(packedPListArray);

							prevPListArray->insert(prevPListArray->end(), packedPListArray.begin(), packedPListArray.end());

							packedPListArray.erase(packedPListArray.begin(), packedPListArray.end());
						}
						archive.CloseArchiveMMAP();
					}
				}

				for(PListType i = 0; i < threadsToDispatch; i++)
				{
					if(!ProcessorConfig::history)
					{
						//TODO -PJM
						//DeleteArchives(tempFileList[i], ARCHIVE_FOLDER);
					}
				}
				//Transition to using entire file when first level was hard disk processing and next level is pure ram
				if(ProcessorConfig::files[ProcessorConfig::f]->fileString.size() != ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
				{
					//new way to read in file
					ProcessorConfig::files[ProcessorConfig::f]->copyBuffer->seekg( 0 );
					ProcessorConfig::files[ProcessorConfig::f]->fileString.resize(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize);
					ProcessorConfig::files[ProcessorConfig::f]->copyBuffer->read( &ProcessorConfig::files[ProcessorConfig::f]->fileString[0], ProcessorConfig::files[ProcessorConfig::f]->fileString.size());
				}
			}
		}

		for(int a = 0; a < ProcessorStats::usedRAM.size(); a++)
		{
			ProcessorStats::usedRAM[a] = !prediction;
		}
	}

	bool DispatchNewThreads(DataType newPatternCount, bool& morePatternsToFind, vector<DataType> &linearList, vector<DataType> &pListLengths, LevelPackage levelInfo, bool& isThreadDefuncted);
	void TreeSearch(vector<vector<DataType>*>* patterns, vector<DataType> patternIndexList, vector<string> fileList, LevelPackage levelInfo);
	vector<vector<DataType>> ProcessThreadsWorkLoadRAM(unsigned int threadsToDispatch, vector<vector<DataType>*>* patterns);
	vector<vector<DataType>> ProcessThreadsWorkLoadRAMFirstLevel(unsigned int threadsToDispatch, vector<vector<DataType>*>* patterns);

private:

	vector<vector<DataType>*>* prevPListArray;
	vector<vector<DataType>*>* globalPListArray;
};
