#pragma once
#include <iostream>
#include <mutex>
#include <memory>

#include "Socket/DataHandler.hpp"
#include "Socket/DataChannel.hpp"
#include "FTPCommon.hpp"

#define END_OF_LINE    "\r\n"
#define BLANK          " "

enum class eCloseCode : char;

class FTP
{
private:
    static std::unique_ptr<FTP> m_pInstance;
    static std::once_flag       m_Flag;
    static std::string          m_strServerBuffer;
    
    int             m_nServerDesc;
    DataChannel     m_DataChannel;
    
    std::string     m_strServerIP;
    uint16_t        m_nServerPort;

    std::string     m_strClientIP;
    int             m_nClientDesc;

    std::string     m_strUser;

    std::mutex      m_mutex_ClientDesc;

    bool readFromServerToBuffer();
    FTP();
public:
    virtual ~FTP() = default;

    bool init(int nClientDesc, std::string strIP, uint16_t nPort);
    static FTP* getInstance();

    std::string getServerIP()   const   { return m_strServerIP;        }
    int  getServerDesc()        const   { return m_nServerDesc;        }
    void setServerDesc(int nServerDesc) { m_nServerDesc = nServerDesc; }    

    bool readFromClient(std::string& strMessage);    
    bool readFromServer(std::string& strMessage);

    bool writeToClient (std::string &&strMessage);
    bool writeToServer (std::string &&strMessage);

    bool transferData();
    bool getClientIP();

    void setDataChannelType(OperationType type) { m_DataChannel.setType(type); }
    
    bool setGetMode() { return m_DataChannel.setGetMode(); }
    bool setPutMode() { return m_DataChannel.setPutMode(); }

    void setUser(std::string strUser) { m_strUser = strUser; }
    std::string getUser() { return m_strUser; }

    bool connectDataChannel();
    bool connectServer();

    void setDataChannelListenSocket(int nSocket)       { m_DataChannel.setListenSocket(nSocket);        }
    void setDataChannelConnectIP(std::string &&strIP)  { m_DataChannel.setConnectIP(std::move(strIP));  }
    void setDataChannelConnectPort(uint16_t nPort)     { m_DataChannel.setConnectPort(nPort);           }
    void setDataChannelMode(OperationType type)        { m_DataChannel.setType(type);                   }
    void closeServer(); 
};