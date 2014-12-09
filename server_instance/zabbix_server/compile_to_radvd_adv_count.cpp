#include <unistd.h>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <cstdio>
#include <map>
#include <list>

#define MAX_GW_AGE 300
#define SOCKET "/tmp/radvdump_gateway_count.sock"
int stop = 0;
int radvdFD = 0;
std::map<std::string, time_t> routerLastSeenMap;

void runRadvd() {
	if (radvdFD != 0) {
		close(radvdFD);
	}

	int fd[2];
	pipe(fd);

	int pid = fork();
	if (pid == 0) {
		close(fd[0]);
		dup2(fd[1], 1);
		execlp("radvdump", "radvdump", NULL);
	} else if (pid < 0) {
		// error
	} else {
		close(fd[1]);
		radvdFD = fd[0];
	}
}

int openConnection() {
	int s, t, len;
	struct sockaddr_un remote;
	char str[100];

	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		return -1;
	}
	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, SOCKET);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);

	if (connect(s, (struct sockaddr *)&remote, len) == -1) {
		return -1;
	} else {
		return s;
	}
}

void updateRouter(const std::string &routerString) {
	if (routerLastSeenMap.find(routerString) != routerLastSeenMap.end()) {
		routerLastSeenMap.erase(routerString);
	}

	routerLastSeenMap.insert(std::make_pair(routerString, time(NULL)));
}

void parseBlock(const std::string &string) {
	std::stringstream ss(string);
	std::string line;

	while (std::getline(ss, line, '\n')) {
		size_t pos;
		std::string needle("based on Router Advertisement from");
		if ((pos = line.find(needle)) != std::string::npos) {
			std::string routerString = line.substr(pos + needle.size() + 1);
			updateRouter(routerString);
		}
	}
}

uint64_t countRouters() {
	std::list<std::string> timedOut;
	uint64_t count = 0;
	time_t now = time(NULL);
	for (std::map<std::string, time_t>::iterator it = routerLastSeenMap.begin(); it != routerLastSeenMap.end(); ++it) {
		if (now - it->second < MAX_GW_AGE) {
			count++;
		} else {
			timedOut.push_back(it->first);
		}
	}

	for (std::list<std::string>::iterator it = timedOut.begin(); it != timedOut.end(); ++it) {
		routerLastSeenMap.erase(*it);
	}
	return count;
}

int createSocket() {
        int create_socket;
        if((create_socket = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0) {
                perror("socket");
		return -1;
        }

        unlink(SOCKET);

        struct sockaddr_un address;
        socklen_t addrlen;
        address.sun_family = AF_UNIX;
        strcpy(address.sun_path, SOCKET);
        addrlen = sizeof(address);

        if(bind(create_socket, (struct sockaddr *)&address, addrlen) != 0) {
                perror("bind");
		return -1;
        }
        listen(create_socket, 5);
        chmod(SOCKET, 0666);

        return create_socket;
}

void handleClient(int socket) {
        struct sockaddr_un address;
        socklen_t addrlen;
	int client = accept(socket, (struct sockaddr *)&address, &addrlen);
	if(client >= 0) {
		std::stringstream ss("");
		ss << countRouters() << "\n";
		write(client, ss.str().c_str(), ss.str().size());
		close(client);
	}
}


void serverMode() {
	int socket = createSocket();
	runRadvd();
	while (!stop) {
		static fd_set fdSet;
		FD_ZERO(&fdSet);
		//FD_SET(0, &fdSet);
		FD_SET(socket, &fdSet);
		FD_SET(radvdFD, &fdSet);

		int maxFd = radvdFD;

		int res = select(maxFd + 1, &fdSet, NULL, NULL, NULL);
		if (res == -1)
			continue;

		if (FD_ISSET(socket, &fdSet)) {
			handleClient(socket);
		}

		if (FD_ISSET(radvdFD, &fdSet)) {
			char buf[1024];
			int n = read(radvdFD, buf, 1024);
			if (n > 0) {
				parseBlock(std::string(buf, n));
			}
		}

		if (FD_ISSET(0, &fdSet)) {
			char buf[16];
			int n = read(0, buf, 16);
			std::cout << countRouters() << std::endl;
		}
	}
}

int main(void) {

	int clientFd = openConnection();
	if (clientFd == -1) {
		int pid = fork();
		if (pid == 0)
			serverMode();
		sleep(10);
		clientFd = openConnection();
		if (clientFd == -1) {
			std::cout << "FAIL." << std::endl;
		}
	}

	char buf[128];
	size_t n = read(clientFd, buf, 128);
	write(1, buf, n);

	return 0;
}
