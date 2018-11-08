#include "cutils/stream.h"
#include "bearssl_wrapper.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef WIN32
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <WS2tcpip.h>
#endif

#ifdef _MSC_VER
#define strcasecmp(a,b) _stricmp(a,b)
#endif

#include "ssl-roots.h"

static int open_client_socket(const char *host, int port) {
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

static int do_read(void *read_context, unsigned char *data, size_t len) {
	int fd = *(int*)read_context;
	return recv(fd, (char*) data, len > INT_MAX ? INT_MAX : (int)len, 0);
}

static int do_write(void *write_context, const unsigned char *data, size_t len) {
	int fd = *(int*)write_context;
	return send(fd, (const char*) data, len > INT_MAX ? INT_MAX : (int)len, 0);
}

typedef struct https_stream https_stream;

struct https_stream {
	stream iface;
	br_sslio_context ctx;
	br_ssl_client_context sc;
	br_x509_minimal_context xc;
	int fd, port;
	size_t bufsz, avail, consumed;
	unsigned is_https;
	char *host;
	int64_t length_remaining;
	int last_percent_report;
	uint8_t inrec[32 * 1024];
	uint8_t outrec[32 * 1024];
	uint8_t *buf;
};

static void close_https(struct stream *s) {
	https_stream *os = (https_stream*)s;
	free(os->host);
	closesocket(os->fd);
	free(os->buf);
	free(os);
}

static const uint8_t *read_https(struct stream *s, size_t consume, size_t need, size_t *plen) {
	https_stream *os = (https_stream*) s;
	os->consumed += consume;
	os->length_remaining -= consume;
	if ((int64_t)need > os->length_remaining) {
		need = (size_t)os->length_remaining;
	}

	// see if we can service from the existing buffer
	if (os->consumed + need <= os->avail) {
		*plen = os->avail - os->consumed;
		return os->buf + os->consumed;
	}

	// compress the buffer
	if (os->consumed && os->consumed < os->avail) {
		memmove(os->buf, os->buf + os->consumed, os->avail - os->consumed);
	}
	os->avail -= os->consumed;
	os->consumed = 0;

	// get more data
	while (os->avail < need) {
		if (os->avail == os->bufsz) {
			size_t bufsz = os->bufsz + 32 * 1024;
			uint8_t *buf = realloc(os->buf, bufsz);
			if (!buf) {
				goto err;
			}
			os->buf = buf;
			os->bufsz = bufsz;
		}
		int r;
		if (os->is_https) {
			r = br_sslio_read(&os->ctx, os->buf + os->avail, os->bufsz - os->avail);
			if (r <= 0) {
				fprintf(stderr, "ssl error on read %d\n", br_ssl_engine_last_error(os->ctx.engine));
				goto err;
			}
		} else {
			r = recv(os->fd, (char*)os->buf + os->avail, (int)(os->bufsz - os->avail), 0);
			if (r <= 0) {
				fprintf(stderr, "error on read\n");
				goto err;
			}
		}
		os->avail += r;
		if ((int64_t)os->avail > os->length_remaining) {
			goto err;
		}
	}
	*plen = os->avail;
	return os->buf;
err:
	os->length_remaining = 0;
	*plen = 0;
	return NULL;
}

static char *get_line(struct https_stream *os) {
	size_t line_len = 0;
	for (;;) {
		os->length_remaining = INT64_MAX;
		char *line = (char*) read_https(&os->iface, 0, line_len+1, &line_len);
		char *nl = memchr(line, '\n', line_len);
		if (nl) {
			os->consumed += nl - line + 1;
			if (nl > line && nl[-1] == '\r') {
				nl--;
			}
			*nl = 0;
			return line;
		} else if (line_len > 512) {
			fprintf(stderr, "overlong header line\n");
			return NULL;
		}
	}
}

static int is_space(char ch) {
	return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static char *trim(char *str) {
	while (is_space(*str)) {
		str++;
	}
	char *end = str + strlen(str);
	while (end > str && is_space(end[-1])) {
		end--;
	}
	*end = 0;
	return str;
}

stream *open_http_downloader(const char *url, uint64_t *ptotal) {
	int redirects = 0;
	char *free_url = NULL;
	https_stream *os = calloc(1, sizeof(https_stream));
	if (!os) {
		return NULL;
	}
	os->iface.close = &close_https;
	os->iface.read = &read_https;
	os->fd = -1;
	for (;;) {
		if (++redirects == 5) {
			fprintf(stderr, "too many redirects\n");
			goto err;
		}

		unsigned is_https;

		if (!strncmp(url, "https://", strlen("https://"))) {
			is_https = 1;
			url += strlen("https://");
		} else if (!strncmp(url, "http://", strlen("http://"))) {
			is_https = 0;
			url += strlen("http://");
		} else {
			goto err;
		}

		const char *path = strchr(url, '/');
		char *host;
		if (path) {
			host = (char*) malloc(path - url + 1);
			memcpy(host, url, path - url);
			host[path - url] = 0;
		} else {
			host = strdup(url);
			path = "/";
		}

		int port = is_https ? 443 : 80;
		char *colon = strchr(host, ':');
		if (colon) {
			port = atoi(colon + 1);
			*colon = 0;
		}

		if (os->host && !strcmp(host, os->host) && os->is_https == is_https && os->port == port) {
			// we can reuse the existing connection
			free(host);
			host = os->host;
		} else {
			int fd = open_client_socket(host, port);
			if (fd < 0) {
				fprintf(stderr, "failed to connect to %s\n", host);
				free(host);
				goto err;
			}
			free(os->host);
			closesocket(os->fd);
			os->port = port;
			os->host = host;
			os->fd = fd;
			os->is_https = is_https;
			os->avail = 0;
			os->consumed = 0;

			if (is_https) {
				br_ssl_client_init_full(&os->sc, &os->xc, TAs, TAs_NUM);
				br_ssl_engine_set_buffers_bidi(&os->sc.eng, os->inrec, sizeof(os->inrec), os->outrec, sizeof(os->outrec));
				br_ssl_client_reset(&os->sc, host, 0);
				br_sslio_init(&os->ctx, &os->sc.eng, &do_read, &os->fd, &do_write, &os->fd);
			}
		}

		char request[1024];
		int reqsz = snprintf(request, sizeof(request), "GET %s HTTP/1.1\r\nHost:%s\r\n\r\n", path, host);

		if (is_https) {
			int w = br_sslio_write_all(&os->ctx, request, reqsz);
			int f = br_sslio_flush(&os->ctx);
			if (w < 0 || f < 0) {
				fprintf(stderr, "ssl error on write %d\n", br_ssl_engine_last_error(&os->sc.eng));
				goto err;
			}
		} else {
			char *p = request;
			while (reqsz) {
				int w = send(os->fd, p, reqsz, 0);
				if (w <= 0) {
					fprintf(stderr, "write error\n");
					goto err;
				}
				reqsz -= w;
			}
		}

		char *hdr = get_line(os);
		if (!hdr) {
			goto err;
		}
		char *httpcode = strchr(hdr, ' ');
		if (!httpcode) {
			fprintf(stderr, "invalid header %s\n", hdr);
			goto err;
		}
		int httperr = atoi(httpcode);
		if (httperr / 100 > 3) {
			fprintf(stderr, "http error %d\n", httperr);
			goto err;
		}

		int64_t content_length = -1;
		unsigned have_location = 0;

		for (;;) {
			char *p = get_line(os);
			if (!p) {
				goto err;
			} else if (*p == 0) {
				break;
			}
			char *keysep = strchr(p, ':');
			if (!keysep) {
				continue;
			}
			*keysep = 0;
			char *key = trim(p);
			char *value = trim(keysep + 1);

			if (!strcasecmp(key, "content-length")) {
				content_length = strtoll(value, NULL, 0);
			} else if (!strcasecmp(key, "location")) {
				free(free_url);
				free_url = strdup(value);
				have_location = 1;
			}
		}

		switch (httperr) {
		case 302:
			// redirect
			if (!have_location) {
				fprintf(stderr, "redirect without new url\n");
				goto err;
			}
			url = free_url;
			continue;

		case 200:
			// success
			if (content_length < 0) {
				fprintf(stderr, "content length not specified\n");
				goto err;
			}
			free(free_url);
			*ptotal = content_length;
			os->length_remaining = content_length;
			os->last_percent_report = 0;
			return &os->iface;

		default:
			fprintf(stderr, "http error %d\n", httperr);
			goto err;
		}
	}

err:
	free(free_url);
	close_https(&os->iface);
	return NULL;
}

