#pragma once
#include <memory>

#include "Command/FTPCommand.hpp"

class CommandBuilder;
class FTP;

class ServerState
{
protected:    
    static std::shared_ptr<FTPCommandData> m_pCommand; 
    static std::string m_strRequestMessage;
public:
    ServerState()          = default;
    virtual ~ServerState() = default;
    virtual bool handle()  = 0;
};

class GetRequestState : public ServerState
{
private:
    FTP*  m_pFTP;
    bool getRequest();
public:
    GetRequestState(FTP* pFTP)
    {
        m_pFTP = pFTP;
    }
    virtual ~GetRequestState() = default;

    bool handle() { return getRequest(); }
};

class StartState : public ServerState
{
private:    
    CommandBuilder* m_pBuilder;
    bool start();
public:
    StartState(CommandBuilder* pBuilder)
    {
        m_pBuilder = pBuilder;
    }
    virtual ~StartState() = default;

    bool handle() { return start(); }
};

class CreateCommandState : public ServerState
{
private:    
    CommandBuilder* m_pBuilder;
    bool createCommand();
public:
    CreateCommandState(CommandBuilder* pBuilder)
    {
        m_pBuilder = pBuilder;
    }
    virtual ~CreateCommandState() = default;

    bool handle() { return createCommand(); }
};

class ProcessCommandState : public ServerState
{
private:
    bool processCommand();
public:
    ProcessCommandState()          = default;
    virtual ~ProcessCommandState() = default;

    bool handle() { return processCommand(); }
};

class CloseState : public ServerState
{
private:
    FTP*  m_pFTP;
    bool closeServer();
public:
    CloseState(FTP* pFTP)
    {
        m_pFTP = pFTP;
    }
    virtual ~CloseState() = default;
    bool handle() { return closeServer(); }
};