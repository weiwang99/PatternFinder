#include "ProcessorConfig.h"
#include "ProcessorStats.h"
#include "Logger.h"
#include <locale>

unsigned int ProcessorConfig::numThreads;
vector<FileReader*> ProcessorConfig::files;
int ProcessorConfig::f;
string ProcessorConfig::patternToSearchFor;
vector<PListType> ProcessorConfig::fileSizes;
unsigned int ProcessorConfig::levelToOutput = 0;
int ProcessorConfig::history = 0;
PListType ProcessorConfig::minimum;
PListType ProcessorConfig::maximum;
PListType ProcessorConfig::memoryBandwidthMB = 0;
PListType ProcessorConfig::memoryPerThread;
bool ProcessorConfig::displayEachLevelSearch = false;
bool ProcessorConfig::findBestThreadNumber = false;
bool ProcessorConfig::usingMemoryBandwidth = false;
unsigned int ProcessorConfig::testIterations;
bool ProcessorConfig::usingPureRAM = false;
bool ProcessorConfig::usingPureHD = false;
bool ProcessorConfig::outlierScans;
PListType ProcessorConfig::minOccurrence;


ProcessorConfig::ProcessorConfig(void)
{
}


ProcessorConfig::~ProcessorConfig(void)
{
}

void ProcessorConfig::CommandLineParser(int argc, char **argv)
{
	bool minEnter = false;
	bool maxEnter = false;
	bool fileEnter = false;
	bool threadsEnter = false;
	bool coverageTracking = false;
	//All files need to be placed in data folder relative to your executable
	string tempFileName = DATA_FOLDER;

	for (int i = 1; i < argc; i++)
	{
		string arg(argv[i]);
		locale loc;
		for (std::string::size_type j = 0; j < arg.length(); ++j)
		{
			arg[j] = std::tolower(arg[j], loc);
		}
		if (arg.compare("-min") == 0)
		{
			// We know the next argument *should* be the minimum pattern to display
			minimum = atoi(argv[i + 1]);
			minEnter = true;
			i++;
		}
		else if (arg.compare("-max") == 0)
		{
			// We know the next argument *should* be the maximum pattern to display
			maximum = atoi(argv[i + 1]);
			maxEnter = true;
			i++;
		}
		else if (arg.compare("-f") == 0)
		{
			bool isFile = false;
			// We know the next argument *should* be the filename
			string header = DATA_FOLDER;
			tempFileName.append(argv[i + 1]);
			string fileTest = argv[i + 1];

			if(fileTest.find('.') != string::npos && fileTest[0] != '-') 
			{
				ProcessorConfig::files.push_back(new FileReader(tempFileName, isFile));
				fileSizes.push_back(files.back()->fileStringSize);
				i++;
			}
			else if(fileTest.find('.') == string::npos && fileTest[0] != '-')
			{
			#if defined(_WIN64) || defined(_WIN32)
				header = "../../../../Database/";
			#elif defined(__linux__)
				header = "../../Database/";
			#endif
				header.append(fileTest);
				header.append("/");

				//Access files with full path
				if(fileTest.find(":") != std::string::npos)
				{
					header = fileTest;
					header.append("/");
				}

				FindFiles(header);
				i++;
			}
			else
			{
				FindFiles(header);	
			}
			fileEnter = true;
		}
		else if (arg.compare("-v") == 0)
		{
			// We know the next argument *should* be the filename
			Logger::verbosity = atoi(argv[i + 1]);
			i++;
		}
		else if (arg.compare("-d") == 0)
		{
			displayEachLevelSearch = true;
		}
		else if (arg.compare("-c") == 0)
		{
			findBestThreadNumber = true;
		}
		else if (arg.compare("-threads") == 0)
		{
			// We know the next argument *should* be the maximum pattern to display
			ProcessorConfig::numThreads = atoi(argv[i + 1]);
			threadsEnter = true;
			i++;
		}
		else if(arg.compare("-mem") == 0)
		{
			memoryBandwidthMB = atoi(argv[i + 1]);
			usingMemoryBandwidth = true;
			i++;
		}
		else if(arg.compare("-lev") == 0 )
		{
			levelToOutput = atoi(argv[i + 1]);
			i++;
		}
		else if(arg.compare("-his") == 0)
		{
			history = atoi(argv[i + 1]);
			i++;
		}
		else if(arg.compare("-ram") == 0)
		{
			usingPureRAM = true;
		}
		else if(arg.compare("-hd") == 0)
		{
			usingPureHD = true;
		}
		else if(arg.compare("-p") == 0)
		{
			patternToSearchFor = argv[i+1];
			maximum = patternToSearchFor.size();
			i++;
		}
		else if(arg.compare("-o") == 0)
		{
			minOccurrence = atoi(argv[i+1]);
			i++;
		}
		else if(arg.compare("-cov") == 0)
		{
			coverageTracking = true;
			i++;
		}
		else if(arg.compare("-s") == 0)
		{
			outlierScans = true;
		}
		else if(arg.compare("-help") == 0 || arg.compare("/?") == 0)
		{
			DisplayHelpMessage();
			do
			{
				cout << '\n' <<"Press the Enter key to continue." << endl;
			} while (cin.get() != '\n');
			exit(0);
		}
		else
		{
			cout << "incorrect command line format at : " << arg << endl;
			DisplayHelpMessage();
			do
			{
				cout << '\n' <<"Press the Enter key to continue." << endl;
			} while (cin.get() != '\n');
			exit(0);
		}
	}

	//Make maximum the largest if not entered
	if(!maxEnter)
	{
		maximum = -1;
	}

	if(outlierScans)
	{
		minOccurrence = -1;
	}

	//If no file is entered we exit because there is nothing to play with
	if (!fileEnter)
	{
		exit(0);
	}

	unsigned long concurentThreadsSupported = std::thread::hardware_concurrency();

	stringstream buff;
	buff << "Number of threads on machine: " << concurentThreadsSupported << endl;
	Logger::WriteLog(buff.str());
	cout << buff.str();

	////If max not specified then make the largest pattern the fileSize
	//if (!maxEnter)
	//{
	//	maximum = files[f]->fileStringSize;
	//}
	//If min not specified then make the smallest pattern of 0
	if (!minEnter)
	{
		minimum = 0;
	}
	//If numCores is not specified then we use number of threads supported cores plus the main thread
	if (!threadsEnter /*|| ProcessorConfig::numThreads > concurentThreadsSupported*/)
	{
		ProcessorConfig::numThreads = concurentThreadsSupported + 1;
	}

	int bestThreadCount = 0;
	double fastestTime = 1000000000.0f;
	testIterations = 0;
	if (findBestThreadNumber)
	{
		ProcessorConfig::numThreads = 2;
		testIterations = concurentThreadsSupported;
	}

	ProcessorStats::activeThreads.resize(ProcessorConfig::numThreads  - 1);
	ProcessorStats::usedRAM.resize(ProcessorConfig::numThreads  - 1);
}

void ProcessorConfig::DisplayHelpMessage()
{
	bool isFile;
	FileReader tempHelpFile(READMEPATH, isFile, true);
	tempHelpFile.LoadFile();
	cout << tempHelpFile.fileString << endl;
}

void ProcessorConfig::FindFiles(string directory)
{
	bool isFile = false;
#if defined(_WIN64) || defined(_WIN32)
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (directory.c_str())) != NULL) 
	{
		Logger::WriteLog("Files to be processed: \n");
		/* print all the files and directories within directory */
		while ((ent = readdir (dir)) != NULL) 
		{
			if(*ent->d_name)
			{
				string fileName = string(ent->d_name);

				if(!fileName.empty() && fileName != "." && fileName !=  ".." && fileName.find(".") != std::string::npos && fileName.find(".ini") == std::string::npos)
				{
					string name = string(ent->d_name);
					Logger::WriteLog(name + "\n");
					//cout << name << endl;
					string tempName = directory;
					tempName.append(ent->d_name);
					FileReader* file = new FileReader(tempName, isFile);
					if(isFile)
					{
						files.push_back(file);
						fileSizes.push_back(files.back()->fileStringSize);
					}
					else //This is probably a directory then
					{
						FindFiles(directory + fileName + "/");
					}
				}
				else if(fileName != "." && fileName !=  ".." && fileName.find(".ini") == std::string::npos)
				{
					FindFiles(directory + fileName + "/");
				}
			}
		}
		closedir (dir);
	} else
	{
		//cout << "Problem reading from directory!" << endl;
	}
#elif defined(__linux__)
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(directory.c_str())))
		return;
	if (!(entry = readdir(dir)))
		return;
	do {
		
		string fileName = string(entry->d_name);

		if(!fileName.empty() && fileName != "." && fileName !=  ".." && fileName.find(".") != std::string::npos && fileName.find(".ini") == std::string::npos)
		{
			string name = string(entry->d_name);
			Logger::WriteLog(name + "\n");
			//cout << name << endl;
			string tempName = directory;
			tempName.append(entry->d_name);

			FileReader* file = new FileReader(tempName, isFile);
			if(isFile)
			{
				files.push_back(file);
				fileSizes.push_back(files.back()->fileStringSize);
			}
			else //This is probably a directory then
			{
				FindFiles(directory + fileName + "/");
			}
		}
		else if(fileName != "." && fileName !=  ".." && fileName.find(".ini") == std::string::npos)
		{
			FindFiles(directory + fileName + "/");
		}
					
	} while (entry = readdir(dir));
	closedir(dir);
#endif		
}