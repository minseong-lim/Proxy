#pragma once
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include "ServerState.hpp"

using namespace std;

class FTP;
class CommandBuilder;

class FTPProxyServer
{
private:
    static std::unique_ptr<FTPProxyServer>  m_pInstance;
    static std::once_flag                   m_Flag;

    vector<unique_ptr<ServerState>>         m_States;
    unique_ptr<ServerState>                 m_StartState;
    unique_ptr<ServerState>                 m_CloseState;

    size_t                                  m_nStateIndex;
    bool                                    m_bStop;
    future<void>                            m_Future;

    FTPProxyServer();
public:    
    virtual ~FTPProxyServer() = default;   

    bool init(FTP* pFTP, CommandBuilder* pBuilder);

    void run();
    bool wait();
    void stop();

    static FTPProxyServer* getInstance();
};