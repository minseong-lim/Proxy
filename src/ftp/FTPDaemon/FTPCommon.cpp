#include "FTPCommon.hpp"
#include <signal.h>
#include <boost/format.hpp>

namespace Global
{
	AnalysisSocketControl		asc;
	std::vector<unsigned int>   DataChannelPort;

	bool 		bValidAnalysisServer = true;
	std::string strResponseMessage	 = std::string();
	std::string strDenyCommand		 = std::string();
	eCloseCode 	CloseCode 			 = eCloseCode::eNormal;

	const char* pMainPipePath		 = nullptr;
	const char* pMainPipeName		 = nullptr;

	HSCommon::ShmemUtil<Shm> shmUtil;
	ShmAll 					 shmAll;
	
	std::unique_ptr<ShmClient>	pClientShm	= nullptr;
	
	int nListenerIndex = 0;
	int	nHandlerIndex  = 0;
	int	nClienteIndex  = 0;
}

auto execSystem = [](std::string strCommand)
{
	int nStatus = system(strCommand.c_str());	
        
	if (WIFSIGNALED(nStatus) && (WTERMSIG(nStatus) == SIGINT || WTERMSIG(nStatus) == SIGQUIT))
	{
		hsfdlog(D_ERRO, "ERROR: Program terminited by user abort");
		return false;
	}

	hsfdlog(D_INFO, "EXEC : %s.....%s", strCommand.c_str(), (0 == WEXITSTATUS(nStatus) ? "OK!" : "Fail!"));
	return true;
};

bool AddDataChannelPort(unsigned int nPort)
{
    boost::format fmt(ADD_FIREWALL_PORT);
	fmt % nPort;

	if (!execSystem(fmt.str()))
	{
		return false;
	}

	Global::DataChannelPort.emplace_back(nPort);

    return true;
}

bool RemoveDataChannelPort()
{
    if (Global::DataChannelPort.empty())
    {
        return true;
    }
    
    std::for_each(Global::DataChannelPort.begin(), Global::DataChannelPort.end(), [](const auto &nPort)
	{
		boost::format fmt(REMOVE_FIREWALL_PORT);
		fmt % nPort;

		execSystem(fmt.str());
	});

    Global::DataChannelPort.clear();

    return true;
}