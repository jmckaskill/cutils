#pragma once

#ifdef WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
typedef WSAPOLLFD pollfd;
#define poll WSAPoll
#pragma comment(lib,"ws2_32")
#define SHUT_WR SD_SEND
#else
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#define closesocket(fd) close(fd)
#endif

#include <stdbool.h>

static inline bool would_block() {
#ifdef WIN32
	return WSAGetLastError() == WSAEWOULDBLOCK;
#else
	return errno == EWOULDBLOCK || errno == EINTR;
#endif
}

static inline int set_non_blocking(int fd) {
#ifdef WIN32
	u_long nonblock = 1;
	return ioctlsocket(fd, FIONBIO, &nonblock);
#else
	return fcntl(fd, F_SETFL, O_NONBLOCK);
#endif
}

struct sockaddr_string {
	struct {
		size_t len;
		char c_str[40];
	} host;
	struct {
		size_t len;
		char c_str[8];
	} port;
};

int must_open_server_socket(int socktype, const char *host, int port);
int open_server_socket(int socktype, const char *host, int port);
int open_client_socket(int socktype, const char *host, int port);

int print_sockaddr(const struct sockaddr *sa, int sasz, struct sockaddr_string *s);


