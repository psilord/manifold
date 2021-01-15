#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "common.h"

void* xmalloc(unsigned long size)
{
	void *space;
	
	space = malloc(size);
	if (space == NULL)
	{
		printf("Out of memory!\n");
		exit(EXIT_FAILURE);
	}

	return space;
}

/* ensure to read n bytes from fd into ptr array */
ssize_t readn(int fd, void *vptr, size_t n)
{
	size_t nleft;
	ssize_t nread;
	char *ptr = NULL;

	ptr = vptr;
	nleft = n;

	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR) {
				nread = 0;
			} else {
				return -1;
			}
		} else if (nread == 0) {
			break;
		}

		nleft -= nread;
		ptr += nread;
	}

	return n - nleft;
}
