#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/uio.h>

#define _CONSTRUCTOR __attribute__((constructor))
#define _DESTRUCTOR __attribute__ ((destructor))

#define HOOK_LOG "hooklog.bin"

struct hook_ctx {
	int logfd;
	int (*SSL_read)(void *ssl, void *buf, int num);
	int (*SSL_write)(void *ssl, const void *buf, int num);
	int (*SSL_get_rfd)(void *ssl);
	int (*SSL_get_wfd)(void *ssl);
};

static struct hook_ctx _ctx;

#define LOADORDIE(var, name) \
	do {\
		const char *err; \
		(var) = dlsym(RTLD_NEXT, (name)); \
		if ((err = dlerror()) != NULL) { \
			fprintf(stderr, "dlsym %s: %s\n", (name), err); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)

void _CONSTRUCTOR hook_init(void) {
	_ctx.logfd = open(HOOK_LOG, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if (_ctx.logfd < 0) {
		fprintf(stderr, "unable to create " HOOK_LOG "\n");
		exit(EXIT_FAILURE);
	}

	dlerror();
	LOADORDIE(_ctx.SSL_read, "SSL_read");
	LOADORDIE(_ctx.SSL_write, "SSL_write");
	LOADORDIE(_ctx.SSL_get_rfd, "SSL_get_rfd");
	LOADORDIE(_ctx.SSL_get_wfd, "SSL_get_wfd");
}

void _DESTRUCTOR hook_fini(void) {
	close(_ctx.logfd);
}

#define LOG_READ 0
#define LOG_WRITE 1

static inline void log_or_die(uint8_t type, int fd, void *buf, int num) {
	struct sockaddr saddr, paddr;
	socklen_t saddrlen = sizeof(struct sockaddr), paddrlen = sizeof(struct sockaddr);
	struct iovec vecs[8];
	size_t rlen;

	if (getsockname(fd, &saddr, &saddrlen) < 0) {
		fprintf(stderr, "getsockname error\n");
		exit(EXIT_FAILURE);
	}

	if (getpeername(fd, &paddr, &paddrlen) < 0) {
		fprintf(stderr, "getpeername error\n");
		exit(EXIT_FAILURE);
	}

	rlen =	sizeof(rlen) + sizeof(uint8_t) +
			sizeof(saddrlen) + sizeof(saddr) +
			sizeof(paddrlen) + sizeof(paddr) +
			sizeof(num) + num;

	vecs[0].iov_base = &rlen;
	vecs[0].iov_len = sizeof(rlen);
	vecs[1].iov_base = &type;
	vecs[1].iov_len = sizeof(type);
	vecs[2].iov_base = &saddrlen;
	vecs[2].iov_len = sizeof(saddrlen);
	vecs[3].iov_base = &saddr;
	vecs[3].iov_len = sizeof(saddr);
	vecs[4].iov_base = &paddrlen;
	vecs[4].iov_len = sizeof(paddrlen);
	vecs[5].iov_base = &paddr;
	vecs[5].iov_len = sizeof(paddr);
	vecs[6].iov_base = &num;
	vecs[6].iov_len = sizeof(num);
	vecs[7].iov_base = buf;
	vecs[7].iov_len = num;
	if (writev(_ctx.logfd, vecs, sizeof(vecs)/sizeof(struct iovec)) != rlen) {
		fprintf(stderr, "writev error\n");
		exit(EXIT_FAILURE);
	}
}

int SSL_read(void *ssl, void *buf, int num) {
	int fd, ret;

	ret = _ctx.SSL_read(ssl, buf, num);
	if (ssl != NULL && buf != NULL && ret > 0) {
		fd = _ctx.SSL_get_rfd(ssl);
		if (fd < 0) {
			fprintf(stderr, "SSL_get_rfd error\n");
			exit(EXIT_FAILURE);
		}

		log_or_die(LOG_READ, fd, buf, ret);
	}

	return ret;
}

int SSL_write(void *ssl, void *buf, int num) {
	int fd;

	if (ssl != NULL && buf != NULL && num > 0) {
		fd = _ctx.SSL_get_wfd(ssl);
		if (fd < 0) {
			fprintf(stderr, "SSL_get_wfd error\n");
			exit(EXIT_FAILURE);
		}

		log_or_die(LOG_WRITE, fd, buf, num);
	}

	return _ctx.SSL_write(ssl, buf, num);
}
