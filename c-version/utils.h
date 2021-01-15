#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>

/* bail if I can't get the memory */
void* xmalloc(unsigned long size);

/* read all n bytes unless there is a short read due to EOF. return 0 on EOF */
ssize_t readn(int fd, void *vptr, size_t n);

#endif
