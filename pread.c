#include "config.h"

#ifndef HAVE_PREAD

#include <sys/types.h>
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
		return -1;
	return read(fd, buf, nbytes);
}

extern ssize_t pwrite(int fd, const void *buf, size_t n, off_t offset)
{
	if (lseek(fd, offset, SEEK_SET) < 0)
		return -1;
	return write(fd, buf, n);
}

#endif
