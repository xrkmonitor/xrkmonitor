
#ifndef PID_GUARD_H
#define PID_GUARD_H

#include <fstream>
#include <sstream>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>

class CPidGuard
{
public:
	bool IsProcessAlive(const char* pszProcessName) 
	{ 
		assert(pszProcessName);

		std::stringstream oss;
		oss << basename(const_cast<char*>(pszProcessName));
		std::string sThisPidFile = "/var/tmp/";
		sThisPidFile += oss.str();
		sThisPidFile += ".pid";

		std::ifstream ifStream;
		ifStream.open(sThisPidFile.c_str());
		if(!ifStream.good())//读不成功,说明无此文件(即当前无进程运行）
		{
			WriteThisPid(sThisPidFile.c_str());//将当前pid写入文件，表示已有在运行
			return false;
		}

		pid_t iPid = 0;
		ifStream >> iPid;
		ifStream.close();

		oss.str("");
		oss << iPid;
		std::string sProcPidFile = "/proc/";
		sProcPidFile += oss.str();
		sProcPidFile += "/status";

		ifStream.open(sProcPidFile.c_str());
		if(!ifStream.good())//读不成功，即当前无文件中记录的pid，说明旧进程已退出（存在问题：其他程序占用了旧的pid就会发生判断错误）
		{
			WriteThisPid(sThisPidFile.c_str());//写入文件，用当前pid替代旧的
			return false;
		}
		ifStream.close();

		return true; 
	} 

private:
	void WriteThisPid(const char* pszPidFileName)
	{
		std::ofstream OfStream;
		OfStream.open(pszPidFileName);
		assert(OfStream.good());

		OfStream << getpid();
		OfStream.close();
	}
};

#endif /* PID_GUARD_H */

