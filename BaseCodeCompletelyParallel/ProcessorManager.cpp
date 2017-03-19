#include "ProcessorManager.h"
#include "ChunkFactory.h"

ProcessorManager::ProcessorManager(void)
{
	dramProc = new DRAMProcessor<PListType>();
	hdProc = new HardDiskProcessor<PListType>();
}


ProcessorManager::~ProcessorManager(void)
{
	delete dramProc;
	delete hdProc;
}

void ProcessorManager::ProcessFirstLevelDater(PListType patternCount, PListType overallFilePosition, LevelPackage &levelInfo)
{
	bool prediction = PredictHardDiskOrRAMProcessing(levelInfo, 1);

	if(!prediction)
	{
		dramProc->PlantSeedProcessing(patternCount, overallFilePosition, levelInfo);
	}
	else
	{
		hdProc->PlantSeedProcessing(patternCount, overallFilePosition, levelInfo);
	}
}

void ProcessorManager::CleanUp()
{
	dramProc->Cleanup();
}

void ProcessorManager::ProcessDater()
{

	unsigned int threadsToDispatch = ProcessorConfig::numThreads - 1;

	LevelPackage levelInfo;
	levelInfo.currLevel = 2;
	levelInfo.inceptionLevelLOL = 0;
	levelInfo.threadIndex = 0;
	levelInfo.useRAM = ProcessorStats::usedRAM[0];
	levelInfo.coreIndex = 0;

	//Do one prediction for them all
	bool prediction = PredictHardDiskOrRAMProcessing(levelInfo, ProcessorStats::levelRecordings[0]);

	vector<vector<string>> fileList = hdProc->GetPrevFileNameList();
	
	vector<vector<PListType>*>* prevList = dramProc->GetPrevPList();

	PrepDataFirstLevel(prediction, fileList, dramProc->GetPrevPList(), dramProc->GetNextPList());

	vector<vector<PListType>*>* prevLocalPListArray = NULL;
	vector<vector<string>> newFileList;
	bool leftRAMProcessing = false;

	while(true)
	{
		if(!prediction)
		{
			prevLocalPListArray = dramProc->BuildThreadJobs(levelInfo);
			prediction = !prediction;
		}
		else
		{
			newFileList = hdProc->BuildThreadJobs(levelInfo);
			prediction = !prediction;
		}
	}
}


bool ProcessorManager::PredictHardDiskOrRAMProcessing(LevelPackage levelInfo, PListType sizeOfPrevPatternCount)
{
	//Break early if memory usage is predetermined by command line arguments
	if(ProcessorConfig::usingPureRAM)
	{
		return false;
	}
	if(ProcessorConfig::usingPureHD)
	{
		return true;
	}
	//main thread is a hardware thread so dispatch threads requested minus 1
	PListType threadsToDispatch = ProcessorConfig::numThreads - 1;

	//predictedMemoryForLevelProcessing has to include memory loading in from previous level and memory for the next level
	//First calculate the size of the file because that is the maximum number of pLists we can store minus the level
	//Second calculate the size of Tree objects will be allocated based on the number of POTENTIAL PATTERNS...
	//POTENTIAL PATTERNS equals the previous list times 256 possible byte values but this value can't exceed the file size minus the current level
	PListType potentialPatterns = sizeOfPrevPatternCount*256;
	//cout << "Level " << levelInfo.currLevel << " had " << sizeOfPrevPatternCount << " patterns!" << endl;

	//cout << "Eradicated file indexes: " << eradicatedPatterns << endl;
	if(potentialPatterns > ProcessorConfig::files[ProcessorConfig::f]->fileStringSize - (levelInfo.currLevel - 1))
	{
		//Factor in eradicated patterns because those places no longer need to be checked in the file
		potentialPatterns = ProcessorConfig::files[ProcessorConfig::f]->fileStringSize - ProcessorStats::eradicatedPatterns;
		//cout << "Potential patterns has exceed file size so we downgrade to " << potentialPatterns << endl;
	}

	//cout << "Potential patterns for level " << levelInfo.currLevel << " is " << potentialPatterns << endl;


	PListType sizeOfTreeMap = 0;
	if(levelInfo.useRAM)
	{
		sizeOfTreeMap = sizeof(char)*potentialPatterns + sizeof(PListType)*potentialPatterns;
	}
	else
	{
		sizeOfTreeMap = sizeof(char)*levelInfo.currLevel*potentialPatterns + sizeof(TreeHD)*potentialPatterns;
	}

	PListType vectorSize = sizeOfPrevPatternCount*sizeof(vector<PListType>*);
	PListType pListIndexesLeft = (ProcessorConfig::files[ProcessorConfig::f]->fileStringSize - ProcessorStats::eradicatedPatterns)*sizeof(PListType);
	PListType predictedMemoryForNextLevelMB = (PListType)((vectorSize + pListIndexesLeft + sizeOfTreeMap)/1000000.0f);

	//cout << "Predicted size for level " << levelInfo.currLevel << " is " << predictedMemoryForNextLevelMB << " MB" << endl;

	PListType previousLevelMemoryMB = 0;
	PListType predictedMemoryForLevelProcessing = 0;

	double prevMemoryMB = MemoryUtils::GetProgramMemoryConsumption() - ProcessorStats::MemoryUsageAtInception;
	if(prevMemoryMB > 0.0f)
	{
		previousLevelMemoryMB = (PListType)prevMemoryMB;
	}

	//cout << "Size used for previous level " << levelInfo.currLevel - 1 << " is " << previousLevelMemoryMB << " MB" << endl;

	predictedMemoryForLevelProcessing = previousLevelMemoryMB + predictedMemoryForNextLevelMB;

	if(predictedMemoryForLevelProcessing > ProcessorConfig::memoryBandwidthMB)
	{

		stringstream stringbuilder;
		stringbuilder << "Using HARD DISK! Total size for level " << levelInfo.currLevel << " processing is " << predictedMemoryForLevelProcessing << " MB" << endl;
		cout << stringbuilder.str();
		Logger::WriteLog(stringbuilder.str());

		return true;
	}
	else
	{
		if(ProcessorConfig::files[ProcessorConfig::f]->fileString.size() == 0)
		{
			//new way to read in file
			ProcessorConfig::files[ProcessorConfig::f]->fileString.resize(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize);
			ProcessorConfig::files[ProcessorConfig::f]->copyBuffer->read( &ProcessorConfig::files[ProcessorConfig::f]->fileString[0], ProcessorConfig::files[ProcessorConfig::f]->fileString.size());
		}

		//stringstream stringbuilder;
		//stringbuilder << "Using RAM! Total size for level " << levelInfo.currLevel << " processing is " << predictedMemoryForLevelProcessing << " MB" << endl;
		//cout << stringbuilder.str();
		//Logger::WriteLog(stringbuilder.str());

		return false;
	}
}

void ProcessorManager::PrepData(bool prediction, LevelPackage& levelInfo, vector<string>& fileList, vector<vector<PListType>*>* prevLocalPListArray)
{
	PListType threadsToDispatch = ProcessorConfig::numThreads - 1;

	if(prediction)
	{
		if(levelInfo.useRAM)
		{
			//chunk file
			PListArchive* threadFile;
			stringstream threadFilesNames;

			fileList.clear();

			threadFilesNames.str("");
			
			threadFilesNames << "PListChunks" << ChunkFactory::instance()->GenerateUniqueID();

			threadFile = new PListArchive(threadFilesNames.str(), true);
			fileList.push_back(threadFilesNames.str());

			for(PListType i = 0; i < prevLocalPListArray->size(); i++)
			{
				list<PListType> *sorting = new list<PListType>();
				copy( (*prevLocalPListArray)[i]->begin(), (*prevLocalPListArray)[i]->end(), std::back_inserter(*sorting));
				((*prevLocalPListArray)[i])->erase(((*prevLocalPListArray)[i])->begin(), ((*prevLocalPListArray)[i])->end());
				sorting->sort();
				std::copy( sorting->begin(), sorting->end(), std::back_inserter(*((*prevLocalPListArray)[i])));
				sorting->clear();
				delete sorting;

				threadFile->WriteArchiveMapMMAP(*(*prevLocalPListArray)[i]);

				delete (*prevLocalPListArray)[i];

				if(threadFile->totalWritten >= PListArchive::writeSize) 
				{
					threadFile->WriteArchiveMapMMAP(vector<PListType>(), "", true);
				}

			}
			//Clear out the array also after deletion
			prevLocalPListArray->clear();

			threadFile->WriteArchiveMapMMAP(vector<PListType>(), "", true);
			threadFile->CloseArchiveMMAP();
			delete threadFile;
		}
	}
	else if(!prediction)
	{
		if(!levelInfo.useRAM)
		{
			for(PListType prevChunkCount = 0; prevChunkCount < fileList.size(); prevChunkCount++)
			{
				PListArchive archive(fileList[prevChunkCount]);
				while(archive.Exists() && !archive.IsEndOfFile())
				{
					//Just use 100 GB to say we want the whole file for now
					vector<vector<PListType>*> packedPListArray;
					archive.GetPListArchiveMMAP(packedPListArray);
					prevLocalPListArray->insert(prevLocalPListArray->end(), packedPListArray.begin(), packedPListArray.end());

					packedPListArray.erase(packedPListArray.begin(), packedPListArray.end());
				}
				archive.CloseArchiveMMAP();
			}
			if(!ProcessorConfig::history)
			{
				ChunkFactory::instance()->DeleteArchives(fileList, ARCHIVE_FOLDER);
			}
			fileList.clear();
		}
	}

	levelInfo.useRAM = !prediction;
	ProcessorStats::usedRAM[levelInfo.threadIndex] = !prediction;

	if(levelInfo.useRAM && ProcessorConfig::files[ProcessorConfig::f]->fileString.size() != ProcessorConfig::files[ProcessorConfig::f]->fileStringSize)
	{
		//new way to read in file
		ProcessorConfig::files[ProcessorConfig::f]->copyBuffer->seekg( 0 );
		ProcessorConfig::files[ProcessorConfig::f]->fileString.resize(ProcessorConfig::files[ProcessorConfig::f]->fileStringSize);
		ProcessorConfig::files[ProcessorConfig::f]->copyBuffer->read( &ProcessorConfig::files[ProcessorConfig::f]->fileString[0], ProcessorConfig::files[ProcessorConfig::f]->fileString.size());
	}
}

void ProcessorManager::PrepDataFirstLevel(bool prediction, vector<vector<string>>& fileList, vector<vector<PListType>*>* prevLocalPListArray, vector<vector<PListType>*>* nextLocalPListArray)
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
				threadFilesNames << "PListChunks" << ChunkFactory::instance()->GenerateUniqueID();

				threadFiles.push_back(new PListArchive(threadFilesNames.str(), true));
				fileList[a].push_back(threadFilesNames.str());
			}
			for(PListType prevIndex = 0; prevIndex < prevLocalPListArray->size(); )
			{
				list<PListType> *sorting = new list<PListType>();

				for(int threadCount = 0; threadCount < threadsToDispatch; threadCount++)
				{
					if((*prevLocalPListArray)[prevIndex] != NULL)
					{
						copy( (*prevLocalPListArray)[prevIndex]->begin(), (*prevLocalPListArray)[prevIndex]->end(), std::back_inserter(*sorting));
						((*prevLocalPListArray)[prevIndex])->erase(((*prevLocalPListArray)[prevIndex])->begin(), ((*prevLocalPListArray)[prevIndex])->end());
						delete (*prevLocalPListArray)[prevIndex];
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
			prevLocalPListArray->clear();

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
			prevLocalPListArray->clear();
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

						prevLocalPListArray->insert(prevLocalPListArray->end(), packedPListArray.begin(), packedPListArray.end());

						packedPListArray.erase(packedPListArray.begin(), packedPListArray.end());
					}
					archive.CloseArchiveMMAP();
				}
			}

			for(PListType i = 0; i < threadsToDispatch; i++)
			{
				if(!ProcessorConfig::history)
				{
					ChunkFactory::instance()->DeleteArchives(tempFileList[i], ARCHIVE_FOLDER);
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