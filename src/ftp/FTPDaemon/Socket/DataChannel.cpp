#include "DataChannel.hpp"
#include "hsfCommon/DefineLog.hpp"
#include "FTPCommon.hpp"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

DataChannel::DataChannel()
{
    m_nConnectPort 	    =  0;
    m_nConnectSocket 	= -1;
    m_nListenSocket	    = -1;
    m_nReadSocket		= -1;
    m_nWriteSocket		= -1;
    m_Type				= OperationType::NONE;
}

DataChannel::~DataChannel()
{
    reset();
}

void DataChannel::setType(OperationType type)
{
	m_Type = type;
}

void DataChannel::reset()
{
	if (m_nReadSocket >= 0)
	{
		close(m_nReadSocket);
	}

	if (m_nWriteSocket >= 0)
	{
		close(m_nWriteSocket);
	}

	m_strConnectIP.clear();
    m_strListenIP.clear();

	m_nConnectPort 		=  0;
	m_nConnectSocket 	= -1;
	m_nListenSocket 	= -1;
	m_nReadSocket 		= -1;
	m_nWriteSocket 		= -1;
	m_Type				= OperationType::NONE;
}

bool DataChannel::Connect()
{
	m_nConnectSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (m_nConnectSocket < 0)
    {
        reset();
        return false;
    }

    struct sockaddr_in stAddr;
    stAddr.sin_family = AF_INET;

    struct hostent *hostp = gethostbyname(m_strConnectIP.c_str());

    if (!hostp)
    {
        reset();
        return false;
    }

    memcpy(&stAddr.sin_addr, hostp->h_addr, hostp->h_length);
    stAddr.sin_port = htons(m_nConnectPort);

    if (connect(m_nConnectSocket, (struct sockaddr *)&stAddr, sizeof(stAddr)) < 0)
    {
        reset();
        return false;
    }

    return true;
}

bool DataChannel::Accept()
{    
    hsfdlog(D_INFO, "Accept");
	fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(m_nListenSocket, &fdset);

	struct timeval tv;
	tv.tv_sec  = TIME_OUT_SEC;
	tv.tv_usec = 0;

    hsfdlog(D_INFO, "ListenSocket : %d", m_nListenSocket);

    int nResult = select(m_nListenSocket + 1, &fdset, nullptr, nullptr, &tv);

    hsfdlog(D_INFO, "select OK");

    if (nResult <= 0)
    {
        hsfdlog(D_ERRO, "select() error: %s", strerror(errno));        
        reset();
        return false;
    }

    if (FD_ISSET(m_nListenSocket, &fdset))
    {
        struct sockaddr_in stAddr;
        socklen_t nAddrLength = sizeof(sockaddr_in);

        int nSocket = accept(m_nListenSocket, (struct sockaddr *)&stAddr, &nAddrLength);

        if (nSocket < 0)
        {
            reset();
            return false;
        }

        hsfdlog(D_INFO, "Accept on socket");

        m_strListenIP = inet_ntoa(stAddr.sin_addr);

        hsfdlog(D_INFO, "Connection from %s", m_strListenIP.c_str());

        dup2(nSocket, m_nListenSocket);
        close(nSocket);

        return true;
    }

    reset();
    return false;
}

bool DataChannel::connectDataChannel()
{
    return Accept() && Connect();
}

bool DataChannel::checkValidation()
{
	if(OperationType::NONE == m_Type)
	{
		return false;
	}

    if(0 > m_nReadSocket)
    {
        return false;
    }

    if(0 > m_nWriteSocket)
    {
        return false;
    }

	return true;
}

bool DataChannel::setGetMode()
{
	if(m_nListenSocket < 0 || m_nConnectSocket < 0)
	{
		reset();
		return false;
	}

	m_nReadSocket  = m_nConnectSocket;
	m_nWriteSocket = m_nListenSocket;

	if(m_Type == OperationType::ACTIVE)
	{
		std::swap(m_nReadSocket, m_nWriteSocket);
	}

    hsfdlog(D_INFO, "Set get mode... ReadSocket : [%d] WriteSocket : [%d]", m_nReadSocket, m_nWriteSocket);    
	return true;
}

bool DataChannel::setPutMode()
{
	if(m_nListenSocket < 0 || m_nConnectSocket < 0)
	{
		reset();
		return false;
	}

	m_nReadSocket  = m_nListenSocket;
	m_nWriteSocket = m_nConnectSocket;

	if(m_Type == OperationType::ACTIVE)
	{
		std::swap(m_nReadSocket, m_nWriteSocket);
	}
	
    hsfdlog(D_INFO, "Set put mode... ReadSocket : [%d] WriteSocket : [%d]", m_nReadSocket, m_nWriteSocket);
	return true;
}