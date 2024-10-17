#include "ProxyThread.hpp"

#include <sys/socket.h>
#include <unistd.h>
#include <future>
#include <netdb.h>
#include <future>
#include <ifaddrs.h>
#include <boost/asio.hpp>

constexpr auto THREAD_USLEEP_TIME 			= 1000;
constexpr auto ANALYSIS_CHECK_TIME_INTERVAL 	= 10;

#define USE_BOOST_ASIO 0

// Get Listener IP
const std::string strProxyIP = []
{
	std::string strIP;

	struct ifaddrs *pAddr = nullptr;
	getifaddrs(&pAddr);

	while (true)
	{
		if (!pAddr->ifa_addr)
		{
			pAddr = pAddr->ifa_next;
			continue;
		}

		if ((pAddr->ifa_addr->sa_family == AF_INET) && (strcmp(pAddr->ifa_name, "lo") != 0))
		{
			char szBuffer[INET_ADDRSTRLEN];
			memset(szBuffer, 0x00, sizeof(szBuffer));

			strIP = inet_ntoa(((struct sockaddr_in *)pAddr->ifa_addr)->sin_addr);
			break;
		}

		pAddr = pAddr->ifa_next;
	}

	if (!pAddr)
	{
		freeifaddrs(pAddr);
	}

	return strIP;
}();

int get_sock_port(int sock, int local)
{
	struct sockaddr_storage from;
	char strport[NI_MAXSERV];
	int r;

	/* Get IP address of client. */
	socklen_t fromlen = sizeof(from);
	memset(&from, 0, sizeof(from));
	if (local) {
		if (getsockname(sock, (struct sockaddr *)&from, &fromlen) == -1)
        {
			// g_SysLog.WriteInfo("getsockname failed: %.100s", strerror(errno));
            hsrLog(D_ERRO, "getsockname failed: %.100s", strerror(errno));
			return 0;
		}
	} else {
		if (getpeername(sock, (struct sockaddr *)&from, &fromlen) == -1)
        {
			// g_SysLog.WriteInfo("getpeername failed: %.100s", strerror(errno));
            hsrLog(D_ERRO, "getsockname failed: %.100s", strerror(errno));
			return -1;
		}
	}

	/* Work around Linux IPv6 weirdness */
	if (from.ss_family == AF_INET6)
		fromlen = sizeof(struct sockaddr_in6);

	/* Non-inet sockets don't have a port number. */
	if (from.ss_family != AF_INET && from.ss_family != AF_INET6)
		return 0;

	/* Return port number. */
	if ((r = getnameinfo((struct sockaddr *)&from, fromlen, NULL, 0, strport, sizeof(strport), NI_NUMERICSERV)) != 0)
    {
		// g_SysLog.WriteInfo("getnameinfo NI_NUMERICSERV failed: %s", gai_strerror(r));
        hsrLog(D_ERRO, "getnameinfo NI_NUMERICSERV failed: %s", gai_strerror(r));
    }

	return atoi(strport);
}

/// @brief 각각의 Client 들을 처리하기 위한 Thread Function
/// @return
void ProxyThreadProc(const std::atomic<bool>& bStop, int clientSockDesc, int iClientIndex, int iListenerIndex, ShmAll *shmUtil)
{
	HandlerStartInfo* pHandlerInfo = HandlerStartInfo::getInstance();
	
	if (!pHandlerInfo || !shmUtil->IsValid())
	{
		close(clientSockDesc);		// Client 소켓 종료
		return;
	}

	int nHandlerIndex = pHandlerInfo->getHandlerIndex();
			
	struct timeval tv_timeo = { 3, 0 };

	if (setsockopt( clientSockDesc, SOL_SOCKET, SO_SNDTIMEO, &tv_timeo, sizeof(tv_timeo)) != 0)
	{
		fprintf(stderr, ">>>Client SO_SNDTIMEO Set Failed.\n");
	}

	int iServerPort = shmUtil->Get_ListenerTargetPort(iListenerIndex);
	string strServerIp;

	shmUtil->Get_ListenerTargetIp( iListenerIndex, strServerIp);


	int serverSockDesc = ::ConnectServer(strServerIp.c_str(), iServerPort);

	if( serverSockDesc < 0) // 연결을 실패한 경우.
	{
		close(clientSockDesc);
		return;
	}

	clientShm.Set_ProtocolStr("RDP");	

	std::atomic<bool> bAsyncStop = false;	
	eCloseCode closeCode = eCloseCode::eNormal;

#if USE_BOOST_ASIO
	boost::asio::io_context clientContext;
	boost::asio::io_context serverContext;

	boost::asio::ip::tcp::socket clientSocket(clientContext, boost::asio::ip::tcp::v4(), clientSockDesc);
	boost::asio::ip::tcp::socket serverSocket(serverContext, boost::asio::ip::tcp::v4(), serverSockDesc);
#endif

	auto readFromClientProc = std::async(std::launch::async, [&]
	{
		fd_set set;
		struct timeval timeout;
		timeout.tv_sec  = PROXY_CONNECT_CHECK;
		timeout.tv_usec = 0;

		auto time = std::chrono::steady_clock::now() - std::chrono::seconds(15);

#if USE_BOOST_ASIO		
	std::array<char, PROXY_SOCKET_BUFFER_SIZE> buffer;
	boost::system::error_code error;
#else
	char szBuffer[PROXY_SOCKET_BUFFER_SIZE];
#endif
		while (!bAsyncStop)
		{
			usleep(THREAD_USLEEP_TIME);
			FD_ZERO(&set);
			FD_SET(clientSockDesc, &set);

			if (select(clientSockDesc + 1, &set, nullptr, nullptr, &timeout) < 0)
			{
				if (errno == EINTR)
					continue;
				break;
			}

			if (!FD_ISSET(clientSockDesc, &set))
			{
				continue;
			}

#if USE_BOOST_ASIO
			try
			{
				size_t nReadSize = clientSocket.read_some(boost::asio::buffer(buffer), error);

				if (error == boost::asio::error::eof)
				{
					hsrLog(D_INFO, "Connection closed by peer");
					break;
				}
				else if (error)
				{
					throw boost::system::system_error(error);
				}

				boost::asio::const_buffer write_buf(buffer.data(), nReadSize);
				size_t nWriteSize = serverSocket.write_some(boost::asio::buffer(write_buf), error);

				if (error)
				{
					throw boost::system::system_error(error);
				}

				if (nReadSize != nWriteSize)
				{
					hsrLog(D_ERRO, "PT-ERR: WriteN(C->S): Data sending to server is failed [%d/%d][%d][%s]", nWriteSize, nReadSize, errno, strerror(errno));
					break;
				}
			}
			catch (std::exception& e)
			{
				hsrLog(D_ERRO, "%s", e.what());
				break;
			}
#else
			// Read
			memset(szBuffer, 0x00, PROXY_SOCKET_BUFFER_SIZE);        
			ssize_t nReadSize = ::Read(clientSockDesc, szBuffer, PROXY_SOCKET_BUFFER_SIZE);

			if (nReadSize <= 0)
			{
				hsrLog(D_ERRO, "Result : %d... Close socket", nReadSize);
				hsrLog(D_ERRO, "strerror : %s", strerror (errno));
				hsrLog(D_ERRO, "errno : %d", errno);				
				break;
			}

			// Write
			ssize_t nWriteSize = ::WriteN(serverSockDesc, szBuffer, nReadSize);

			if (nWriteSize != nReadSize)
			{
				hsrLog(D_ERRO, "PT-ERR: WriteN(C->S): Data sending to server is failed [%d/%d][%d][%s]", nWriteSize, nReadSize, errno, strerror(errno));
				break;
			}
#endif
		}

		hsrLog(D_INFO, "readFromClient end");
	});

	auto readFromTargetProc = std::async(std::launch::async, [&]
	{
		fd_set set;
		struct timeval timeout;
		timeout.tv_sec  = PROXY_CONNECT_CHECK;
		timeout.tv_usec = 0;

		auto time = std::chrono::steady_clock::now() - std::chrono::seconds(15);

#if USE_BOOST_ASIO
		std::array<char, PROXY_SOCKET_BUFFER_SIZE> buffer;
		boost::system::error_code error;
#else
		char szBuffer[PROXY_SOCKET_BUFFER_SIZE];
#endif
		while (!bAsyncStop)
		{
			usleep(THREAD_USLEEP_TIME);
			FD_ZERO(&set);
			FD_SET(serverSockDesc, &set);

			if (select(serverSockDesc + 1, &set, nullptr, nullptr, &timeout) < 0)
			{
				if (errno == EINTR)
					continue;
				break;
			}

			if (!FD_ISSET(serverSockDesc, &set))
			{
				continue;
			}

#if USE_BOOST_ASIO
			try
			{
				size_t nReadSize = serverSocket.read_some(boost::asio::buffer(buffer), error);

				if (error == boost::asio::error::eof)
				{
					hsrLog(D_INFO, "Connection closed by server");
					break;
				}
				else if (error)
				{
					throw boost::system::system_error(error);
				}

				boost::asio::const_buffer write_buf(buffer.data(), nReadSize);
				size_t nWriteSize = clientSocket.write_some(boost::asio::buffer(write_buf), error);

				if (error)
				{
					throw boost::system::system_error(error);
				}

				if (nReadSize != nWriteSize)
				{
					hsrLog(D_ERRO, "PT-ERR: WriteN(S->N): Data sending to client is failed [%d/%d][%d][%s]", nWriteSize, nReadSize, errno, strerror(errno));
					break;
				}
			}
			catch (std::exception& e)
			{
				hsrLog(D_ERRO, "%s", e.what());
				break;
			}
#else
			// Read
			memset(szBuffer, 0x00, PROXY_SOCKET_BUFFER_SIZE);        
			ssize_t nReadSize = ::Read(serverSockDesc, szBuffer, PROXY_SOCKET_BUFFER_SIZE);

			if (nReadSize <= 0)
			{
				hsrLog(D_ERRO, "Result : %d... Close socket", nReadSize);
				break;
			}

			// Write
			ssize_t nWriteSize = ::WriteN(clientSockDesc, szBuffer, nReadSize);

			if (nWriteSize != nReadSize)
			{
				hsrLog(D_ERRO, "PT-ERR: WriteN(S->C): Data sending to server is failed [%d/%d][%d][%s]", nWriteSize, nReadSize, errno, strerror(errno));
				break;
			}
#endif
		}

		hsrLog(D_INFO, "readFromTargetProc end");
	});

	// 실제 Loop 시작
	while (true)
	{
		bAsyncStop = (readFromClientProc.wait_for(std::chrono::seconds(1)) == std::future_status::ready) ||
					 (readFromTargetProc.wait_for(std::chrono::seconds(1)) == std::future_status::ready) ||
					 bStop;

		if (bAsyncStop)
		{
			readFromClientProc.get();
			readFromTargetProc.get();
			break;
		}
	}	

#if USE_BOOST_ASIO
	clientSocket.close();
	serverSocket.close();
#endif

	close(clientSockDesc);
	close(serverSockDesc);

	usleep(20000);

	return;
}