#pragma once
#include <iostream>

#include "Command/FTPCommand.hpp"
#include "Command/DefineMessage.hpp"
#include "Command/DefineResponseCode.hpp"
#include "FTPCommon.hpp"

#define FTP_NULL_CHECK  if(!pData || !pData->getFTP()) return false;

class FTPStrategy
{
public:
    FTPStrategy()          = default;
    virtual ~FTPStrategy() = default;
    
    virtual bool execute(FTPCommandData* pData) = 0;
};

class SendRequest : public FTPStrategy
{
public:
    virtual ~SendRequest() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        return pData->writeToServer();
    }
};

class GetResponse : public FTPStrategy
{
public:
    virtual ~GetResponse() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        return pData->readFromServer();
    }
};

class GetFEATResponse : public FTPStrategy
{
public:
    virtual ~GetFEATResponse() = default;
    bool execute(FTPCommandData* pData);
};

class SendResponse : public FTPStrategy
{
public:
    virtual ~SendResponse() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK

        int nCode = !pData->getResponseCode().empty() ? std::stoi(pData->getResponseCode()) : 0;

        // than 500 -> Error...
        if (nCode >= 500)
        {
            Global::strResponseMessage = pData->getResponseLine();
        }

        return pData->writeToClient();
    }
};

class ConnectDataChannel : public FTPStrategy
{
public:
    virtual ~ConnectDataChannel() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->connectDataChannel();
    }
};

class CheckDataChannelConnection : public FTPStrategy
{
public:
    virtual ~CheckDataChannelConnection() = default;
    bool execute(FTPCommandData* pData);
};

class DataTransfer : public FTPStrategy
{
public:
    virtual ~DataTransfer() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->transferData();
    }
};

class GetMode : public FTPStrategy
{
public:
    virtual ~GetMode() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->setGetMode();
    }
};

class PutMode : public FTPStrategy
{
public:
    virtual ~PutMode() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->setPutMode();
    }
};

class PassiveResponse : public FTPStrategy
{
public:
    virtual ~PassiveResponse() = default;
    bool execute(FTPCommandData* pData);
};

class PortRequest : public FTPStrategy
{
public:
    virtual ~PortRequest() = default;
    bool execute(FTPCommandData* pData);
};

class CheckPortResponse : public FTPStrategy
{
public:
    virtual ~CheckPortResponse() = default;
    bool execute(FTPCommandData* pData);
};

class SetPassiveMode : public FTPStrategy
{
public:
    virtual ~SetPassiveMode() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        pData->getFTP()->setDataChannelType(OperationType::PASSIVE);
        return true;
    }
};

class SetActiveMode : public FTPStrategy
{
public:
    virtual ~SetActiveMode() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        pData->getFTP()->setDataChannelType(OperationType::ACTIVE);
        return true;
    }
};

class GetClientIP : public FTPStrategy
{
public:
    virtual ~GetClientIP() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->getClientIP();
    }
};

class ConnectToServer : public FTPStrategy
{
public:
    virtual ~ConnectToServer() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
	    return pData->getFTP()->connectServer();
    }
};

class CheckServer : public FTPStrategy
{
public:
    virtual ~CheckServer() = default;
    bool execute(FTPCommandData* pData);
};

class ServerReady : public FTPStrategy
{
public:
    virtual ~ServerReady() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        pData->setResponseCode(SERVER_READY_CODE);
        pData->setResponseMessage(SERVER_READY_MESSAGE);
        return pData->writeToClient();
    }
};

class AuthRequest : public FTPStrategy
{
public:
    virtual ~AuthRequest() = default;
    bool execute(FTPCommandData* pData);
};

class Error : public FTPStrategy
{
public:
    virtual ~Error() = default;
    bool execute(FTPCommandData* pData);
};

class RequestToAnalysis : public FTPStrategy
{
public:
    virtual ~RequestToAnalysis() = default;
    bool execute(FTPCommandData* pData);
};

class ResultFromAnalysis : public FTPStrategy
{
public:
    virtual ~ResultFromAnalysis() = default;
    bool execute(FTPCommandData* pData);
};

class ResponseToAnalysis : public FTPStrategy
{
public:
    virtual ~ResponseToAnalysis() = default;
    bool execute(FTPCommandData* pData);
};

class SetUser : public FTPStrategy
{
public:
    virtual ~SetUser() = default;
    bool execute(FTPCommandData* pData)
    {
        FTP_NULL_CHECK
        pData->getFTP()->setUser(pData->getParameter());
        return true;
    }
};