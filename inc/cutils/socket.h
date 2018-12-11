#pragma once

#ifdef WIN32
#define _WIN32_WINNT 0x600
#include <basetsd.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32")
typedef WSAPOLLFD pollfd;
static int poll(_Inout_ LPWSAPOLLFD fdArray, _In_ ULONG fds, _In_ INT timeout) {
	return WSAPoll(fdArray, fds, timeout);
}
#define SHUT_WR SD_SEND
typedef SSIZE_T ssize_t;
#define SSIZE_T_MIN MINSSIZE_T
#define SSIZE_T_MAX MAXSSIZE_T
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

static inline bool call_again() {
#ifdef WIN32
	return false;
#else
	return errno == EINTR;
#endif
}

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

static inline int set_cloexec(int fd) {
#ifdef WIN32
	return 0;
#else
	return fcntl(fd, F_SETFD, O_CLOEXEC);
#endif
}

static inline int set_ipv6_only(int fd, bool v6only) {
	unsigned long val = v6only;
	return setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&val, sizeof(val));
}

int must_open_server_socket(int socktype, const char *host, int port);
int open_server_socket(int socktype, const char *host, int port);
int open_client_socket(int socktype, const char *host, int port);

typedef struct stack_string stack_string;
struct stack_string {
	size_t len;
	char c_str[128];
};

const char *sockaddr_string(stack_string *s, const struct sockaddr *sa, socklen_t sasz);
const char *syserr_string(stack_string *s);
