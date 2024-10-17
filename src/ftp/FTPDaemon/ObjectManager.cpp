#include "ObjectManager.hpp"
#include "FTP.hpp"
#include "Command/CommandBuilder.hpp"
#include "FTPProxyServer.hpp"
#include "FTPCommon.hpp"

constexpr const char* PROTOCOL = "FTP";

namespace Global
{
	FTP*             			pFTP     	= FTP::getInstance();
	CommandBuilder*  			pBuilder 	= CommandBuilder::getInstance();
	FTPProxyServer*  			pServer  	= FTPProxyServer::getInstance();
}

bool initAnalysisServer(const struct stArgument& args)
{
	Global::nListenerIndex = args.m_nListenerIndex;
	Global::nHandlerIndex  = args.m_nHandlerIndex;
	Global::nClienteIndex  = args.m_nClientIndex;

	if (!Global::shmUtil.Attach(args.m_nShmKey, sizeof(Shm)))
	{
		hsfdlog(D_ERRO, "hsfd Error: Shared memory attach failed");
		return false;
	}
	
	if (!Global::shmAll.init(Global::shmUtil.GetShmPtr()))
	{
		Global::shmUtil.Detach();
		hsfdlog(D_ERRO, "hsfd Error: Shared memory reference is not valid");
		return false;
	}

	Global::pClientShm = std::make_unique<ShmClient>(Global::shmAll.Get_ClientInfo_Ptr(args.m_nHandlerIndex, args.m_nClientIndex));

	if (!Global::pClientShm)
	{
		Global::shmUtil.Detach();
		hsfdlog(D_ERRO, "hsfd Error: ClientShm is not valid");
		return false;
	}

	Global::pClientShm->Set_SessionDateID(args.m_nHandlerIndex, args.m_nClientIndex);  							// 1. Session Start Dt, Session ID 설정
	Global::pClientShm->Set_Pid_PSHandler(Global::shmAll.Get_PSHandlerInfo_Pid(args.m_nHandlerIndex)); 			// 2. Proxy Handler Process ID Set
	Global::pClientShm->Set_TargetServerInfo(args.m_nListenerIndex, args.m_strTargetIP, args.m_nTargetPort);	// 3. Target Server ip, port, Listener Index Set			
	Global::pClientShm->Set_PtlCode(PTL_FTP);
	Global::pClientShm->Set_ProtocolStr(PROTOCOL);

	Global::pMainPipePath = Global::shmAll.Get_MainPipePath();
	Global::pMainPipeName = Global::shmAll.Get_MainPipeName();

	auto tuple_connect_result = Global::asc.Connect(Global::pMainPipePath, Global::pMainPipeName);
	
	hsfdlog(D_ERRO, "%s", std::get<1>(tuple_connect_result).c_str());

	if (!std::get<0>(tuple_connect_result))
	{
		hsfdlog(D_ERRO, "Analysis not connected...");		

		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			return false;
		}
	}

	std::shared_ptr<AnalysisPacket> packet(new AnalysisPacket);
	packet->setConnectInfo((args.m_nHandlerIndex * Global::shmAll.Get_PSHandlerMaxClient()) + args.m_nClientIndex);
	
	// 분석 서버로 접속 정보 전송.
	auto tuple_send_analysis_result = Global::asc.SendToAnalysis(packet);

	if (!std::get<0>(tuple_send_analysis_result))
	{
		// TODO : 보안 설정 등급을 확인하여 분석 서버 연결이 안된 경우에 어떻게 동작할 것인지 확인
		hsfdlog(D_ERRO, "Send Fail...");
		hsfdlog(D_ERRO, "%s", std::get<1>(tuple_send_analysis_result).c_str());
		
		Global::asc.Close();
		Global::pClientShm->Set_Pid_ASHandler(-1);										
		Global::pClientShm->Set_Valid(1);

		if (Global::shmAll.Get_ASNotCon() == 1)
		{
			return false;
		}
	}
	
	if (Global::asc.IsValid())
	{
		hsfdlog(D_INFO, "Analysis Server Connect OK...");
	}

	Global::pClientShm->Set_Valid(2);
	return true;	
}

bool init(struct stArgument args)
{
	if(!Global::pFTP || !Global::pFTP->init(args.m_nClientSocket, args.m_strTargetIP, args.m_nTargetPort))
	{
		hsfdlog(D_ERRO, "Get FTP instance fail...");
		Global::CloseCode = eCloseCode::eAbNormal;
		return false;
	}

	if (!initAnalysisServer(args))
	{
		Global::CloseCode = eCloseCode::eAbNormal;
		return false;
	}

	if(!Global::pBuilder || !Global::pBuilder->init(Global::pFTP))
	{
		hsfdlog(D_ERRO, "Get command builder instance fail...");
		Global::CloseCode = eCloseCode::eAbNormal;
		return false;
	}
	
	if(!Global::pServer || !Global::pServer->init(Global::pFTP, Global::pBuilder))
	{
		hsfdlog(D_ERRO, "FTP Proxy Server init fail...");
		Global::CloseCode = eCloseCode::eAbNormal;
		return false;
	}

	hsfdlog(D_INFO, "Init OK...");
	return true;
}

void run()
{
	hsfdlog(D_INFO, "Run proxy server...");
    Global::pServer->run();
}

void wait()
{
    while (Global::pServer && !Global::pServer->wait())
	{
		if (!Global::pClientShm)
		{
			break;
		}

		auto nStatus = Global::pClientShm->Get_Status();

		if (M_SHM_STATUS_KILLED_MANAGER == nStatus)
		{
			hsfdlog(D_INFO, "Killed by manager");
			Global::CloseCode = eCloseCode::eManagerKill;
			break;
		}

		if (M_SHM_STATUS_KILLED_AGENT == nStatus)
		{
			hsfdlog(D_INFO, "killed by agent");
			Global::CloseCode = eCloseCode::eClientError;
			break;
		}

		if (M_SHM_STATUS_SESSION_TIMEOUT == nStatus)
		{
			hsfdlog(D_INFO, "Session time out");
			Global::CloseCode = eCloseCode::eSessTimeOut;
			break;
		}
	}
	
	Global::pFTP->closeServer();
	Global::pServer->stop();
}

void close()
{
	hsfdlog(D_INFO, "Close proxy server...");
	Global::shmAll.Set_PSHandlerInfo_CurrentUserMinus(Global::nHandlerIndex);
	Global::shmAll.Set_ClientInfo_Valid(Global::nHandlerIndex, Global::nClienteIndex, 0);

	RemoveDataChannelPort();

	// 하드코딩 수정
	if (!Global::bValidAnalysisServer && (Global::pClientShm->Get_Valid() == 2))
	{
		Global::pClientShm->Set_Valid(1);
		Global::pClientShm->Set_UpdateFlag(true);
	}

	if (!Global::asc.IsValid())
	{
		return;
	}

	std::shared_ptr<AnalysisPacket> packet(new AnalysisPacket);
	packet->setCloseInfo(Global::CloseCode);
	packet->setData(eDataIndex::eResponseMessage, std::move(Global::strResponseMessage));

	if (eCloseCode::ePolicyDeny == Global::CloseCode)
	{
		packet->setData(eDataIndex::eCommand, Global::strDenyCommand);
	}

	Global::asc.SendToAnalysis(packet);
	Global::asc.Close();
}