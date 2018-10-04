#include <sys/sysinfo.h>
#include <unistd.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <cmath>

using namespace std;

const int NUM_CPU_STATES = 10;
bool loggingEnabled;
std::fstream logFile;
unsigned long memory = 0;
int count = 1;

struct sysinfo info;

enum CPUStates
{
	S_USER = 0,
	S_NICE,
	S_SYSTEM,
	S_IDLE,
	S_IOWAIT,
	S_IRQ,
	S_SOFTIRQ,
	S_STEAL,
	S_GUEST,
	S_GUEST_NICE
};

typedef struct CPUData
{
	std::string cpu;
	size_t times[NUM_CPU_STATES];
} CPUData;

void ReadStatsCPU(std::vector<CPUData> & entries);

size_t GetIdleTime(const CPUData & e);
size_t GetActiveTime(const CPUData & e);

void PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2);

void enableLogging();

int main(int argc, char * argv[])
{
	std::vector<CPUData> entries1;
	std::vector<CPUData> entries2;
	enableLogging();
	while(1){
		// snapshot 1
		ReadStatsCPU(entries1);
		// 100ms pause
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		// snapshot 2
		ReadStatsCPU(entries2);
		// print output
		PrintStats(entries1, entries2);
	}
	return 0;
}

void enableLogging()
{
	//save the weights
	char file_name[255];
	int file_no = 0;
	sprintf(file_name,"log%d.csv",file_no);
	for ( int i=0 ; i < 100 ; i++ )
	{
		ifstream test(file_name);
		if (!test) break;
		file_no++;
		sprintf(file_name,"log%d.csv",file_no);
	}
	logFile.open(file_name,ios::out);
	if ( logFile.is_open() )
	{
		logFile << "Time(s),CPU[Total](%),CPU[0](%),CPU[1](%),CPU[2](%),CPU[3](%),Memory(%)" << endl;
		loggingEnabled = true;
		cout << "Logged at : " << file_name << endl;
	}
}

void ReadStatsCPU(std::vector<CPUData> & entries)
{
	std::ifstream fileStat("/proc/stat");

	std::string line;

	const std::string STR_CPU("cpu");
	const std::size_t LEN_STR_CPU = STR_CPU.size();
	const std::string STR_TOT("tot");

	while(std::getline(fileStat, line))
	{
		// cpu stats line found
		if(!line.compare(0, LEN_STR_CPU, STR_CPU))
		{
			std::istringstream ss(line);

			// store entry
			entries.emplace_back(CPUData());
			CPUData & entry = entries.back();

			// read cpu label
			ss >> entry.cpu;

			// remove "cpu" from the label when it's a processor number
			if(entry.cpu.size() > LEN_STR_CPU)
				entry.cpu.erase(0, LEN_STR_CPU);
			// replace "cpu" with "tot" when it's total values
			else
				entry.cpu = STR_TOT;

			// read times
			for(int i = 0; i < NUM_CPU_STATES; ++i)
				ss >> entry.times[i];
		}
	}
}

size_t GetIdleTime(const CPUData & e)
{
	return	e.times[S_IDLE] + 
			e.times[S_IOWAIT];
}

size_t GetActiveTime(const CPUData & e)
{
	return	e.times[S_USER] +
			e.times[S_NICE] +
			e.times[S_SYSTEM] +
			e.times[S_IRQ] +
			e.times[S_SOFTIRQ] +
			e.times[S_STEAL] +
			e.times[S_GUEST] +
			e.times[S_GUEST_NICE];
}

void PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{
	const size_t NUM_ENTRIES = entries1.size();
	
	//Logging 
	if ( loggingEnabled && logFile.is_open())
	{
		logFile << count << ",";
		count++;

		for(size_t i = NUM_ENTRIES-5; i < NUM_ENTRIES; ++i)
		{
			const CPUData & e1 = entries1[i];
			const CPUData & e2 = entries2[i];

			std::cout.width(3);
			std::cout << e1.cpu << "] ";

			const float ACTIVE_TIME	= static_cast<float>(GetActiveTime(e2) - GetActiveTime(e1));
			const float IDLE_TIME	= static_cast<float>(GetIdleTime(e2) - GetIdleTime(e1));
			const float TOTAL_TIME	= ACTIVE_TIME + IDLE_TIME;
 
			std::cout << "active: ";
			std::cout.setf(std::ios::fixed, std::ios::floatfield);
			std::cout.width(6);
			std::cout.precision(2);
			float usage = (float) (100.f * ACTIVE_TIME / TOTAL_TIME);
			
			if (isnan(usage)) usage = 0;
			std::cout << usage << "%";
		
			std::cout << " - idle: ";
			std::cout.setf(std::ios::fixed, std::ios::floatfield);
			std::cout.width(6);
			std::cout.precision(2);
			std::cout << (100.f * IDLE_TIME / TOTAL_TIME) << "%" << std::endl;

			logFile << usage << ",";
		}

		// memory check
		sysinfo(&info);
		memory = (unsigned long)((info.totalram-info.freeram)/(double)(info.totalram)*100);
		cout <<"Memory Load : " << memory << "%" << endl;
		logFile << memory << endl;
	
	}

}
