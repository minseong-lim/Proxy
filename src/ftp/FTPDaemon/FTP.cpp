#include "FTP.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

std::unique_ptr<FTP> FTP::m_pInstance = nullptr;
std::once_flag       FTP::m_Flag;
std::string          FTP::m_strServerBuffer;

FTP::FTP()
{
    m_nServerDesc = -1;
    m_nClientDesc = -1;
}

bool FTP::init(int nClientDesc, std::string strIP, uint16_t nPort)
{
    if (nClientDesc < 0)
    {
        return false;
    }

    if(strIP.empty())
    {
        return false;
    }

    m_strServerIP = strIP;
    m_nServerPort = nPort;

    // non-blocking 처리
    int flags = fcntl(nClientDesc, F_GETFL, 0);

    if (flags == -1)
	{
        hsfdlog(D_ERRO, "Error getting socket flags");
        return false;
    }

	if (fcntl(nClientDesc, F_SETFL, flags | O_NONBLOCK) == -1)
	{
        hsfdlog(D_ERRO, "Error setting socket to non-blocking mode");
        return false;
    }

    m_nClientDesc = nClientDesc;    
    return true;
}

bool FTP::readFromClient(std::string& strMessage)
{
    hsfdlog(D_INFO, "readFromClient");

    auto start = std::chrono::steady_clock::now();

    while (true)
    {
        usleep(1000);
        ssize_t nResult = 0;

        {
            std::unique_lock<std::mutex> lock(m_mutex_ClientDesc);
            nResult = DataHandler::ReadCMD(m_nClientDesc, strMessage, 1);
        }

        // success
        if (nResult > 0)
        {
            break;
        }

        // timeout
        if (nResult == -2)
        {
            auto now = std::chrono::steady_clock::now();
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - start);

            if (sec > std::chrono::seconds(TIME_OUT_SEC))
            {            
                return false;
            }
            continue;
        }

        return false;               
    }

    if (strMessage.empty())
    {
        hsfdlog(D_INFO, "Receive from client 0 byte...");
        return false;
    }

    std::size_t nIndex  = strMessage.find_last_of(END_OF_LINE);

    if (std::string::npos == nIndex)
    {
        hsfdlog(D_ERRO, "Can't find end of string...");
        return false;
    }

    std::size_t nLength = strMessage.length() - 1;

    if (nIndex != nLength)
    {
        hsfdlog(D_ERRO, "There is a character at the end of string...");
        return false;
    }

    return true;
}

bool FTP::readFromServerToBuffer()
{
    if(m_strServerBuffer.empty())
    {
        if(DataHandler::ReadCMD(m_nServerDesc, m_strServerBuffer) <= 0)
        {
            hsfdlog(D_ERRO, "Read from server fail...");
            return false;
        }
    }

    if(m_strServerBuffer.empty())
    {
        hsfdlog(D_ERRO, "Receive from server 0 byte...");
        return false;
    }

    return true;
}

bool FTP::readFromServer(std::string& strMessage)
{
    if(!readFromServerToBuffer())
    {
        return false;
    }

    std::size_t nIndex  = m_strServerBuffer.find_first_of(END_OF_LINE);
    
    if (nIndex == std::string::npos)
    {
        hsfdlog(D_ERRO, "Can't find end of string...");
        return false;
    }

    nIndex += strlen(END_OF_LINE);
    strMessage        = m_strServerBuffer.substr(0, nIndex);
    m_strServerBuffer = m_strServerBuffer.substr(nIndex);

    // hsfdlog(D_INFO, "SERVER >> PROXY : %s", strMessage.c_str());
    return true;
}

bool FTP::writeToClient(std::string &&strMessage)
{
    if(!DataHandler::WriteCMD(m_nClientDesc, std::move(strMessage)))
    {
        return false;
    }

    // hsfdlog(D_INFO, "PROXY >> CLIENT : %s", strMessage.c_str());
    return true;
}

bool FTP::writeToServer(std::string &&strMessage)
{
    if (!DataHandler::WriteCMD(m_nServerDesc, std::move(strMessage)))
    {
        hsfdlog(D_INFO, "Write to server failed... %d", errno);
        return false;
    }

    // 24.03.22 LMS [ password 때문에 주석처리.. ]
    // hsfdlog(D_INFO, "PROXY >> SERVER : %s", strMessage.c_str());
    return true;
}

bool FTP::transferData()
{
    if(!m_DataChannel.checkValidation())
    {
        hsfdlog(D_ERRO, "Check validation fail");
        return false;
    }

    auto pReadData = DataHandler::ReadData(m_DataChannel.getReadSocket());
    bool bResult   = DataHandler::WriteData(m_DataChannel.getWriteSocket(), std::move(pReadData));

    m_DataChannel.reset();

    return bResult;
}

bool FTP::connectServer()
{
	m_nServerDesc = socket(AF_INET, SOCK_STREAM, 0);

	if (m_nServerDesc < 0)
    {
        return false;
    }
  	
    struct hostent* pHost = gethostbyname(m_strServerIP.c_str());

    if(!pHost)
    {
        return false;
    }

    struct sockaddr_in stServer;
	stServer.sin_family = AF_INET;
	
	memmove(&stServer.sin_addr, pHost->h_addr, pHost->h_length);
	stServer.sin_port = htons(m_nServerPort);

	if (connect(m_nServerDesc, (struct sockaddr*)&stServer, sizeof(stServer)) < 0)
    {
        close(m_nServerDesc);
        return false;
    }

    hsfdlog(D_INFO, "Server Desc : %d", m_nServerDesc);
    
    // Non block 설정..
    int flags = fcntl(m_nServerDesc, F_GETFL, 0);

    if (flags == -1)
	{
        close(m_nServerDesc);
        return false;
    }
    
	if (fcntl(m_nServerDesc, F_SETFL, flags | O_NONBLOCK) == -1)
	{
        close(m_nServerDesc);
        return false;
    }
    
    hsfdlog(D_INFO, "Connect Server OK...");
    return true;
}

bool FTP::getClientIP()
{
    struct sockaddr_in sockAddr;
    int nLength = sizeof(sockaddr_in);

    if (0 != getpeername(m_nClientDesc, (struct sockaddr*)&sockAddr, (socklen_t*)&nLength))
    {
        return false;
    }

    m_strClientIP = inet_ntoa(sockAddr.sin_addr);
    hsfdlog(D_INFO, "Client IP : %s", m_strClientIP.c_str());

	return true;
}

bool FTP::connectDataChannel()
{
    if(!m_DataChannel.connectDataChannel())
    {
        return false;
    }

    if(OperationType::PASSIVE == m_DataChannel.getType())
    {
        if(m_DataChannel.getListenIP() == m_strClientIP)
        {
            return true;
        }
    }
    else if(OperationType::ACTIVE == m_DataChannel.getType())
    {
        if(m_DataChannel.getListenIP() == m_strServerIP)
        {
            return true;
        }
    }

    m_DataChannel.reset();
    return false;
}

void FTP::closeServer()
{
    if(m_nServerDesc >= 0)
    {
        close(m_nServerDesc);
        m_nServerDesc = -1;
    }

    if(m_nClientDesc >= 0)
    {
        // std::unique_lock<std::mutex> lock(m_mutex_ClientDesc);
        close(m_nClientDesc);
        m_nClientDesc = -1;
    }

    m_DataChannel.reset();
}

FTP* FTP::getInstance()
{
    std::call_once(m_Flag, []
    {
        std::unique_ptr<FTP> ftp(new FTP);
        m_pInstance = std::move(ftp);
    });

    return m_pInstance.get();
}