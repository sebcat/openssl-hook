#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <netinet/in.h>

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

// returns -1 on error, 0 on success
static int record_print(struct record *r, FILE *out) {
	// TODO: Implement
	return 0;
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
		perror("fopen");
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
