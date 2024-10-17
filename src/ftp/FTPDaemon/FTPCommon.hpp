#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <memory>

template <typename T>
class HSLogger;

namespace Global
{
	extern AnalysisSocketControl asc;

	extern bool 		bValidAnalysisServer;
	extern std::string  strResponseMessage;
	extern eCloseCode 	CloseCode;
	extern std::string 	strDenyCommand;

	extern const char* pMainPipePath;
	extern const char* pMainPipeName;

	extern HSCommon::ShmemUtil<Shm> 	shmUtil;
	extern ShmAll 						shmAll;
	extern std::unique_ptr<ShmClient>	pClientShm;
	
	extern int 	nListenerIndex;
	extern int	nHandlerIndex;
	extern int	nClienteIndex;
}

bool AddDataChannelPort(unsigned int nPort);
bool RemoveDataChannelPort();

constexpr const int BUFFER_SIZE     = 16 * 1024;
constexpr const int TIME_OUT_SEC    = 60 * 60;

using DataChannelPacket = struct DataChannelPacket
{
	std::vector<std::unique_ptr<u_char[]>> 	m_Data;
	std::vector<size_t> 					m_Size;

	DataChannelPacket() = default;
};