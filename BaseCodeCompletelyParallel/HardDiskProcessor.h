#pragma once
#include "ChunkFactory.h"
#include "ProcessorStats.h"
#include "ProcessorConfig.h"
#include "MemoryUtils.h"
#include "Processor.h"
template<class DataType>
class HardDiskProcessor : Processor<DataType>
{
public:
	HardDiskProcessor(void)
	{
		chunkFileFactorio = ChunkFactory::instance();
	}
	~HardDiskProcessor(void)
	{
		delete fileIDMutex;
	}

	void PlantSeedProcessing(DataType patternCount, DataType overallFilePosition, LevelPackage &levelInfo)
	{
		prevFileNameList.clear();
		newFileNameList.clear();
		prevFileNameList.resize(ProcessorConfig::numThreads - 1);
		newFileNameList.resize(ProcessorConfig::numThreads - 1);

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
						
			threadPlantSeedPool->push_back(std::async(std::launch::async, &HardDiskProcessor<PListType>::PlantSeed, this, overallFilePosition, position, span, i));
					
			position += span;
		}


		vector<unsigned int> localWorkingThreads;
		for(unsigned int i = 0; i < threadsToDispatch; i++)
		{
			localWorkingThreads.push_back(i);
		}

		Processor<DataType>::WaitForThreads(localWorkingThreads, threadPlantSeedPool); 

		overallFilePosition += position;

		PlantSeedPostProcessing(levelInfo);

		threadPlantSeedPool->erase(threadPlantSeedPool->begin(), threadPlantSeedPool->end());
		(*threadPlantSeedPool).clear();
		delete threadPlantSeedPool;
	}

	vector<vector<string>> BuildThreadJobs(LevelPackage& levelInfo)
	{
		vector<string> files;
		for(int i = 0; i < fileList.size(); i++)
		{
			for(int j = 0; j < fileList[i].size(); j++)
			{
				files.push_back(fileList[i][j]);
			}
		}
		vector<vector<string>> balancedTruncList = ProcessThreadsWorkLoadHD(threadsToDispatch, levelInfo, files);
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
			levelInfo.useRAM = false;
			levelInfo.coreIndex = i;
			threadsDispatched++;
			vector<PListType> temp;

			threadPool->push_back(std::async(std::launch::async, &HardDiskProcessor::ThreadBranch, this, NULL, temp, balancedTruncList[i], levelInfo));
		}
		ProcessorStats::countMutex->unlock();

		Processor<DataType>::WaitForThreads(localWorkingThreads, threadPool);

		return balancedTruncList;
	}

	void ThreadBranch(vector<string> fileList, LevelPackage levelInfo)
	{
		PListType numPatternsToSearch = patternIndexList.size();
		bool isThreadDefuncted = false;

		int tempCurrentLevel = levelInfo.currLevel;
		int threadsToDispatch = ProcessorConfig::numThreads - 1;

		if(ProcessorStats::threadsDispatched - ProcessorStats::threadsDefuncted > threadsToDispatch)
		{
			cout << "WENT OVER THREADS ALLOCATION SIZE!" << endl;
		}

		bool continueSearching = true;
		bool prediction = true;

		while(continueSearching && prediction)
		{
			continueSearching = Process(prevLocalPListArray,  nextLocalPListArray, levelInfo, isThreadDefuncted);
			
			prediction = PredictHardDiskOrRAMProcessing(levelInfo, ProcessorStats::levelRecordings[levelInfo.currLevel - 2]);
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

	//First level post processing 
	void PlantSeedPostProcessing(LevelPackage &levelInfo)
	{
		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		for(int z = 0; z < threadsToDispatch; z++)
		{
			for(int a = 0; a < newFileNameList[z].size(); a++)
			{
				backupFilenames.push_back(newFileNameList[z][a]);
			}
		}
		
		for(int a = 0; a < newFileNameList.size(); a++)
		{
			newFileNameList[a].clear();
		}

		PListType memDivisor = (PListType)((ProcessorConfig::memoryPerThread*1000000)/3.0f);
		vector<string> temp;

		if(ProcessorConfig::levelToOutput == 0 || (ProcessorConfig::levelToOutput != 0 && ProcessorStats::globalLevel >= ProcessorConfig::levelToOutput))
		{
			ProcessChunksAndGenerateLargeFile(backupFilenames, temp, memDivisor, 0, 1, true);
		}
		
	}

	void PlantSeed(DataType positionInFile, DataType startPatternIndex, DataType numPatternsToSearch, DataType threadNum)
	{
		LevelPackage levelInfo;
		levelInfo.currLevel = ProcessorStats::globalLevel;
		levelInfo.coreIndex = threadNum;
		levelInfo.threadIndex = threadNum;

		int threadsToDispatch = ProcessorConfig::numThreads - 1;
		PListType earlyApproximation = ProcessorConfig::files[ProcessorConfig::f]->fileString.size()/(256*(ProcessorConfig::numThreads - 1));
		vector<vector<PListType>*> leaves(256);
		for(int i = 0; i < 256; i++)
		{
			leaves[i] = new vector<PListType>();
			leaves[i]->reserve(earlyApproximation);
		}

		PListType counting = 0;
		PListType memDivisorInMB = (PListType)(ProcessorConfig::memoryPerThread/3.0f);
		//used primarily for just storage containment
		for (PListType i = startPatternIndex; i < numPatternsToSearch + startPatternIndex; i++)
		{
			int temp = i + positionInFile + 1;
			uint8_t tempIndex = (uint8_t)ProcessorConfig::files[ProcessorConfig::f]->fileString[i];
			if(ProcessorConfig::patternToSearchFor.size() == 0 || ProcessorConfig::files[ProcessorConfig::f]->fileString[i] == ProcessorConfig::patternToSearchFor[0])
			{
				leaves[tempIndex]->push_back(temp);
				counting++;
			}
			if(ProcessorStats::overMemoryCount && counting >= PListArchive::hdSectorSize)
			{
				stringstream stringBuilder;
				stringBuilder << chunkFileFactorio->GenerateUniqueID();
				newFileNameList[threadNum].push_back(chunkFileFactorio->CreateChunkFile(stringBuilder.str(), leaves, levelInfo));
				for(int i = 0; i < 256; i++)
				{
					leaves[i] = new vector<PListType>();
					leaves[i]->reserve(earlyApproximation);
				}
			}
		}

		stringstream stringBuilder;
		stringBuilder << chunkFileFactorio->GenerateUniqueID();
		newFileNameList[threadNum].push_back(chunkFileFactorio->CreateChunkFile(stringBuilder.str(), leaves, levelInfo));
	
		return;
	}

	bool ProcessHD(LevelPackage& levelInfo, vector<string>& fileList, bool &isThreadDefuncted)
	{
		double threadMemoryConsumptionInMB = MemoryUtils::GetProgramMemoryConsumption();

		int threadNum = levelInfo.threadIndex;
		unsigned int currLevel = levelInfo.currLevel;
		PListType newPatternCount = 0;
		//Divide between file load and previous level pLists and leave some for new lists haha 
		PListType memDivisor = (PListType)(((ProcessorConfig::memoryPerThread*1000000)/3.0f));

		bool morePatternsToFind = false;

		unsigned int fileIters = (unsigned int)(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize/memDivisor);
		if(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize%memDivisor != 0)
		{
			fileIters++;
		}

		vector<string> fileNamesToReOpen;
		string saveOffPreviousStringData = "";
		vector<string> newFileNames;

		PListType globalTotalMemoryInBytes = 0;
		PListType globalTotalLeafSizeInBytes = 0;
		try
		{
			for(PListType prevChunkCount = 0; prevChunkCount < fileList.size(); prevChunkCount++)
			{
				PListArchive archive(fileList[prevChunkCount]);
				while(!archive.IsEndOfFile())
				{

					vector<vector<PListType>*> packedPListArray;
					archive.GetPListArchiveMMAP(packedPListArray, memDivisor/1000000.0f); 

					if(packedPListArray.size() > 0)
					{
						PListType packedListSize = packedPListArray.size();
						vector <PListType> prevLeafIndex(packedListSize, 0);

						//Get minimum and maximum indexes so we can see if some chunks can be skipped from being loaded bam!
						PListType minimum = -1;
						PListType maximum = 0;
						for(int m = 0; m < packedPListArray.size(); m++)
						{
							for(int n = 0; n < packedPListArray[m]->size(); n++)
							{
								if((*packedPListArray[m])[0] < minimum)
								{
									minimum = (*packedPListArray[m])[0];
								}
							}
						}

						unsigned int firstIndex = (unsigned int)(minimum/memDivisor);
						unsigned int lastIndex = fileIters;

						int threadChunkToUse = threadNum;
						for(unsigned int j = firstIndex; j < lastIndex && minimum != -1; j++)
						{
							if(packedListSize > 0)
							{
								if(fileChunks.size() > threadChunkToUse && fileChunks[threadChunkToUse].size() > 0)
								{
									saveOffPreviousStringData = fileChunks[threadChunkToUse].substr(fileChunks[threadChunkToUse].size() - (currLevel - 1), currLevel - 1);
								}

								PListType patternCount = 0;
								if(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize <= memDivisor*j + memDivisor)
								{
									patternCount = ProcessorConfig::files[ProcessorConfig::f]->fileStringSize - memDivisor*j;
								}
								else
								{
									patternCount = memDivisor;
								}


								ProcessorStats::countMutex->lock();
								bool foundChunkInUse = false;
								for(map<unsigned int, unsigned int>::iterator iterator = chunkIndexToFileChunk.begin(); iterator != chunkIndexToFileChunk.end(); iterator++)
								{
									if(iterator->first == j)
									{
										threadChunkToUse = iterator->second;
										foundChunkInUse = true;
										break;
									}
								}

								if(!foundChunkInUse)
								{
									writingFlag = true;

									//Shared file space test
									fileChunks.push_back("");
									fileChunks[fileChunks.size() - 1].resize(patternCount);

									PListType offset = memDivisor*j;
									bool isFile;
									FileReader fileReaderTemp(ProcessorConfig::files[ProcessorConfig::f]->fileName, isFile, true);
									fileReaderTemp.copyBuffer->seekg( offset );
									fileReaderTemp.copyBuffer->read( &fileChunks[fileChunks.size() - 1][0], patternCount );

									threadChunkToUse = (int)(fileChunks.size() - 1);

									chunkIndexToFileChunk[j] = threadChunkToUse;

									writingFlag = false;

								}
								else
								{
									//otherwise use what has already been lifted from file
								}
								ProcessorStats::countMutex->unlock();

							}

							TreeHD leaf;
							//Start over
							globalTotalLeafSizeInBytes = 0;
							globalTotalMemoryInBytes = 32;

							bool justPassedMemorySize = false;

							for(PListType i = 0; i < packedListSize; i++)
							{
								vector<PListType>* pList = packedPListArray[i];
								PListType pListLength = packedPListArray[i]->size();
								PListType k = prevLeafIndex[i];

								if(leaf.leaves.size() > 0)
								{
									//Size needed for each node in the map overhead essentially
									globalTotalLeafSizeInBytes = (leaf.leaves.size() + 1)*32;
									globalTotalLeafSizeInBytes += (leaf.leaves.size() + 1)*(leaf.leaves.begin()->first.capacity() + sizeof(string));
									globalTotalLeafSizeInBytes += leaf.leaves.size() * 32;
									//Size of TreeHD pointer
									globalTotalLeafSizeInBytes += leaf.leaves.size() * 8;

								}

								if(((globalTotalLeafSizeInBytes/1000000.0f) + (globalTotalMemoryInBytes/1000000.0f)) < (memDivisor/1000000.0f)/* && !overMemoryCount*/)
								{
									/*stringstream crap;
									crap << "Approximation overflow at Process HD of " << sizeInMB << " in MB!\n";
									crap << "Overflow at Process HD of " << currMemoryOverflow << " in MB!\n";
									Logger::WriteLog(crap.str());*/
									signed long long relativeIndex = 0;
									PListType indexForString = 0;
									while( k < pListLength && ((*pList)[k]) < (j+1)*memDivisor )
									{
										if(!writingFlag)
										{

											try
											{
												if(((*pList)[k]) < ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
												{
													//If index comes out to be larger than fileString than it is a negative number 
													//and we must use previous string data!
													if(((((*pList)[k]) - memDivisor*j) - (currLevel-1)) >= ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
													{
														relativeIndex = ((((*pList)[k]) - memDivisor*j) - (currLevel-1));
														string pattern = "";
														relativeIndex *= -1;
														indexForString = saveOffPreviousStringData.size() - relativeIndex;
														if(saveOffPreviousStringData.size() > 0 && relativeIndex > 0)
														{

															pattern = saveOffPreviousStringData.substr(indexForString, relativeIndex);
															pattern.append(fileChunks[threadChunkToUse].substr(0, currLevel - pattern.size()));

															if(ProcessorConfig::patternToSearchFor.size() == 0 || pattern[pattern.size() - 1] == ProcessorConfig::patternToSearchFor[levelInfo.currLevel - 1])
															{
																if(leaf.leaves.find(pattern) != leaf.leaves.end())
																{
																	globalTotalMemoryInBytes -= leaf.leaves[pattern].pList.capacity()*sizeof(PListType);
																}
																leaf.addLeaf((*pList)[k]+1, pattern);

																globalTotalMemoryInBytes += leaf.leaves[pattern].pList.capacity()*sizeof(PListType);
															}

														}
													}
													else
													{
														//If pattern is past end of string stream then stop counting this pattern
														if(((*pList)[k]) < ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
														{

															string pattern = fileChunks[threadChunkToUse].substr(((((*pList)[k]) - memDivisor*j) - (currLevel-1)), currLevel);

															if(ProcessorConfig::patternToSearchFor.size() == 0 || pattern[pattern.size() - 1] == ProcessorConfig::patternToSearchFor[levelInfo.currLevel - 1])
															{
																leaf.addLeaf((*pList)[k]+1, pattern);
																globalTotalMemoryInBytes += sizeof(PListType);
															}
														}
														else if(((((*pList)[k]) - memDivisor*j) - (currLevel-1)) < 0)
														{
															cout << "String access is out of bounds at beginning" << endl;
														}
														else if((((*pList)[k]) - memDivisor*j) >= ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
														{
															cout << "String access is out of bounds at end" << endl;
														}
													}
												}
												else
												{
													ProcessorStats::eradicatedPatterns++;
													//cout << "don't pattern bro at this index: " << ((*pList)[k]) << endl;
												}
											}
											catch(exception e)
											{
												cout << "Exception at global index: " << (*pList)[k] << "Exception at relative index: " << ((((*pList)[k]) - memDivisor*j) - (currLevel-1)) << " and computed relative index: " << relativeIndex << " and index for string: " << indexForString << " System exception: " << e.what() << endl;
											}
											k++;
										}
									}
									prevLeafIndex[i] = k;
									justPassedMemorySize = false;
								}
								else
								{

									globalTotalLeafSizeInBytes = 0;
									globalTotalMemoryInBytes = 32;
									//if true already do not write again until memory is back in our hands
									if(!justPassedMemorySize && leaf.leaves.size() > 0)
									{

										PListType patterns = leaf.leaves.size();

										justPassedMemorySize = true;
										stringstream stringBuilder;
										fileIDMutex->lock();
										fileID++;
										stringBuilder << fileID;
										fileIDMutex->unlock();
										fileNamesToReOpen.push_back(chunkFileFactorio->CreateChunkFile(stringBuilder.str(), leaf, levelInfo));

									}
									else
									{

										//If memory is unavailable sleep for one second
										//std::this_thread::sleep_for (std::chrono::seconds(1));
									}
									i--;
								}
							}

							if(packedListSize > 0 && leaf.leaves.size() > 0)
							{

								globalTotalLeafSizeInBytes = 0;
								globalTotalMemoryInBytes = 32;

								stringstream stringBuilder;
								fileIDMutex->lock();
								fileID++;
								stringBuilder << fileID;
								fileIDMutex->unlock();
								fileNamesToReOpen.push_back(chunkFileFactorio->CreateChunkFile(stringBuilder.str(), leaf, levelInfo));

							}
						}
					
						for(PListType pTits = 0; pTits < packedPListArray.size(); pTits++)
						{
							delete packedPListArray[pTits];
						}
					}
				}
				archive.CloseArchiveMMAP();
			}

			if(ProcessorConfig::levelToOutput == 0 || (ProcessorConfig::levelToOutput != 0 && currLevel >= ProcessorConfig::levelToOutput))
			{
				newPatternCount += ProcessChunksAndGenerate(fileNamesToReOpen, newFileNames, memDivisor, threadNum, currLevel, levelInfo.coreIndex);
			}
			//Logger::WriteLog("Eradicated patterns: " + std::to_string(eradicatedPatterns) + "\n");
		}
		catch(exception e)
		{
			cout << e.what() << endl;
			MemoryUtils::print_trace();
		}

		if(!ProcessorConfig::history)
		{
			chunkFileFactorio->DeleteArchives(fileList, ARCHIVE_FOLDER);
		}

		fileList.clear();
		for(int i = 0; i < newFileNames.size(); i++)
		{
			fileList.push_back(newFileNames[i]);
		}

		newFileNames.clear();

		if(fileList.size() > 0 && levelInfo.currLevel < ProcessorConfig::maximum)
		{
			morePatternsToFind = true;
			levelInfo.currLevel++;

			DispatchNewThreadsHD(newPatternCount, morePatternsToFind, fileList, levelInfo, isThreadDefuncted);

		}
		else
		{
			chunkFileFactorio->DeleteArchives(fileList, ARCHIVE_FOLDER);
		}
		return morePatternsToFind;
	}

	PListType ProcessChunksAndGenerateLargeFile(vector<string> fileNamesToReOpen, vector<string>& newFileNames, PListType memDivisor, unsigned int threadNum, unsigned int currLevel, bool firstLevel)
	{
		int currentFile = 0;
		int prevCurrentFile = currentFile;
		bool memoryOverflow = false;
		PListType interimCount = 0;
		unsigned int threadNumber = 0;
		unsigned int threadsToDispatch = ProcessorConfig::numThreads - 1;

		PListType currPatternCount = 0;
		//Approximate pattern count for this level
		if(currLevel == 1)
		{
			currPatternCount = 256;
		}
		else
		{
			currPatternCount = 256*ProcessorStats::levelRecordings[currLevel - 1];
		}


		map<string, PListArchive*> currChunkFiles;
		for(int a = 0; a < currPatternCount; a++)
		{
			stringstream fileNameage;
			stringstream fileNameForPrevList;
			PListType tempID = ChunkFactory::instance()->GenerateUniqueID();
			fileNameage << ARCHIVE_FOLDER << "PListChunks" << tempID << ".txt";
			fileNameForPrevList << "PListChunks" << tempID;

			prevFileNameList[threadNumber].push_back(fileNameForPrevList.str());

			stringstream pattern;
			pattern << (char)a;
			currChunkFiles[pattern.str()] = new PListArchive(fileNameForPrevList.str(), true);

			threadNumber++;
			threadNumber %= threadsToDispatch;
		}

		vector<string> fileNamesBackup;
		for(int a = 0; a < fileNamesToReOpen.size(); a++)
		{
			fileNamesBackup.push_back(fileNamesToReOpen[a]);
		}

		int removedPatterns = 0;
		map<string, pair<PListType, PListType>> patternCounts;
		while(currentFile < fileNamesBackup.size())
		{
			memoryOverflow = false;

			vector<PListArchive*> archiveCollection;

			for(int a = 0; a < fileNamesBackup.size() - prevCurrentFile; a++)
			{

				archiveCollection.push_back(new PListArchive(fileNamesBackup[a+prevCurrentFile]));

				//Our job is to trust whoever made the previous chunk made it within the acceptable margin of error so we compensate by saying take up to double the size if the previous
				//chunk went a little over the allocation constraint

				vector<vector<PListType>*> packedPListArray;
				vector<string> *stringBuffer = NULL;
				PListArchive *stringBufferFile = NULL;
				string fileNameForLater ="";
				PListType packedPListSize = 0; 
				string fileName ="";
				bool foundAHit = true;


				if(ProcessorStats::overMemoryCount && !memoryOverflow)
				{
					stringstream crap;
					crap << "Overflow at Process Chunks And Generate of " << ProcessorStats::currMemoryOverflow << " in MB!\n";
					Logger::WriteLog(crap.str());
				}
				else if(ProcessorStats::overMemoryCount && memoryOverflow)
				{
					stringstream crap;
					crap << "Overflow at Process Chunks And Generate of " << ProcessorStats::currMemoryOverflow << " in MB!\n";
					Logger::WriteLog(crap.str());
				}


				archiveCollection[a]->GetPListArchiveMMAP(packedPListArray); //Needs MB
				packedPListSize = packedPListArray.size();

				std::string::size_type i = archiveCollection[a]->fileName.find(".txt");
				std::string::size_type begin = archiveCollection[a]->fileName.find("P");
				string tempString = archiveCollection[a]->fileName;
				tempString.erase(i);
				tempString.erase(0, begin);

				fileNameForLater.append(tempString);
				fileName = fileNameForLater;
				fileName.append("Patterns");
				stringBufferFile = new PListArchive(fileName);
				std::string::size_type j = archiveCollection[a]->fileName.find("_");
				string copyString = archiveCollection[a]->fileName;
				copyString.erase(0, j + 1);
				std::string::size_type k = copyString.find(".txt");
				copyString.erase(k, 4);
				std::string::size_type sz;   // alias of size_t
				PListType sizeOfPackedPList = std::stoll (copyString,&sz);
				stringBuffer = stringBufferFile->GetPatterns(currLevel, packedPListSize);


				PListType countAdded = 0;
				if(foundAHit)
				{
					for(PListType partialLists = 0; partialLists < packedPListArray.size(); partialLists++)
					{
						try
						{

							if(ProcessorStats::overMemoryCount && !memoryOverflow )
							{
								stringstream crap;
								crap << "Overflow at Process Chunks And Generate of " << ProcessorStats::currMemoryOverflow << " in MB!\n";
								Logger::WriteLog(crap.str());
							}

							string pattern = (*stringBuffer)[partialLists];
						
							if(patternCounts.find(pattern) != patternCounts.end())
							{
								patternCounts[pattern].first++;
							}
							else
							{
								patternCounts[pattern].first = 1;
								patternCounts[pattern].second = (*(packedPListArray[partialLists]))[0];
							}

							currChunkFiles[pattern]->WriteArchiveMapMMAP(*(packedPListArray[partialLists]));
							delete packedPListArray[partialLists];
							packedPListArray[partialLists] = NULL;

						}
						catch(exception e)
						{
							cout << "System exception: " << e.what() << endl;
						}
					}
				}

				archiveCollection[a]->CloseArchiveMMAP();
				stringBufferFile->CloseArchiveMMAP();

				ChunkFactory::instance()->DeleteChunk(fileNameForLater, ARCHIVE_FOLDER);

				delete archiveCollection[a];


				if(stringBuffer != NULL)
				{
					stringBuffer->clear();
					delete stringBuffer;
				}

				delete stringBufferFile;

				if(!memoryOverflow)
				{
					currentFile++;
				}
			}


			for(int a = 0; a < currPatternCount; a++)
			{
				stringstream buff;
				buff << (char)a;
				currChunkFiles[buff.str()]->WriteArchiveMapMMAP(vector<PListType>(), "", true);
				bool empty = true;
				PListType patterCount = (currChunkFiles[buff.str()]->prevMappingIndex/sizeof(PListType)) - sizeof(PListType);
				if(currChunkFiles[buff.str()]->mappingIndex > (2*sizeof(PListType)))
				{
					empty = false;
					interimCount++;
				

					if(ProcessorStats::mostCommonPatternIndex.size() < currLevel)
					{
						ProcessorStats::mostCommonPatternIndex.resize(currLevel);
						ProcessorStats::mostCommonPatternCount.resize(currLevel);
					}
				
					if(patternCounts.find(buff.str()) != patternCounts.end() && patternCounts[buff.str()].first > ProcessorStats::mostCommonPatternCount[currLevel - 1])
					{
						ProcessorStats::mostCommonPatternCount[currLevel - 1] = patternCounts[buff.str()].first;

						ProcessorStats::mostCommonPatternIndex[currLevel - 1] = patternCounts[buff.str()].second - currLevel;
					}
				}
				else if(currChunkFiles[buff.str()]->mappingIndex == (2*sizeof(PListType)))
				{
					ProcessorStats::countMutex->lock();
					ProcessorStats::eradicatedPatterns++;
					ProcessorStats::countMutex->unlock();
				}

				string fileToDelete = currChunkFiles[buff.str()]->patternName;
				currChunkFiles[buff.str()]->CloseArchiveMMAP();
				delete currChunkFiles[buff.str()];

				if(fileNamesBackup.size() == currentFile && empty)
				{
					ChunkFactory::instance()->DeleteChunk(fileToDelete, ARCHIVE_FOLDER);
				}
			}
		}

		ProcessorStats::countMutex->lock();
		if(ProcessorStats::levelRecordings.size() < currLevel)
		{
			ProcessorStats::levelRecordings.resize(currLevel);
		}
		ProcessorStats::levelRecordings[currLevel - 1] += interimCount;

		if(currLevel > ProcessorStats::currentLevelVector[threadNum])
		{
			ProcessorStats::currentLevelVector[threadNum] = currLevel;
		}

		ProcessorStats::countMutex->unlock();

		return interimCount;
	}

	mutex* GetFileMutex(){return fileMutex};
	DataType GetFileID(){return fileID};
	
	vector<vector<string>> GetPrevFileNameList(){return prevFileNameList;};
	vector<vector<string>> GetNewFileNameList(){return newFileNameList;};

	private:
		vector<string> fileChunks;
		bool writingFlag;
		mutex *fileIDMutex;
		DataType fileID;
		map<unsigned int, unsigned int> chunkIndexToFileChunk;
		ChunkFactory* chunkFileFactorio;
		vector<vector<string>> prevFileNameList;
		vector<vector<string>> newFileNameList;
		DataType overallFilePosition;
		vector<string> backupFilenames;

};

