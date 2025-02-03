// Start cogserver in a new process.
//

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <opencog/cogserver/server/CogServer.h>

using namespace opencog;

static CogServer* srvr = nullptr;
std::thread * main_loop = nullptr;

// Send stuff to the server.
void chat(const char* msg)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(16001);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	int rc = connect(sock, (struct sockaddr*)&saddr, sizeof(saddr));
	TSM_ASSERT("Failed to connect to cogserver!", -1 != rc);
	write(sock, msg, strlen(msg));
	close(sock);
}

void start_cogserver(bool do_fork = true)
{
	// Create a single cogserver node that will act as
	// as the repo for the duration of the test.
	if (do_fork)
	{
		pid_t child = fork();
		TSM_ASSERT("Failed to fork cogserver!", -1 != child);
		if (0 != child)
		{
			// Wait for child to finish initializing
			sleep(3);
			chat("help");
			printf("Connected to CogServer\n");
			return;
		}
	}
	srvr = &cogserver();
	srvr->loadModules();
	srvr->enableNetworkServer(16001);

	if (not do_fork)
	{
		// Run cogserver in a thread.
		main_loop = new std::thread(&CogServer::serverLoop, srvr);
		printf("Started CogServer in thread\n");
		return;
	}
	printf("Started CogServer in separate process\n");
	srvr->serverLoop();
	printf("Exit from cogserver main loop\n");
}

void stop_cogserver()
{
	// If main loop is null, then the cogserver is in a different
	// process; lese its in a different thread.
	if (nullptr == main_loop)
	{
		chat("shutdown");
		sleep(1);
		printf("Told cogserver to stop\n");
		return;
	}

	srvr->stop();
	main_loop->join();
	srvr->disableNetworkServer();
	delete main_loop;
	delete srvr;
}
