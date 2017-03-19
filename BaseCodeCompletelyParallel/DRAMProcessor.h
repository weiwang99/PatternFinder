#pragma once
#include "Processor.h"

template<class DataType>
class DRAMProcessor : Processor<DataType>
{
public:
	DRAMProcessor(void)
	{
		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		prevPListArray = new vector<vector<DataType>*>();
		nextPListArray = new vector<vector<DataType>*>();

		//Initialize all possible values for the first list to NULL
		prevPListArray->resize(256*threadsToDispatch);
		for(int i = 0; i < 256*threadsToDispatch; i++)
		{
			(*prevPListArray)[i] = NULL;
		}
	}
	~DRAMProcessor(void)
	{
		Cleanup();

		delete prevPListArray;
		delete nextPListArray;
	}

	void Cleanup()
	{
		/*for (int i = 0; i < prevPListArray->size(); i++)
		{
			if((*prevPListArray)[i] != NULL)
			{
				delete (*prevPListArray)[i];
			}
		}
		prevPListArray->clear();

		for (int i = 0; i < nextPListArray->size(); i++)
		{
			if((*nextPListArray)[i] != NULL)
			{
				delete (*nextPListArray)[i];
			}
		}*/
		nextPListArray->clear();
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

	void WaitForThreads(vector<unsigned int> localWorkingThreads, vector<future<vector<vector<PListType>*>*>> *localThreadPool, bool recursive = false, unsigned int level = 0)
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
						vector<vector<PListType>*>* newBatch = (*localThreadPool)[localWorkingThreads[k]].get();

						threadsFinished++;
					}
					else
					{
						vector<vector<PListType>*>* newBatch = (*localThreadPool)[localWorkingThreads[k]].get();
						
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

	void WaitForFirstLevelThreads(vector<unsigned int> localWorkingThreads, vector<future<void>> *localThreadPool, bool recursive = false, unsigned int level = 0)
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

	vector<vector<PListType>*>* ThreadBranch(vector<vector<PListType>*>* patterns, vector<PListType> patternIndexList, LevelPackage levelInfo)
	{
		PListType numPatternsToSearch = patternIndexList.size();
		bool isThreadDefuncted = false;

		int tempCurrentLevel = levelInfo.currLevel;
		int threadsToDispatch = ProcessorConfig::numThreads - 1;

		if(ProcessorStats::threadsDispatched - ProcessorStats::threadsDefuncted > threadsToDispatch)
		{
			cout << "WENT OVER THREADS ALLOCATION SIZE!" << endl;
		}

		vector<vector<PListType>*>* prevLocalPListArray = new vector<vector<PListType>*>();
		vector<vector<PListType>*>*	nextLocalPListArray = new vector<vector<PListType>*>();

		for(PListType i = 0; i < numPatternsToSearch; i++)
		{
			if((*patterns)[patternIndexList[i]] != NULL)
			{
				prevLocalPListArray->push_back((*patterns)[patternIndexList[i]]);
			}
		}

		continueSearching = Process(prevLocalPListArray,  nextLocalPListArray, levelInfo, isThreadDefuncted);

		//Set patterns to the previous level's pLists to transition from ram to hd processing
		if(!continueSearching)
		{
			if(prevLocalPListArray != NULL)
			{
				delete prevLocalPListArray;
			}

			if(nextLocalPListArray != NULL)
			{
				delete nextLocalPListArray;
			}
			prevLocalPListArray = NULL;
			nextLocalPListArray = NULL;
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

		return prevLocalPListArray;
	}

	vector<vector<DataType>*>* BuildThreadJobs(LevelPackage &levelData)
	{
		vector<future<vector<vector<PListType>*>* >> *threadPool = new vector<future<vector<vector<PListType>*>* >>();

		int level = levelData.currLevel;
		
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

			threadPool->push_back(std::async(std::launch::async, &DRAMProcessor::ThreadBranch, this, prevPListArray, balancedTruncList[i], levelInfo));
	
		}
		ProcessorStats::countMutex->unlock();

		WaitForThreads(localWorkingThreads, threadPool);

		return prevPListArray;
	}

	bool Process(vector<vector<DataType>*>* prevLocalPListArray, vector<vector<DataType>*>* nextLocalPListArray, LevelPackage& levelInfo, bool& isThreadDefuncted)
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

				if(!ProcessorStats::firstLevelProcessedHD)
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
			}
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
				bool prediction = ProcessorManager::PredictHardDiskOrRAMProcessing(levelInfo, ProcessorStats::levelRecordings[levelInfo.currLevel - 2]);
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
				
					DispatchNewThreads(0, continueSearching, linearList, pListLengths, levelInfo, isThreadDefuncted);

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

	bool DispatchNewThreads(PListType newPatternCount, bool& morePatternsToFind, vector<PListType> &linearList, vector<PListType> &pListLengths, LevelPackage levelInfo, bool& isThreadDefuncted)
	{
		bool dispatchedNewThreads = false;
		bool alreadyUnlocked = false;
		ProcessorStats::countMutex->lock();

		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		int unusedCores = (threadsToDispatch - (ProcessorStats::threadsDispatched - ProcessorStats::threadsDefuncted)) + 1;
	
		if(pListLengths.size() < unusedCores && unusedCores > 1)
		{
			unusedCores = (int)pListLengths.size();
		}
		//Be conservative with thread allocation
		//Only create new thread for work if the new job will have atleast 10 patterns
		//Stack overflow can occur if there are too many little jobs being assigned
		//Need to have an available core, need to still have patterns to search and need to have more than 1 pattern to be worth splitting up the work
		if(unusedCores > 1 && morePatternsToFind && pListLengths.size()/unusedCores > 10)
		{
		
			bool spawnThreads = true;
			//If this thread is at the lowest level of progress spawn new threads
			if(spawnThreads)
			{
				//cout << "PList size: " << linearList.size() << " with " << pListLengths.size() << " pattern nodes!" << endl;
				vector<vector<PListType>*>* prevLocalPListArray = new vector<vector<PListType>*>();
				PListType indexing = 0;
				for(int piss = 0; piss < pListLengths.size(); piss++)
				{
					prevLocalPListArray->push_back(new vector<PListType>(linearList.begin() + indexing, linearList.begin() + indexing + pListLengths[piss]));
					indexing += pListLengths[piss];
				}

				linearList.clear();
				linearList.reserve(0);
				pListLengths.clear();
				pListLengths.reserve(0);

				vector<vector<PListType>> balancedTruncList = BalanceLoad(unusedCores, prevLocalPListArray);
				vector<unsigned int> localWorkingThreads;
				for(unsigned int i = 0; i < balancedTruncList.size(); i++)
				{
					localWorkingThreads.push_back(i);
				}

				if(localWorkingThreads.size() > 1)
				{
					int threadsToTest = (ProcessorStats::threadsDispatched - ProcessorStats::threadsDefuncted) - 1;
					if(threadsToTest + localWorkingThreads.size() <= threadsToDispatch)
					{

						for(int z = 0; z < balancedTruncList.size(); z++)
						{
							unsigned int tally = 0;
							for(int d = 0; d < balancedTruncList[z].size(); d++)
							{
								tally += (unsigned int)(*prevLocalPListArray)[balancedTruncList[z][d]]->size();
							}
						}

						dispatchedNewThreads = true;

						LevelPackage levelInfoRecursion;
						levelInfoRecursion.currLevel = levelInfo.currLevel;
						levelInfoRecursion.threadIndex = levelInfo.threadIndex;
						levelInfoRecursion.inceptionLevelLOL = levelInfo.inceptionLevelLOL + 1;
						levelInfoRecursion.useRAM = true;

						ProcessorStats::threadsDefuncted++;
						isThreadDefuncted = true;

						vector<future<void>> *localThreadPool = new vector<future<void>>();
						for (PListType i = 0; i < localWorkingThreads.size(); i++)
						{
							ProcessorStats::threadsDispatched++;
							localThreadPool->push_back(std::async(std::launch::async, &DRAMProcessor::ThreadBranch, this, prevLocalPListArray, balancedTruncList[i], vector<string>(), levelInfoRecursion));
						}
						ProcessorStats::countMutex->unlock();

						alreadyUnlocked = true;
						WaitForThreads(localWorkingThreads, localThreadPool, true, levelInfo.threadIndex);

						localThreadPool->erase(localThreadPool->begin(), localThreadPool->end());
						(*localThreadPool).clear();
						delete localThreadPool;
						morePatternsToFind = false;
						delete prevLocalPListArray;
					}
					else
					{
						for(int piss = 0; piss < prevLocalPListArray->size(); piss++)
						{
							delete (*prevLocalPListArray)[piss];
						}
						delete prevLocalPListArray;
					}
				}
				else
				{
					for(int piss = 0; piss < prevLocalPListArray->size(); piss++)
					{
						delete (*prevLocalPListArray)[piss];
					}
					delete prevLocalPListArray;
				}
			}
			else
			{

			}
		}
		if(!alreadyUnlocked)
		{
			ProcessorStats::countMutex->unlock();
		}
		return dispatchedNewThreads;
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

		WaitForThreads(localWorkingThreads, threadPlantSeedPool); 

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

	vector<vector<PListType>*>* GetPrevPList(){return prevPListArray;};
	vector<vector<PListType>*>* GetNextPList(){return nextPListArray;};

private:

	vector<vector<DataType>*>* prevPListArray;
	vector<vector<DataType>*>* nextPListArray;
};
