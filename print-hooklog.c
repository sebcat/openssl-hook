#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>

struct record {
	size_t recsz;
	uint8_t type;
	socklen_t saddrlen;
	struct sockaddr saddr;
	socklen_t paddrlen;
	struct sockaddr paddr;
	int num;
	char *buf;
};

#define RTYPE_READ 0
#define RTYPE_WRITE 1

// returns -1 on error, 0 on eof, 1 on read record
static int record_read(struct record *r, FILE *in) {
	if (fread(&r->recsz, sizeof(r->recsz), 1, in) != 1) {
		if (feof(in)) {
			return 0;
		}  else {
			return -1;
		}
	}

	if (fread(&r->type, sizeof(r->type), 1, in) != 1) {
		return -1;
	}

	if (fread(&r->saddrlen, sizeof(r->saddrlen), 1, in) != 1) {
		return -1;
	}

	if (fread(&r->saddr, r->saddrlen, 1, in) != 1) {
		return -1;
	}

	if (fread(&r->paddrlen, sizeof(r->paddrlen), 1, in) != 1) {
		return -1;
	}

	if (fread(&r->paddr, r->paddrlen, 1, in) != 1) {
		return -1;
	}

	if (fread(&r->num, sizeof(r->num), 1, in) != 1) {
		return -1;
	}

	if (r->num <= 0) {
		r->buf = NULL;
	} else {
		r->buf = malloc(r->num);
		if (r->buf == NULL) {
			return -1;
		}

		if (fread(r->buf, r->num, 1, in) != 1) {
			return -1;
		}
	}

	return 1;
}

static void record_clean(struct record *r) {
	if (r->buf != NULL) {
		free(r->buf);
		r->buf = NULL;
	}
}

#define ISPRINT(c) \
	(isascii((c)) && (c) != '\r' && (c) != '\n')

static void record_print(struct record *r, FILE *out) {
	char addr[128], port[32], peeraddr[128], peerport[32];
	int i, j, nrowbytes;

	if ((getnameinfo(&r->saddr, r->saddrlen, addr, sizeof(addr),
			port, sizeof(port), NI_NUMERICHOST|NI_NUMERICSERV) < 0) ||
			(getnameinfo(&r->paddr, r->paddrlen, peeraddr, sizeof(peeraddr),
			peerport, sizeof(peerport), NI_NUMERICHOST|NI_NUMERICSERV) < 0)) {
		fprintf(out, "<invalid addr record>\n\n");
		return;
	}

	if (r->type == RTYPE_READ) {
		fprintf(out, "read  - %s %s -> %s %s\n", peeraddr, peerport, addr, port);
	} else if (r->type == RTYPE_WRITE) {
		fprintf(out, "write - %s %s -> %s %s\n", addr, port, peeraddr, peerport);
	} else {
		fprintf(out, "unknown record type (%hhu) local: %s:%s peer: %s:%s\n",
				r->type, addr, port, peeraddr, peerport);
	}

	i=0;
	while (i < r->num) {
		fprintf(out, "%08x  ", i);
		nrowbytes = r->num-i;
		if (nrowbytes > 16) {
			nrowbytes = 16;
		}

		for(j=0; j<nrowbytes; j++) {
			fprintf(out, "%02x ", (int)r->buf[i+j]);
			if (j == 7) {
				fprintf(out, " ");
			}
		}

		for(j=nrowbytes; j < 16; j++) {
			fprintf(out, "   ");
		}

		fprintf(out, " |");
		for(j=0; j < nrowbytes; j++) {
			fprintf(out, "%c", ISPRINT(r->buf[i+j]) ? r->buf[i+j] : '.');
		}

		fprintf(out, "|\n");
		i+=nrowbytes;
	}

	fprintf(out, "%08x\n\n", r->num);
}

int main(int argc, char *argv[]) {
	FILE *fp;
	struct record r;
	int ret;

	if (argc != 2) {
		fprintf(stderr, "print-hooklog <dump-file>\n");
		return EXIT_FAILURE;
	}

	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		perror(argv[1]);
		return EXIT_FAILURE;
	}

	while ((ret = record_read(&r, fp)) > 0) {
		record_print(&r, stdout);
		record_clean(&r);
	}

	if (ret < 0) {
		perror("record_read");
		fclose(fp);
		exit(EXIT_FAILURE);
	}

	fclose(fp);
	return EXIT_SUCCESS;
}
