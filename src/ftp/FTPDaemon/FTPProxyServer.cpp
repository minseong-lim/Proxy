#include "FTPProxyServer.hpp"
#include "ServerState.hpp"
#include "Command/CommandBuilder.hpp"
#include "FTP.hpp"

#include <functional>
#include <chrono>

std::unique_ptr<FTPProxyServer> FTPProxyServer::m_pInstance = nullptr;
std::once_flag                  FTPProxyServer::m_Flag;

FTPProxyServer::FTPProxyServer()
{    
    m_StartState    = nullptr;
    m_CloseState    = nullptr;
    m_nStateIndex   = 0;
    m_bStop         = false;

    m_States.clear();
}

///////////////////////////////////////////////////////////////////////////

bool FTPProxyServer::init(FTP* pFTP, CommandBuilder* pBuilder)
{
    if (!pFTP || !pBuilder)
    {
        return false;
    }

    m_nStateIndex = 0;
    m_StartState  = make_unique<StartState>(pBuilder);
    m_CloseState  = make_unique<CloseState>(pFTP);

    m_States.clear();
    m_States.emplace_back(make_unique<ProcessCommandState>());
    m_States.emplace_back(make_unique<GetRequestState>(pFTP));
    m_States.emplace_back(make_unique<CreateCommandState>(pBuilder));

    return true;
}

void FTPProxyServer::run()
{
    m_Future = async(launch::async, [this]
    {        
        if(!m_StartState->handle())
        {
            return;
        }

        while (!m_bStop)
        {
            if(!m_States[m_nStateIndex]->handle())
            {
                break;
            }

            m_nStateIndex = (m_nStateIndex + 1) % m_States.size();            
        }

        m_CloseState->handle();
    });    
}

// if running end -> return true else false
bool FTPProxyServer::wait()
{
    if (m_Future.valid() && (m_Future.wait_for(std::chrono::seconds(1)) == std::future_status::timeout))
    {
        return false;
    }

    return true;
}

void FTPProxyServer::stop()
{
    m_bStop = true;

    if (m_Future.valid())
    {
        m_Future.get();
    }
}

FTPProxyServer* FTPProxyServer::getInstance()
{
    std::call_once(m_Flag, []
    {
        std::unique_ptr<FTPProxyServer> server(new FTPProxyServer);
        m_pInstance = std::move(server);
    });

    return m_pInstance.get();
}