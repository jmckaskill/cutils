#include "cutils/socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int open_server_socket(const char *host, int port) {
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	struct addrinfo hints, *result, *rp;
	int sfd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

	char ports[32];
	sprintf(ports, "%d", port);

	int err = getaddrinfo(host, ports, &hints, &result);
	if (err) {
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = (int)socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1) {
			continue;
		}
#ifdef WIN32
		DWORD not_v6only = 0;
		setsockopt(sfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &not_v6only, sizeof(not_v6only));
#endif
		unsigned long reuse = 1;
		if (!setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse))
		&& !bind(sfd, rp->ai_addr, (int)rp->ai_addrlen)
		&& !listen(sfd, SOMAXCONN)) {
			break;                  /* Success */
		}
		closesocket(sfd);
	}

	if (rp == NULL) {
		return -1;
	}

	freeaddrinfo(result);
	return sfd;
}

int must_open_server_socket(const char *host, int port) {
	int fd = open_server_socket(host, port);
	if (fd < 0) {
		fprintf(stderr, "failed to bind server port %s:%d\n", host ? host : "", port);
		exit(2);
	}
	return fd;
}

int open_client_socket(const char *host, int port) {
#ifdef WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

	struct addrinfo hints, *result, *rp;
	int fd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* TCP socket */

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
