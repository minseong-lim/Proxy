#include "ServerState.hpp"
#include "Command/CommandBuilder.hpp"
#include "FTP.hpp"

std::shared_ptr<FTPCommandData> ServerState::m_pCommand = nullptr;
std::string ServerState::m_strRequestMessage;

bool StartState::start()
{    
    hsfdlog(D_INFO, "Start");
    m_pCommand = m_pBuilder ? m_pBuilder->getStartCommand() : nullptr;
    return true;
}

bool GetRequestState::getRequest()
{
    hsfdlog(D_INFO, "Get Request");
    return m_pFTP ? m_pFTP->readFromClient(m_strRequestMessage) : false;
}

bool CreateCommandState::createCommand()
{
    hsfdlog(D_INFO, "Create Command");
    m_pCommand = m_pBuilder ? m_pBuilder->doBuild(std::move(m_strRequestMessage)) : nullptr;
    return true;
}

bool ProcessCommandState::processCommand()
{
    hsfdlog(D_INFO, "Process command");
    if(!m_pCommand)
    {
        hsfdlog(D_ERRO, "Command is nullptr");
        return false;
    }

    if(m_pCommand->processCommand())
    {
        m_pCommand->clear();
        return true;
    }

    hsfdlog(D_ERRO, "Process command fail...");
    return false;
}

bool CloseState::closeServer()
{
    hsfdlog(D_INFO, "Close Server...");
    m_pFTP->closeServer();
    return true;
}