#include "FTPStrategy.hpp"
#include "FTP.hpp"
#include "FTPCommon.hpp"
#include "Command/DefineCommand.hpp"

#include <regex>
#include <functional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <chrono>
#include <thread>

auto getIPPort = [](const std::string& strMessage, std::string& strIP, uint16_t& nPort)
{
    std::regex regex("(\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)");
    std::smatch match;
    std::string port;

    if (regex_search(strMessage, match, regex) && match.size() == 7)
	{
		for(int ni = 1 ; ni <= 4 ; ni++)
		{
			strIP += (match[ni].str() + ".");
		}

		strIP.erase(strIP.length() - 1);

        int highByte 	= stoi(match[5]);
        int lowByte 	= stoi(match[6]);
        int portNumber 	= (highByte << 8) | lowByte;
				
		if (portNumber > std::numeric_limits<uint16_t>::max())
		{
			// throw std::out_of_range("Port is out of range for unsigned short");
			return false;
		}

		nPort = static_cast<uint16_t>(portNumber);

		return true;
    }

	return false;
};

auto replaceIPPort = [](std::string& strMessage, std::string strIP, unsigned int nPort)
{
	std::regex regex("(\\d+),(\\d+),(\\d+),(\\d+),(\\d+),(\\d+)");
    std::smatch match;

	if (regex_search(strMessage, match, regex))
	{
		if (!RemoveDataChannelPort() || !AddDataChannelPort(nPort))
		{
			return false;
		}

		for(size_t ni = 0 ; ni < strIP.length() ; ni++)
		{
			if(strIP.at(ni) == '.')
			{
				strIP.replace(ni, 1, ",");
			}
		}

        std::string strReplaceMessage = match.prefix().str();
		strReplaceMessage += (strIP + ",");
		strReplaceMessage += (std::to_string(nPort >> 8) + ",");
		strReplaceMessage += (std::to_string(nPort & 0xFF) + match.suffix().str());

		strMessage = strReplaceMessage;
		return true;
    }

	hsfdlog(D_ERRO, "Replace IP & Port fail...");
	return false;
};

auto check = [](std::string strCode, std::string strSuccessCodes)
{
    std::istringstream iss(strSuccessCodes);
    std::string token;

    while(getline(iss, token, ','))
    {
        if(token == strCode)
        {
            return true;
        }
    }

    return false;
};

auto getProxyIP = []
{
	std::string strIP;

    struct ifaddrs* pAddr = nullptr;
    getifaddrs(&pAddr);

	while(true)
	{
		if(!pAddr->ifa_addr)
		{
			pAddr = pAddr->ifa_next;
			continue;
		}

		if ( (pAddr->ifa_addr->sa_family == AF_INET) && (strcmp(pAddr->ifa_name, "lo") != 0) )
		{
			char szBuffer[INET_ADDRSTRLEN];
			memset(szBuffer, 0x00, sizeof(szBuffer));

			strIP = inet_ntoa(((struct sockaddr_in*)pAddr->ifa_addr)->sin_addr);
			break;
		}

		pAddr = pAddr->ifa_next;
	}

    if (!pAddr)
	{
        freeifaddrs(pAddr);
    }

    return strIP;
};

auto bindPort = [](const std::string &strIP, int& nSocket)
{
	nSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (nSocket < 0)
	{
		hsfdlog(D_ERRO, "-ERR: can't create socket: %s", strerror(errno));		
		return false;
	}
	else
	{
		int	opt = 1;
		setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	}

	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port   = htons(0);
	//saddr.sin_addr.s_addr	 = htonl(INADDR_ANY);

	if (strIP.empty())
	{
		hsfdlog(D_ERRO, "IP Address is empty");		
		return false;
	}

	struct hostent *ifp = gethostbyname(strIP.c_str());
	if (!ifp)
	{
		hsfdlog(D_ERRO, "-ERR: can't lookup %s", strIP.c_str());		
		return false;
	}

	memcpy(&saddr.sin_addr, ifp->h_addr, ifp->h_length);

	if (bind(nSocket, (struct sockaddr *)&saddr, sizeof(saddr)))
	{
		hsfdlog(D_ERRO, "-ERR: can't bind to %s", strIP.c_str());			
		return false;
	}

	if (listen(nSocket, 5) < 0)
	{
		hsfdlog(D_ERRO, "-ERR: listen error: %s", strerror(errno));
		return false;
	}

	return true;
};

auto getPortInfo = [](int nDesc, unsigned int& nPort)
{
	struct sockaddr_in saddr;
    socklen_t size = sizeof(saddr);

    if (getsockname(nDesc, (struct sockaddr*)&saddr, &size) < 0)
	{
		hsfdlog(D_ERRO, "-ERR: can't get interface info: %s", strerror(errno));
		return false;
    }

    nPort = ntohs(saddr.sin_port);
	return true;
};

bool PassiveResponse::execute(FTPCommandData* pData)
{
	FTP_NULL_CHECK

	if(!check(pData->getResponseCode(), PASV_SUCCESS_CODE))
	{
		return false;
	}

	std::string strIP;
	uint16_t nPort = 0;

	if(!getIPPort(pData->getResponseMessage(), strIP, nPort))
	{
		hsfdlog(D_ERRO, "Fail to get IP & Port from PASV response...");
		return false;
	}

	hsfdlog(D_INFO, "Server listens on %s:%u", strIP.c_str(), nPort);

	pData->getFTP()->setDataChannelConnectIP(std::move(strIP));
	pData->getFTP()->setDataChannelConnectPort(nPort);

    std::string strProxyIP = getProxyIP();
    if(strProxyIP.empty())
    {
        return false;
    }
	
	int nListenSocket = 0;
	
	if(!bindPort(strProxyIP, nListenSocket))
	{
		return false;
	}

	pData->getFTP()->setDataChannelListenSocket(nListenSocket);

    unsigned int nProxyPort = 0;
	
	if(!getPortInfo(nListenSocket, nProxyPort))
	{
		return false;
	}

	hsfdlog(D_INFO, "Listening on %s:%u", strProxyIP.c_str(), nProxyPort);
	
	std::string strReplaceMsg = pData->getResponseMessage();

	if(!replaceIPPort(strReplaceMsg, strProxyIP, nProxyPort))
	{
		return false;
	}

	pData->setResponseMessage(strReplaceMsg);
    
	return true;
}

bool PortRequest::execute(FTPCommandData* pData)
{
	FTP_NULL_CHECK
	std::string strIP;
	uint16_t nPort = 0;

	if(!getIPPort(pData->getParameter(), strIP, nPort))
	{
		hsfdlog(D_ERRO, "Fail to get IP & Port from PORT request...");
		return false;
	}

	hsfdlog(D_INFO, "Client listens on %s:%u", strIP.c_str(), nPort);

	pData->getFTP()->setDataChannelConnectIP(std::move(strIP));
	pData->getFTP()->setDataChannelConnectPort(nPort);

	std::string strProxyIP = getProxyIP();
    if(strProxyIP.empty())
    {		
		hsfdlog(D_ERRO, "Get proxy ip address fail...");
		return false;
    }

	int nListenSocket = 0;

	if(!bindPort(strProxyIP, nListenSocket))
	{
		return false;
	}

	pData->getFTP()->setDataChannelListenSocket(nListenSocket);

	unsigned int nProxyPort = 0;
	
	if(!getPortInfo(nListenSocket, nProxyPort))
	{
		return false;
	}

	hsfdlog(D_INFO, "Listening on %s:%u", strProxyIP.c_str(), nProxyPort);

	std::string strReplaceMsg = pData->getParameter();

	hsfdlog(D_INFO, "Replaced message : %s", strReplaceMsg.c_str());

	if(!replaceIPPort(strReplaceMsg, strProxyIP, nProxyPort))
	{
		return false;
	}

	pData->setParameter(strReplaceMsg);
    
	return true;
}

bool AuthRequest::execute(FTPCommandData *pData)
{
	FTP_NULL_CHECK
	
	std::shared_ptr<AnalysisPacket> pPacket(new AnalysisPacket);
	pPacket->setFTPResponsePacket(std::string(), AUTH_RESPONSE_CODE, AUTH_RESPONSE_MESSAGE);

	auto tuple_send_analysis_result = Global::asc.SendToAnalysis(pPacket);

	if (!std::get<0>(tuple_send_analysis_result))
	{
		hsfdlog(D_ERRO, "%s", std::get<1>(tuple_send_analysis_result).c_str());

		Global::asc.Close();
		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			Global::bValidAnalysisServer = false;
			pData->setResponseCode(ANALYSIS_SERVER_ERROR_CODE);
			pData->setResponseMessage(ANALYSIS_SERVER_ERROR_MESSAGE);
			return false;
		}
	}	

	pData->setResponseCode(AUTH_RESPONSE_CODE);
	pData->setResponseMessage(AUTH_RESPONSE_MESSAGE);
	return pData->writeToClient();
}

bool Error::execute(FTPCommandData* pData)
{
	FTP_NULL_CHECK
	std::string strCode = pData->getResponseCode();

	if(strCode.empty() || stoi(strCode) < 500)	// Upper 500 -> Error code...
	{
		pData->setResponseCode(ERROR_CODE);
		pData->setResponseMessage(ERROR_MESSAGE);
	}
	
	Global::strResponseMessage = pData->getResponseLine();

	return pData->writeToClient();
}

constexpr const char * ALTERNATIVE_PASSWORD_CHARACTER = "********";

bool RequestToAnalysis::execute(FTPCommandData *pData)
{
	FTP_NULL_CHECK

	std::string strUser 		= pData->getFTP()->getUser();
	std::string strCommand 		= pData->getCommand();
	std::string strParameter 	= (PASS_COMMAND == strCommand) ? ALTERNATIVE_PASSWORD_CHARACTER : pData->getParameter();
	std::string strDescription 	= pData->getDescription();

	std::shared_ptr<AnalysisPacket> pPacket(new AnalysisPacket);
	pPacket->setFTPRequestPacket(strUser, strCommand, strParameter, strDescription);

	auto tuple_send_analysis_result = Global::asc.SendToAnalysis(pPacket);

	if (!std::get<0>(tuple_send_analysis_result))
	{
		hsfdlog(D_ERRO, "Request packet error to analysis server..!!");
		hsfdlog(D_ERRO, "%s", std::get<1>(tuple_send_analysis_result).c_str());

		Global::asc.Close();
		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			Global::bValidAnalysisServer = false;
			pData->setResponseCode(ANALYSIS_SERVER_ERROR_CODE);
			pData->setResponseMessage(ANALYSIS_SERVER_ERROR_MESSAGE);
			return false;
		}
	}

	return true;
}

bool ResultFromAnalysis::execute(FTPCommandData *pData)
{
	FTP_NULL_CHECK

	auto tuple_recieve_result = Global::asc.RecieveFromAnalysis();
	auto pPacket = std::get<0>(tuple_recieve_result);

	if (!pPacket)
	{
		hsfdlog(D_ERRO, "Recieve packet error from analysis server..!!");
		hsfdlog(D_ERRO, "%s", std::get<1>(tuple_recieve_result).c_str());

		Global::asc.Close();
		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			Global::bValidAnalysisServer = false;
			pData->setResponseCode(ANALYSIS_SERVER_ERROR_CODE);
			pData->setResponseMessage(ANALYSIS_SERVER_ERROR_MESSAGE);
			return false;
		}

		return true;
	}

	eReturnCode eResult = pPacket->getResult();

	if (eReturnCode::eNormal == eResult)
	{
		return true;
	}

	// FTP는 무시가 없지만 일단 처리...
	if (eReturnCode::eIgnore == eResult)
	{
		hsfdlog(D_INFO, "Ignore Command..!!");		
		pData->setResponseCode(PERMISSION_DENY_CODE);
		pData->setResponseMessage(PERMISSION_DENY_MESSAGE);
		return false;
	}
	
	if (eReturnCode::eError == eResult)
	{
		hsfdlog(D_INFO, "Error Command..!!");
		Global::CloseCode = eCloseCode::eAbNormal;
		pData->setResponseCode(ERROR_CODE);
		pData->setResponseMessage(ERROR_MESSAGE);
	}
	else
	{
		hsfdlog(D_INFO, "Deny Command..!!");
		Global::CloseCode = eCloseCode::ePolicyDeny;

		pData->setResponseCode(PERMISSION_DENY_CODE);

		switch (eResult)
		{
		case eReturnCode::eProtocolDeny:
			pData->setResponseMessage(PROTOCOL_DENY_MESSAGE);
			break;
		case eReturnCode::eWeekDeny:
			pData->setResponseMessage(WEEK_DENY_MESSAGE);
			break;
		case eReturnCode::eTimeDeny:
			pData->setResponseMessage(TIME_DENY_MESSAGE);
			break;
		case eReturnCode::eNoPolicy:
			pData->setResponseMessage(NO_POLICY_MESSAGE);
			break;
		case eReturnCode::eCmdDeny:
			pData->setResponseMessage(COMMAND_DENY_MESSAGE);
			Global::strDenyCommand = pData->getCommand();
			break;
		default:
			pData->setResponseMessage(DENY_MESSAGE);
			break;
		}		
	}

	return false;
}

bool ResponseToAnalysis::execute(FTPCommandData *pData)
{
	FTP_NULL_CHECK
	std::string strUser 	= pData->getFTP()->getUser();
	std::string strCode 	= pData->getResponseCode();
	std::string strMessage 	= pData->getResponseMessage();

	std::shared_ptr<AnalysisPacket> pPacket(new AnalysisPacket);
	pPacket->setFTPResponsePacket(strUser, strCode, strMessage);

	auto tuple_send_analysis_result = Global::asc.SendToAnalysis(pPacket);

	if (!std::get<0>(tuple_send_analysis_result))
	{
		hsfdlog(D_ERRO, "%s", std::get<1>(tuple_send_analysis_result).c_str());

		Global::CloseCode = eCloseCode::eAbNormal;
		Global::asc.Close();
		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			pData->setResponseCode(ANALYSIS_SERVER_ERROR_CODE);
			pData->setResponseMessage(ANALYSIS_SERVER_ERROR_MESSAGE);
			return false;
		}
	}

	return true;
}

auto checkFEATResponse = [](const std::string& strResponse)
{
	if (strResponse.empty())
	{
        return false;
    }

    int count  = 0;
    size_t pos = 0;

    while ((pos = strResponse.find(FEAT_RESPONSE_CODE, pos)) != std::string::npos)
	{
        pos += std::string(FEAT_RESPONSE_CODE).length();
        count++;
    }

    return count > 1;
};

constexpr const int WAIT_FEAT_RESPONSE_TIME = 10;

bool GetFEATResponse::execute(FTPCommandData *pData)
{
	FTP_NULL_CHECK

	std::string strMessage;
	
	auto startTime = std::chrono::steady_clock::now();

	while (true)
	{
		std::string str;

		if (!pData->readFromServer(str))
		{
			return false;
		}

		strMessage += str;

		if (checkFEATResponse(strMessage))
		{
			break;
		}

		auto currentTime = std::chrono::steady_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

		if (elapsedTime >= WAIT_FEAT_RESPONSE_TIME)
		{
			return false;
		}		

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return pData->parseResponse(std::move(strMessage));
}

bool CheckDataChannelConnection::execute(FTPCommandData* pData)
{
	FTP_NULL_CHECK
    return check(pData->getResponseCode(), DATA_CHANNEL_OPEN_SUCCESS_CODE);
}

bool CheckPortResponse::execute(FTPCommandData* pData)
{
    FTP_NULL_CHECK
    return check(pData->getResponseCode(), PORT_SUCCESS_CODE);
}

bool CheckServer::execute(FTPCommandData* pData)
{
    FTP_NULL_CHECK
	return check(pData->getResponseCode(), NEW_USER_READY_CODE);
}