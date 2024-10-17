#pragma once
#include <iostream>

#include "DataHandler.hpp"

enum class OperationType
{
    NONE        = 0,
    ACTIVE,
    PASSIVE
};

class DataChannel
{
private:
    std::string     m_strConnectIP;
    uint16_t        m_nConnectPort;
    int             m_nConnectSocket;

    std::string     m_strListenIP;
    int             m_nListenSocket;

    int             m_nReadSocket;
    int             m_nWriteSocket;
    OperationType   m_Type;
    
    bool Connect();
    bool Accept();
public:
    DataChannel();
    virtual ~DataChannel();

    void setType(OperationType type);
    bool setGetMode();
    bool setPutMode();

    bool checkValidation();

    bool connectDataChannel();
    void reset();

    int             getReadSocket()     const { return m_nReadSocket;  }
    int             getWriteSocket()    const { return m_nWriteSocket; }
    OperationType   getType()           const { return m_Type;         }
    std::string     getListenIP()       const { return m_strListenIP;  }

    void setListenSocket(int nSocket)       { m_nListenSocket  = nSocket; }
    void setConnectIP(std::string &&strIP)  { m_strConnectIP   = strIP;   }
    void setConnectPort(uint16_t nPort)     { m_nConnectPort   = nPort;   }
};