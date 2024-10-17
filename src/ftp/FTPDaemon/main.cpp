#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <string>
#include <netinet/tcp.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__)
#define LOGFACILITY	LOG_FTP
#else
#define LOGFACILITY	LOG_DAEMON
#endif

#define VERSION "1.0.0"

#include <iostream>
void signal_handler(int sig)
{
	/*
	 * Changed the way we handle broken pipes (broken control or
	 * data connection).  We ignore it here but write() returns -1
	 * and errno is set to EPIPE which is checked.
	 */

	if (sig == SIGPIPE)
	{
		signal(SIGPIPE, signal_handler);
		return;
	}

	syslog(LOG_NOTICE, "-ERR: received signal #%d", sig);
	exit(1);
}

int set_signals(void)
{
	signal(SIGHUP, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGQUIT, signal_handler);
	signal(SIGSEGV, signal_handler);
	signal(SIGPIPE, signal_handler);
	signal(SIGALRM, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	return (0);
}

int main(int argc, char *argv[], char *envp[])
{	
	hsfdlog(D_INFO, "Start hsfSDaemon...");

	std::string program = argv[0];
	openlog(program.c_str(), LOG_PID, LOGFACILITY);

	if (8 < argc)
	{
		hsfdlog(D_ERRO, "%s: argument is not valid ...", program.c_str());
		exit(1);
	}

	for(int i = 0; i < argc; i++)
	{
		hsfdlog(D_INFO, "argv[%d]: [%s]", i, argv[i]);
	}

	struct stArgument args(argc, argv);

	struct linger linger;
	hsfdlog(D_INFO, "%d: Child: fork process", getpid());

	linger.l_onoff = 1;
	linger.l_linger = 2;
	int optlen = sizeof(linger);

	if (setsockopt(args.m_nClientSocket, SOL_SOCKET, SO_LINGER, &linger, optlen) != 0)
	{
		hsfdlog(D_ERRO, "%04X: can't set linger", getpid());
	}

#if defined(__linux__)
	int rc = 1;
	if (setsockopt(args.m_nClientSocket, SOL_TCP, TCP_NODELAY, &rc, sizeof(rc)) != 0)
	{
		syslog(LOG_NOTICE, "can't set TCP_NODELAY, error= %s", strerror(errno));
	}
#endif

	//////////////////////////////////////////////////////////////////////////////////////////////////////

	if(init(args))
	{
		run();
		wait();		
	}

	close();

	return 0;
}