#include "config.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#else
typedef unsigned int uint32_t;		/* A guess ...  */
#endif

#include "dvh.h"

#ifndef HAVE___ATTRIBUTE__
#define __attribute__(x)
#endif

#ifndef HAVE_PREAD

extern ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset);
extern ssize_t pwrite(int fd, __const void *buf, size_t n, off_t offset);

#endif

/*
 * Disk Volume Header Library
 */
struct dvh_handle {
	int dvh_fd;
	union {
		struct volume_header vh;
		uint32_t cs[sizeof(struct volume_header) / sizeof(uint32_t)];
	} dvh_vc;
};

#define dvh_vh dvh_vc.vh
#define dvh_cs dvh_vc.cs

#define DVH_READONLY 1
#define DVH_READWRITE 2

extern struct dvh_handle * dvh_open(const char *vh, int mode);
void extern dvh_close(struct dvh_handle *dvh);
extern void dvh_vh_remove(struct dvh_handle *dvh, const char *vh_name);
extern void dvh_vh_to_file(const struct dvh_handle *dvh, const char *vh_name,
                           const char *u_name);
extern void dvh_file_to_vh(struct dvh_handle *dvh, const char *u_name,
                           const char *dvh_name);
extern void dvh_print_vh(const struct dvh_handle *vh);
extern void dvh_print_vd(const struct dvh_handle *vh);
extern void dvh_print_pt(const struct dvh_handle *vh);
extern void __attribute__((noreturn)) die(const char *message);
