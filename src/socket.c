#include "cutils/socket.h"
#include "cutils/char-array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int open_server_socket(int socktype, const char *host, int port) {
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	struct addrinfo hints, *result, *rp;
	int sfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = socktype;
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

	char ports[32];
	sprintf(ports, "%d", port);

	int err = getaddrinfo(host, port ? ports : NULL, &hints, &result);
	if (err) {
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = (int)socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
#ifdef WIN32
		if (rp->ai_family == AF_INET6) {
			set_ipv6_only(sfd, false);
		}
#endif
		unsigned long reuse = 1;
		if (port && setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))) {
			goto err;
		}
		if (bind(sfd, rp->ai_addr, (int)rp->ai_addrlen)) {
			goto err;
		}
		if (socktype == SOCK_STREAM && listen(sfd, SOMAXCONN)) {
			goto err;
		}
		break;
	err:
		closesocket(sfd);
	}

	if (rp == NULL) {
		return -1;
	}

	freeaddrinfo(result);
	return sfd;
}

int must_open_server_socket(int socktype, const char *host, int port) {
	int fd = open_server_socket(socktype, host, port);
	if (fd < 0) {
		fprintf(stderr, "failed to bind server port %s:%d\n", host ? host : "", port);
		exit(2);
	}
	return fd;
}

int open_client_socket(int socktype, const char *host, int port) {
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	struct addrinfo hints, *result, *rp;
	int fd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = socktype;

	char ports[32];
	sprintf(ports, "%d", port);

	if (getaddrinfo(host, ports, &hints, &result)) {
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = (int)socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd == -1) {
			continue;
		}

		if (!connect(fd, rp->ai_addr, (int)rp->ai_addrlen)) {
			break;
		}

		closesocket(fd);
	}

	freeaddrinfo(result);
	return rp ? fd : -1;
}

static const char *print_ipv4(stack_string *s, const uint8_t *addr, uint16_t port) {
	ca_setf(s, "%d.%d.%d.%d:%d", addr[0], addr[1], addr[2], addr[3], ntohs(port));
	return s->c_str;
}

const char *sockaddr_string(stack_string *s, const struct sockaddr *sa, socklen_t sasz) {
	static const uint8_t mapped6[] = { 0,0,0,0,0,0,0,0,0,0,255,255 };
	struct sockaddr_in6 *sa6 = (struct sockaddr_in6*)sa;
	struct sockaddr_in *sa4 = (struct sockaddr_in*)sa;
	if (sa->sa_family == AF_INET6 && !memcmp(sa6->sin6_addr.s6_addr, mapped6, sizeof(mapped6))) {
		return print_ipv4(s, sa6->sin6_addr.s6_addr + 12, sa6->sin6_port);
	} else if (sa->sa_family == AF_INET) {
		return print_ipv4(s, (uint8_t*)&sa4->sin_addr.s_addr, sa4->sin_port);
	} else {
		char host[64], port[16];
		if (getnameinfo(sa, sasz, host, sizeof(host), port, sizeof(port), NI_NUMERICHOST | NI_NUMERICSERV)) {
			return "INVALID ADDRESS";
		} else {
			ca_setf(s, "[%s]:%s", host, port);
			return s->c_str;
		}
	}
}

const char *syserr_string(stack_string *s) {
#ifdef WIN32
	DWORD err = WSAGetLastError();
	wchar_t wbuf[512];
	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		wbuf, 511, NULL);
	wbuf[511] = L'\0';
	wchar_t* end = wbuf + wcslen(wbuf);
	if (end[-1] == L'\n') {
		end--;
	}
	if (end[-1] == L'\r') {
		end--;
	}
	*end = L'\0';
	ca_setf(s, "%d: ", err);
	s->len += (size_t)WideCharToMultiByte(CP_ACP, 0, wbuf, -1, s->c_str + s->len, (int)(sizeof(s->c_str) - s->len - 1), 0, NULL);
	s->c_str[s->len] = 0;
	return s->c_str;
#else
	return strerror(errno);
#endif
}

