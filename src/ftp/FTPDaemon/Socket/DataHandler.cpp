#include "DataHandler.hpp"
#include "hsfCommon/DefineLog.hpp"

#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>

ssize_t DataHandler::ReadCMD(int nFileDesc, std::string& strMessage, long nTimeout)
{
    if (nFileDesc < 0)
    {
        hsfdlog(D_ERRO, "Invalid Socket...");
        return -1;
    }

    strMessage.clear();

    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(nFileDesc, &fdset);

    // hsfdlog(D_INFO, "Socket Read... Filedesc : %d", nFileDesc);

    struct timeval tv;
    tv.tv_sec  = nTimeout;
    tv.tv_usec = 0;

    int nResult = select(nFileDesc + 1, &fdset, nullptr, nullptr, &tv);

    // hsfdlog(D_INFO, "Select OK... %d", nResult);

    if (nResult < 0)
    {
        hsfdlog(D_ERRO, "Select error : %s", strerror(errno));
        return -1;
    }

    if (nResult == 0)
    {
        // hsfdlog(D_ERRO, "Select timed out");
        return -2;
    }

    char szBuffer[BUFFER_SIZE];
    ssize_t nTotal = 0;

    while (true)
    {
        memset(szBuffer, 0x00, sizeof(szBuffer));

        ssize_t nRead = read(nFileDesc, szBuffer, sizeof(szBuffer) - 1);
                
        if (nRead <= 0)        
        {
            break;
        }

        strMessage += szBuffer;
        nTotal += nRead;
    }

    hsfdlog(D_INFO, "Total : %ld", nTotal);
    return nTotal;
}

bool DataHandler::WriteCMD(int nFileDesc, std::string &&strMessage)
{
    return write(nFileDesc, strMessage.c_str(), strMessage.length()) == (ssize_t)strMessage.length();
}   

std::unique_ptr<DataChannelPacket> DataHandler::ReadData(int nFileDesc, long nTimeout)
{    
    fd_set fdset;

    FD_ZERO(&fdset);
    FD_SET(nFileDesc, &fdset);

    if (!FD_ISSET(nFileDesc, &fdset))
    {
        return nullptr;
    }
    
    // hsfdlog(D_INFO, "Socket Read... Filedesc : %d", nFileDesc);

    struct timeval tv;
    tv.tv_sec  = nTimeout;
    tv.tv_usec = 0;

    int nResult = select(nFileDesc + 1, &fdset, nullptr, nullptr, &tv);

    // hsfdlog(D_INFO, "Select OK");

    if (nResult < 0)
    {
        hsfdlog(D_ERRO, "Select error : %s", strerror(errno));
        return nullptr;
    }
    else if (nResult == 0)
    {
        hsfdlog(D_ERRO, "Select timed out");
        return nullptr;
    }

    auto pPacket = std::make_unique<DataChannelPacket>();
    
    while (true)
    {
        auto pMessage = std::make_unique<u_char[]>(BUFFER_SIZE);
        memset(pMessage.get(), 0x00, BUFFER_SIZE);
        
        ssize_t nReadBytes = read(nFileDesc, pMessage.get(), BUFFER_SIZE);
    
        if (nReadBytes <= 0)
        {
            break;
        }

        pPacket->m_Data.emplace_back(std::move(pMessage));
        pPacket->m_Size.emplace_back(nReadBytes);
    }
    
    return pPacket;
}

bool DataHandler::WriteData(int nFileDesc, std::unique_ptr<DataChannelPacket> pPacket)
{
    if (!pPacket)
    {
        hsfdlog(D_ERRO, "Packet data is nullptr...");
        return false;
    }

    if (pPacket->m_Data.size() != pPacket->m_Size.size())
    {
        hsfdlog(D_ERRO, "Invalid packet... It is diffrent size...");
        return false;
    }
    
    for (size_t ni = 0 ; ni < pPacket->m_Data.size() ; ni++)
    {
        auto pData = std::move(pPacket->m_Data[ni]);
        auto nSize = pPacket->m_Size[ni];

        if (!pData)
        {
            hsfdlog(D_ERRO, "Invalid packet... Data is nullptr...");
            return false;
        }

        auto nWriteBytes = write(nFileDesc, pData.get(), nSize);

        if (nWriteBytes != (ssize_t)nSize)
        {
            hsfdlog(D_ERRO, "Write data failed...");
            return false;
        }
    }

    return true;
}