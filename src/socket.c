#include "cutils/socket.h"
#include "cutils/char-array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int print_sockaddr(struct sockaddr_string *s, const struct sockaddr *sa, int sasz) {
	if (getnameinfo(sa, sasz, s->host.c_str, sizeof(s->host.c_str), s->port.c_str, sizeof(s->port.c_str), NI_NUMERICHOST | NI_NUMERICSERV)) {
		ca_setlen(&s->host, 0);
		ca_setlen(&s->port, 0);
		return -1;
	}
	ca_setlen(&s->host, strlen(s->host.c_str));
	ca_setlen(&s->port, strlen(s->port.c_str));
	return 0;
}

