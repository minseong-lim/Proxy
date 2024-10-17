#pragma once
#include <string>
#include <limits>
#include <stdexcept>

struct stArgument
{
	int 			m_nShmKey;
	int 			m_nListenerIndex;
	int 			m_nHandlerIndex;
	int 			m_nClientIndex;
	int 			m_nClientSocket;

	std::string 	m_strTargetIP;
	uint16_t        m_nTargetPort;

	stArgument(int nArgc, char** pArgv)
	{
		setShareMemoryKey(pArgv[1]);
		setListenerIndex(pArgv[2]);
		setHandlerIndex(pArgv[3]);
		setClientIndex(pArgv[4]);
		setClientSocket(pArgv[5]);
		setTargetIP(pArgv[6]);
		setTargetPort(pArgv[7]);
	}
	
	void setShareMemoryKey(std::string strShmKey)
	{
		m_nShmKey = std::stoi(strShmKey);
	}

	void setListenerIndex(std::string strListenerIndex)
	{
		m_nListenerIndex = std::stoi(strListenerIndex);
	}

	void setHandlerIndex(std::string strHandlerIndex)
	{
		m_nHandlerIndex = std::stoi(strHandlerIndex);
	}

	void setClientIndex(std::string strClientIndex)
	{
		m_nClientIndex = std::stoi(strClientIndex);
	}

	void setClientSocket(std::string strClientSocket)
	{
		m_nClientSocket = std::stoi(strClientSocket);
	}

	void setTargetIP(std::string strIP)
	{
		m_strTargetIP = strIP;
	}

	void setTargetPort(std::string strPort)
	{
		unsigned long nValue = std::stoul(strPort);

		if (nValue > std::numeric_limits<uint16_t>::max())
		{
			throw std::out_of_range("Port is out of range for unsigned short");			
		}

		m_nTargetPort = static_cast<uint16_t>(nValue);
	}
};

bool init(struct stArgument args);
void run();
void wait();
void close();