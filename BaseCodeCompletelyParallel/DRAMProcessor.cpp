#include "DRAMProcessor.h"


//template<class DataType>
//DRAMProcessor<DataType>::DRAMProcessor(void)
//{
//}
//
//
//template<class DataType>
//DRAMProcessor<DataType>::~DRAMProcessor(void)
//{
//}

//template <class DataType>
//void DRAMProcessor<DataType>::TreeSearch(vector<vector<DataType>*>* patterns, vector<DataType> patternIndexList, vector<string> fileList, LevelPackage levelInfo)
//{
//	DataType numPatternsToSearch = patternIndexList.size();
//	bool isThreadDefuncted = false;
//	//cout << "Threads dispatched: " << threadsDispatched << " Threads deported: " << threadsDefuncted << " Threads running: " << threadsDispatched - threadsDefuncted << endl;
//
//	int tempCurrentLevel = levelInfo.currLevel;
//	int threadsToDispatch = ProcessorConfig::numThreads - 1;
//
//	if(threadsDispatched - threadsDefuncted > threadsToDispatch)
//	{
//		cout << "WENT OVER THREADS ALLOCATION SIZE!" << endl;
//	}
//
//	vector<vector<DataType>*>* prevLocalPListArray = new vector<vector<DataType>*>();
//	vector<vector<DataType>*>*	globalLocalPListArray = new vector<vector<DataType>*>();
//
//	if(levelInfo.useRAM)
//	{
//		for(DataType i = 0; i < numPatternsToSearch; i++)
//		{
//			if((*patterns)[patternIndexList[i]] != NULL)
//			{
//				prevLocalPListArray->push_back((*patterns)[patternIndexList[i]]);
//			}
//		}
//	}
//
//	bool continueSearching = true;
//
//	while(continueSearching)
//	{
//		bool useRAMBRO = true;
//		if(levelInfo.currLevel != 2)
//		{
//			useRAMBRO = !NextLevelTreeSearchRecursion(prevLocalPListArray, globalLocalPListArray, fileList, levelInfo);
//		}
//		else
//		{
//			useRAMBRO = levelInfo.useRAM;
//		}
//
//		if(useRAMBRO)
//		{
//			continueSearching = Process(prevLocalPListArray, globalLocalPListArray, levelInfo, isThreadDefuncted);
//		}
//		else
//		{
//			continueSearching = Process(levelInfo, fileList, isThreadDefuncted);
//		}
//	}
//
//	if(prevLocalPListArray != NULL)
//	{
//		delete prevLocalPListArray;
//	}
//
//	if(globalLocalPListArray != NULL)
//	{
//		delete globalLocalPListArray;
//	}
//
//	countMutex->lock();
//	if(currentLevelVector[levelInfo.threadIndex] > globalLevel)
//	{
//		globalLevel = currentLevelVector[levelInfo.threadIndex];
//	}
//	if(!isThreadDefuncted)
//	{
//		threadsDefuncted++;
//	}
//	countMutex->unlock();
//}
//
//template<class DataType>
//vector<vector<DataType>> DRAMProcessor<DataType>::ProcessThreadsWorkLoadRAM(unsigned int threadsToDispatch, vector<vector<DataType>*>* patterns)
//{
//	vector<vector<DataType>> balancedList(threadsToDispatch);
//	vector<DataType> balancedSizeList;
//	for(DataType i = 0; i < threadsToDispatch; i++)
//	{
//		balancedSizeList.push_back(0);
//	}
//	for(DataType i = 0; i < patterns->size(); i++)
//	{
//
//		if((*patterns)[i] != NULL)
//		{
//			bool found = false;
//			DataType smallestIndex = 0;
//			DataType smallestAmount = -1;
//			for(DataType z = 0; z < threadsToDispatch; z++)
//			{
//				if(balancedSizeList[z] < smallestAmount && (*patterns)[i]->size() > 0)
//				{
//					smallestAmount = balancedSizeList[z];
//					smallestIndex = z;
//					found = true;
//				}
//			}
//			if(found && (*patterns)[i]->size() > 0)
//			{
//				balancedSizeList[smallestIndex] += (*patterns)[i]->size();
//				balancedList[smallestIndex].push_back(i);
//			}
//		}
//	}
//	DataType sizeToTruncate = 0;
//	for(unsigned int i = 0; i < threadsToDispatch; i++)
//	{
//		if(balancedList[i].size() > 0)
//		{
//			sizeToTruncate++;
//		}
//	}
//
//	vector<vector<DataType>> balancedTruncList(sizeToTruncate);
//	DataType internalCount = 0;
//	for(unsigned int i = 0; i < threadsToDispatch; i++)
//	{
//		if(balancedList[i].size() > 0)
//		{
//			balancedTruncList[internalCount] = balancedList[i];
//			internalCount++;
//		}
//	}
//
//	vector<unsigned int> localWorkingThreads;
//	for(unsigned int i = 0; i < sizeToTruncate; i++)
//	{
//		localWorkingThreads.push_back(i);
//	}
//	
//	return balancedTruncList;
//}
//
//template<class DataType>
//vector<vector<DataType>> DRAMProcessor<DataType>::ProcessThreadsWorkLoadRAMFirstLevel(unsigned int threadsToDispatch, vector<vector<DataType>*>* patterns)
//{
//	vector<vector<DataType>> balancedList(threadsToDispatch);
//	vector<DataType> balancedSizeList;
//	for(DataType i = 0; i < threadsToDispatch; i++)
//	{
//		balancedSizeList.push_back(0);
//	}
//
//	if(patterns->size() == threadsToDispatch*256)
//	{
//		vector<DataType> patternTotals(256, 0);
//		for(DataType i = 0; i < 256; i++)
//		{
//			for(DataType z = 0; z < threadsToDispatch; z++)
//			{
//				patternTotals[i] += (*patterns)[i*threadsToDispatch + z]->size();
//			}
//		}
//
//		for(DataType i = 0; i < patternTotals.size(); i++)
//		{
//			bool found = false;
//			DataType smallestIndex = 0;
//			DataType smallestAmount = -1;
//			for(DataType j = 0; j < threadsToDispatch; j++)
//			{
//				if((*patterns)[(i*threadsToDispatch) + j] != NULL)
//				{
//					for(DataType z = 0; z < threadsToDispatch; z++)
//					{
//						if(balancedSizeList[z] < smallestAmount)
//						{
//							smallestAmount = balancedSizeList[z];
//							smallestIndex = z;
//							found = true;
//						}
//					}
//
//				}
//			}
//			if(found)
//			{
//				balancedSizeList[smallestIndex] += patternTotals[i];
//				for(DataType j = 0; j < threadsToDispatch; j++)
//				{
//					balancedList[smallestIndex].push_back((i*threadsToDispatch) + j);
//				}
//			}
//		}
//	}
//	else
//	{
//		vector<DataType> patternTotals;
//		for(DataType i = 0; i < patterns->size(); i++)
//		{
//			patternTotals.push_back((*patterns)[i]->size());
//		}
//
//		for(DataType i = 0; i < patternTotals.size(); i++)
//		{
//			bool found = false;
//			DataType smallestIndex = 0;
//			DataType smallestAmount = -1;
//			
//			if((*patterns)[i] != NULL)
//			{
//				for(DataType z = 0; z < threadsToDispatch; z++)
//				{
//					if(balancedSizeList[z] < smallestAmount)
//					{
//						smallestAmount = balancedSizeList[z];
//						smallestIndex = z;
//						found = true;
//					}
//				}
//
//			}
//			
//			if(found)
//			{
//				balancedSizeList[smallestIndex] += patternTotals[i];
//				balancedList[smallestIndex].push_back(i);
//			}
//		}
//	}
//
//	DataType sizeToTruncate = 0;
//	for(unsigned int i = 0; i < threadsToDispatch; i++)
//	{
//		if(balancedList[i].size() > 0)
//		{
//			sizeToTruncate++;
//		}
//	}
//
//	vector<vector<DataType>> balancedTruncList(sizeToTruncate);
//	DataType internalCount = 0;
//	for(unsigned int i = 0; i < threadsToDispatch; i++)
//	{
//		if(balancedList[i].size() > 0)
//		{
//			balancedTruncList[internalCount] = balancedList[i];
//			internalCount++;
//		}
//	}
//
//	return balancedTruncList;
//}


//template<class DataType>
//bool DRAMProcessor<DataType>::DispatchNewThreads(DataType newPatternCount, bool& morePatternsToFind, vector<DataType> &linearList, vector<DataType> &pListLengths, LevelPackage levelInfo, bool& isThreadDefuncted)
//{
//	bool dispatchedNewThreads = false;
//	bool alreadyUnlocked = false;
//	countMutex->lock();
//
//	int threadsToDispatch = ProcessorConfig::numThreads - 1;
//	int unusedCores = (threadsToDispatch - (threadsDispatched - threadsDefuncted)) + 1;
//	
//	if(pListLengths.size() < unusedCores && unusedCores > 1)
//	{
//		unusedCores = (int)pListLengths.size();
//	}
//	//Be conservative with thread allocation
//	//Only create new thread for work if the new job will have atleast 10 patterns
//	//Stack overflow can occur if there are too many little jobs being assigned
//	//Need to have an available core, need to still have patterns to search and need to have more than 1 pattern to be worth splitting up the work
//	if(unusedCores > 1 && morePatternsToFind && pListLengths.size()/unusedCores > 10)
//	{
//		
//		bool spawnThreads = true;
//		//If this thread is at the lowest level of progress spawn new threads
//		if(spawnThreads)
//		{
//			//cout << "PList size: " << linearList.size() << " with " << pListLengths.size() << " pattern nodes!" << endl;
//			vector<vector<DataType>*>* prevLocalPListArray = new vector<vector<DataType>*>();
//			DataType indexing = 0;
//			for(int piss = 0; piss < pListLengths.size(); piss++)
//			{
//				prevLocalPListArray->push_back(new vector<DataType>(linearList.begin() + indexing, linearList.begin() + indexing + pListLengths[piss]));
//				indexing += pListLengths[piss];
//			}
//
//			linearList.clear();
//			linearList.reserve(0);
//			pListLengths.clear();
//			pListLengths.reserve(0);
//
//			vector<vector<DataType>> balancedTruncList = ProcessThreadsWorkLoadRAM(unusedCores, prevLocalPListArray);
//			vector<unsigned int> localWorkingThreads;
//			for(unsigned int i = 0; i < balancedTruncList.size(); i++)
//			{
//				localWorkingThreads.push_back(i);
//			}
//
//			if(localWorkingThreads.size() > 1)
//			{
//				int threadsToTest = (threadsDispatched - threadsDefuncted) - 1;
//				if(threadsToTest + localWorkingThreads.size() <= threadsToDispatch)
//				{
//
//					for(int z = 0; z < balancedTruncList.size(); z++)
//					{
//						unsigned int tally = 0;
//						for(int d = 0; d < balancedTruncList[z].size(); d++)
//						{
//							tally += (unsigned int)(*prevLocalPListArray)[balancedTruncList[z][d]]->size();
//						}
//					}
//
//					dispatchedNewThreads = true;
//
//					LevelPackage levelInfoRecursion;
//					levelInfoRecursion.currLevel = levelInfo.currLevel;
//					levelInfoRecursion.threadIndex = levelInfo.threadIndex;
//					levelInfoRecursion.inceptionLevelLOL = levelInfo.inceptionLevelLOL + 1;
//					levelInfoRecursion.useRAM = true;
//
//					threadsDefuncted++;
//					isThreadDefuncted = true;
//
//					vector<future<void>> *localThreadPool = new vector<future<void>>();
//					for (DataType i = 0; i < localWorkingThreads.size(); i++)
//					{
//						threadsDispatched++;
//						localThreadPool->push_back(std::async(std::launch::async, &Forest::TreeSearch, this, prevLocalPListArray, balancedTruncList[i], vector<string>(), levelInfoRecursion));
//					}
//					countMutex->unlock();
//
//					alreadyUnlocked = true;
//					WaitForThreads(localWorkingThreads, localThreadPool, true, levelInfo.threadIndex);
//
//					localThreadPool->erase(localThreadPool->begin(), localThreadPool->end());
//					(*localThreadPool).clear();
//					delete localThreadPool;
//					morePatternsToFind = false;
//					delete prevLocalPListArray;
//				}
//				else
//				{
//					for(int piss = 0; piss < prevLocalPListArray->size(); piss++)
//					{
//						delete (*prevLocalPListArray)[piss];
//					}
//					delete prevLocalPListArray;
//				}
//			}
//			else
//			{
//				for(int piss = 0; piss < prevLocalPListArray->size(); piss++)
//				{
//					delete (*prevLocalPListArray)[piss];
//				}
//				delete prevLocalPListArray;
//			}
//		}
//		else
//		{
//
//		}
//	}
//	if(!alreadyUnlocked)
//	{
//		countMutex->unlock();
//	}
//	return dispatchedNewThreads;
//}

//template<class DataType>
//void DRAMProcessor<DataType>::PlantSeed(DataType positionInFile, DataType startPatternIndex, DataType numPatternsToSearch, DataType threadIndex)
//{
//	int threadsToDispatch = ProcessorConfig::numThreads - 1;
//	DataType earlyApproximation = files[f]->fileString.size()/(256*(ProcessorConfig::numThreads - 1));
//	vector<DataType>* leaves[256];
//	for(int i = 0; i < 256; i++)
//	{
//		leaves[i] = new vector<DataType>();
//		leaves[i]->reserve(earlyApproximation);
//	}
//	DataType endPatternIndex = numPatternsToSearch + startPatternIndex;
//
//	for (DataType i = startPatternIndex; i < endPatternIndex; i++)
//	{
//		int temp = i + positionInFile + 1;
//		uint8_t tempIndex = (uint8_t)files[f]->fileString[i];
//		if(patternToSearchFor.size() == 0 || files[f]->fileString[i] == patternToSearchFor[0])
//		{
//			leaves[tempIndex]->push_back(temp);
//		}
//	}
//
//	for(int i = 0; i < 256; i++)
//	{
//		(*prevPListArray)[threadIndex + i*threadsToDispatch] = leaves[i];
//	}
//	
//	currentLevelVector[threadIndex] = 2;
//}

//template<class DataType>
//bool DRAMProcessor<DataType>::Process(vector<vector<DataType>*>* prevLocalPListArray, vector<vector<DataType>*>* globalLocalPListArray, LevelPackage& levelInfo, bool& isThreadDefuncted)
//{
//	//Keeps track of all pLists in one contiguous block of memory
//	vector<DataType> linearList;
//	//Keeps track of the length of each pList
//	vector<DataType> pListLengths;
//
//	//Keeps track of all pLists in one contiguous block of memory
//	vector<DataType> prevLinearList;
//	//Keeps track of the length of each pList
//	vector<DataType> prevPListLengths;
//
//	bool continueSearching = true;
//	int threadsToDispatch = ProcessorConfig::numThreads - 1;
//	DataType totalTallyRemovedPatterns = 0;
//	DataType newPatterns = 0;
//	string globalStringConstruct;
//	vector<DataType> pListTracker;
//	DataType fileSize = files[f]->fileStringSize;
//	DataType totalCount = 0;
//	if(levelInfo.currLevel == 2)
//	{
//		int threadCountage = 0;
//
//		for (DataType i = 0; i < prevLocalPListArray->size(); i++)
//		{
//			DataType pListLength = (*prevLocalPListArray)[i]->size();
//			if(pListLength > 0)
//			{
//				totalCount += pListLength;
//			}
//		}
//		prevLinearList.reserve(totalCount);
//
//		for (DataType i = 0; i < prevLocalPListArray->size(); i++)
//		{
//			DataType pListLength = (*prevLocalPListArray)[i]->size();
//			if(pListLength > 0)
//			{
//				threadCountage += pListLength;
//				prevLinearList.insert(prevLinearList.end(), (*prevLocalPListArray)[i]->begin(), (*prevLocalPListArray)[i]->end());
//				delete  (*prevLocalPListArray)[i];
//			}
//			else
//			{
//				delete (*prevLocalPListArray)[i];
//			}
//
//			if(!firstLevelProcessedHD)
//			{
//				if(i % threadsToDispatch == (threadsToDispatch - 1) && threadCountage > 1)
//				{
//					prevPListLengths.push_back(threadCountage);
//					threadCountage = 0;
//				}
//			}
//			else
//			{
//				prevPListLengths.push_back(threadCountage);
//				threadCountage = 0;
//			}
//		}
//	}
//	else
//	{
//
//		for (DataType i = 0; i < prevLocalPListArray->size(); i++)
//		{
//			DataType pListLength = (*prevLocalPListArray)[i]->size();
//			if(pListLength > 0)
//			{
//				totalCount += pListLength;
//			}
//		}
//
//		prevLinearList.reserve(totalCount);
//
//		for (DataType i = 0; i < prevLocalPListArray->size(); i++)
//		{
//			DataType pListLength = (*prevLocalPListArray)[i]->size();
//			if(pListLength > 1)
//			{
//				prevLinearList.insert(prevLinearList.end(), (*prevLocalPListArray)[i]->begin(), (*prevLocalPListArray)[i]->end());
//				prevPListLengths.push_back(pListLength);
//				delete (*prevLocalPListArray)[i];
//			}
//			else
//			{
//				delete (*prevLocalPListArray)[i];
//			}
//		}
//	}
//	globalStringConstruct.resize(totalCount);
//	linearList.reserve(totalCount);
//
//	DataType linearListIndex = 0;
//
//	DataType globalStride = 0;
//
//	//We have nothing to process!
//	if(totalCount == 0)
//		return false;
//
//	while(continueSearching)
//	{
//
//		DataType stride = 0;
//		DataType strideCount = 0;
//		DataType prevStride = 0;
//
//		totalTallyRemovedPatterns = 0;
//		DataType minIndex = -1;
//		DataType maxIndex = 0;
//		DataType stringIndexer = 0;
//
//		if(levelInfo.currLevel == 2)
//		{
//			DataType linearListSize = prevLinearList.size();
//			if(linearListSize > 0)
//			{
//				for (DataType i = 0; i < linearListSize; i++)
//				{
//					if (prevLinearList[i] < fileSize)
//					{
//						globalStringConstruct[stringIndexer++] = files[f]->fileString[prevLinearList[i]];
//
//						stride += abs((DataType)(prevLinearList[i] - prevStride));
//						strideCount++;
//						prevStride = prevLinearList[i];
//					}
//				}
//			}
//		}
//		else
//		{
//			DataType linearListSize = prevLinearList.size();
//			if(linearListSize > 1)
//			{
//				for (DataType i = 0; i < linearListSize; i++)
//				{
//					if (prevLinearList[i] < fileSize)
//					{
//						globalStringConstruct[stringIndexer++] = files[f]->fileString[prevLinearList[i]];
//
//						stride += abs((DataType)(prevLinearList[i] - prevStride));
//						strideCount++;
//						prevStride = prevLinearList[i];
//					}
//				}
//			}
//		}
//		
//		if(globalStride == 0)
//		{
//			globalStride = (stride / strideCount);
//		}
//		else
//		{
//			globalStride += (stride / strideCount);
//			globalStride /= 2.0f;
//		}
//
//		if(prevPListLengths.size() == 0)
//		{
//			continueSearching = false;
//			break;
//		}
//
//		globalStringConstruct.resize(stringIndexer);
//		stringIndexer = 0;
//		vector<DataType> newPList[256];
//
//		if(levelInfo.currLevel == 2)
//		{
//			DataType prevPListSize = prevLinearList.size();
//			DataType indexes[256] = {0};
//			DataType indexesToPush[256] = {0};
//			//Banking off very random patterns
//			DataType firstPatternIndex[256] = {0};
//			DataType prevSize = 0;
//			int listLength = 0;
//
//
//			int currList = 0;
//			int currPosition = 0;
//			int limit = prevPListLengths[currList];
//			for (DataType i = 0; i < prevPListSize; i++)
//			{
//				//If pattern is past end of string stream then stop counting this pattern
//				DataType index = prevLinearList[i];
//				if (index < fileSize)
//				{
//					uint_fast8_t indexIntoFile = (uint8_t)globalStringConstruct[stringIndexer++];
//					if(firstPatternIndex[indexIntoFile])
//					{
//						if(newPList[indexIntoFile].empty())
//						{
//							newPList[indexIntoFile].push_back(firstPatternIndex[indexIntoFile]);
//						}
//						newPList[indexIntoFile].push_back(index + 1);	
//						indexes[indexIntoFile]++;
//					}
//					else
//					{
//						firstPatternIndex[indexIntoFile] = index + 1;
//						indexes[indexIntoFile]++;
//						indexesToPush[listLength++] = indexIntoFile;
//					}
//				}
//				else
//				{
//					totalTallyRemovedPatterns++;
//				}
//
//				if(i + 1 == currPosition + limit)
//				{
//					for (DataType k = 0; k < listLength; k++)
//					{
//						int insert = indexes[indexesToPush[k]];
//						if (insert >= minOccurrence)
//						{
//							pListLengths.push_back(newPList[indexesToPush[k]].size());
//							linearList.insert(linearList.end(), newPList[indexesToPush[k]].begin(), newPList[indexesToPush[k]].end());
//
//							indexes[indexesToPush[k]] = 0;
//							firstPatternIndex[indexesToPush[k]] = 0;
//							newPList[indexesToPush[k]].clear();
//						}
//						else if(insert == 1)
//						{
//							totalTallyRemovedPatterns++;
//							indexes[indexesToPush[k]] = 0;
//							firstPatternIndex[indexesToPush[k]] = 0;
//							newPList[indexesToPush[k]].clear();
//						}
//
//					}
//					if(currList + 1 < prevPListLengths.size())
//					{
//						currPosition = i + 1;
//						currList++;
//						limit = prevPListLengths[currList];
//						listLength = 0;
//					}
//				}
//			}
//		}
//		else
//		{
//			DataType prevPListSize = prevLinearList.size();
//			DataType indexes[256] = {0};
//			DataType indexesToPush[256] = {0};
//			//Banking off very random patterns
//			DataType firstPatternIndex[256] = {0};
//			DataType prevSize = 0;
//			int listLength = 0;
//
//
//			int currList = 0;
//			int currPosition = 0;
//			int limit = prevPListLengths[currList];
//			for (DataType i = 0; i < prevPListSize; i++)
//			{
//				//If pattern is past end of string stream then stop counting this pattern
//				DataType index = prevLinearList[i];
//				if (index < fileSize)
//				{
//					uint_fast8_t indexIntoFile = (uint8_t)globalStringConstruct[stringIndexer++];
//					if(firstPatternIndex[indexIntoFile])
//					{
//						if(newPList[indexIntoFile].empty())
//						{
//							newPList[indexIntoFile].push_back(firstPatternIndex[indexIntoFile]);
//						}
//						newPList[indexIntoFile].push_back(index + 1);	
//						indexes[indexIntoFile]++;
//					}
//					else
//					{
//						firstPatternIndex[indexIntoFile] = index + 1;
//						indexes[indexIntoFile]++;
//						indexesToPush[listLength++] = indexIntoFile;
//					}
//				}
//				else
//				{
//					totalTallyRemovedPatterns++;
//				}
//
//				if(i + 1 == currPosition + limit)
//				{
//					for (DataType k = 0; k < listLength; k++)
//					{
//						int insert = indexes[indexesToPush[k]];
//						if (insert >= minOccurrence)
//						{
//							pListLengths.push_back(newPList[indexesToPush[k]].size());
//							linearList.insert(linearList.end(), newPList[indexesToPush[k]].begin(), newPList[indexesToPush[k]].end());
//
//							indexes[indexesToPush[k]] = 0;
//							firstPatternIndex[indexesToPush[k]] = 0;
//							newPList[indexesToPush[k]].clear();
//						}
//						else if(insert == 1)
//						{
//							totalTallyRemovedPatterns++;
//							indexes[indexesToPush[k]] = 0;
//							firstPatternIndex[indexesToPush[k]] = 0;
//							newPList[indexesToPush[k]].clear();
//						}
//
//					}
//					if(currList + 1 < prevPListLengths.size())
//					{
//						currPosition = i + 1;
//						currList++;
//						limit = prevPListLengths[currList];
//						listLength = 0;
//					}
//				}
//			}
//		}
//
//		countMutex->lock();
//
//		eradicatedPatterns += totalTallyRemovedPatterns;
//
//		if(levelRecordings.size() < levelInfo.currLevel)
//		{
//			levelRecordings.resize(levelInfo.currLevel);
//		}
//		levelRecordings[levelInfo.currLevel - 1] += pListLengths.size();
//
//		if(mostCommonPatternIndex.size() < levelInfo.currLevel)
//		{
//			mostCommonPatternIndex.resize(levelInfo.currLevel);
//			mostCommonPatternCount.resize(levelInfo.currLevel);
//		}
//					
//		DataType countage = 0;
//		for (DataType i = 0; i < pListLengths.size(); i++)
//		{
//			if(pListLengths[i] > 0)
//			{
//				if( pListLengths[i] > mostCommonPatternCount[levelInfo.currLevel - 1])
//				{
//					mostCommonPatternCount[levelInfo.currLevel - 1] = pListLengths[i];
//					mostCommonPatternIndex[levelInfo.currLevel - 1] = linearList[countage] - levelInfo.currLevel;
//				}
//			}
//			countage += pListLengths[i];
//		}
//
//		levelInfo.currLevel++;
//
//		if(levelInfo.currLevel > currentLevelVector[levelInfo.threadIndex])
//		{
//			currentLevelVector[levelInfo.threadIndex] = levelInfo.currLevel;
//		}
//
//		countMutex->unlock();
//
//		if(linearList.size() == 0 || levelInfo.currLevel - 1 >= maximum)
//		{
//			prevLinearList.clear();
//			prevLinearList.reserve(linearList.size());
//			prevLinearList.swap((linearList));
//
//			prevPListLengths.clear();
//			prevPListLengths.reserve(pListLengths.size());
//			prevPListLengths.swap((pListLengths));
//			pListLengths.reserve(0);
//
//			continueSearching = false;
//		}
//		else
//		{
//			//Have to add prediction here 
//			bool prediction = PredictHardDiskOrRAMProcessing(levelInfo, levelRecordings[levelInfo.currLevel - 2]);
//			if(prediction)
//			{
//
//				DataType indexing = 0;
//				prevLocalPListArray->clear();
//				for(int piss = 0; piss < pListLengths.size(); piss++)
//				{
//					prevLocalPListArray->push_back(new vector<DataType>(linearList.begin() + indexing, linearList.begin() + indexing + pListLengths[piss]));
//					indexing += pListLengths[piss];
//				}
//
//				linearList.clear();
//				linearList.reserve(0);
//				pListLengths.clear();
//				pListLengths.reserve(0);
//				break;
//			}
//			else
//			{
//				
//				continueSearching = true;
//				
//				DispatchNewThreadsRAM(0, continueSearching, linearList, pListLengths, levelInfo, isThreadDefuncted);
//
//				prevLinearList.clear();
//				prevLinearList.reserve(linearList.size());
//				prevLinearList.swap((linearList));
//
//				prevPListLengths.clear();
//				prevPListLengths.reserve(pListLengths.size());
//				prevPListLengths.swap((pListLengths));
//				pListLengths.reserve(0);
//			}
//
//		}
//	}
//	return continueSearching;
//}
