/*
 * Disk Volume Header Library
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>

#include "dvhlib.h"
#include "dvh.h"

#define blksize 512

#ifdef DEBUG
# define dprintf(x...) fprintf(stderr, x)
#else
# define dprintf(x...) do { } while(0)
#endif

void __attribute__((noreturn))
die(const char *message)
{
	if (errno)
		fprintf(stderr, "%s: %s\n", message, strerror(errno));
	else
		fprintf(stderr, "%s\n", message);
	exit(EXIT_FAILURE);
}

static const char *
ptype2str(int ptype)
{
	switch (ptype) {
	case PTYPE_VOLHDR:	return "Volume Header";
	case PTYPE_TRKREPL:	return "Bad Track Replacement";
	case PTYPE_SECREPL:	return "Bad Sector Replacement";
	case PTYPE_RAW:		return "Data";
	case PTYPE_BSD:		return "BSD filesystem";
	case PTYPE_SYSV:	return "SysV filesystem";
	case PTYPE_VOLUME:	return "Volume";
	case PTYPE_EFS:		return "EFS";
	case PTYPE_LVOL:	return "Logical Volume";
	case PTYPE_RLVOL:	return "Raw Logical Volume";
	case PTYPE_XFS:		return "XFS";
	case PTYPE_XFSLOG:	return "XFS Log";
	case PTYPE_XLV:		return "XLV Volume";
	case PTYPE_XVM:		return "XVM Volume";
	case PTYPE_LSWAP:	return "Linux Swap";
	case PTYPE_LINUX:	return "Linux Native";
	}
	return "Unknown Partition Type";
}

void
dvh_print_vh(const struct dvh_handle *dvh)
{
	const struct volume_header *vh = &dvh->dvh_vh;

	printf("----- bootinfo -----\n");
	printf("Root partition: %d\n", vh->vh_rootpt);
	printf("Swap partition: %d\n", vh->vh_swappt);
	printf("Bootfile: \"%s\"\n", vh->vh_bootfile);
}

void
dvh_print_vd(const struct dvh_handle *dvh)
{
	const struct volume_header *vh = &dvh->dvh_vh;
	char name[VDNAMESIZE+1];
	int i;

	memset(name, 0, VDNAMESIZE+1);
	printf("----- directory entries -----\n");
	for (i = 0; i < NVDIR; i++) {
		const struct volume_directory *vd;

		vd = &vh->vh_vd[i];

		if (vd->vd_name[0] == '\0')
			continue;
		strncpy(name, vd->vd_name, VDNAMESIZE);
		printf("Entry #%d, name \"%s\", start %d, bytes %d\n",
		       i, name, vd->vd_lbn, vd->vd_nbytes);
	}
}

void
dvh_print_pt(const struct dvh_handle *dvh)
{
	const struct volume_header *vh = &dvh->dvh_vh;
	int i;

	printf("----- partitions -----\n");
	for (i = 0; i < NPARTAB; i++) {
		int32_t start, size, type;

		start = vh->vh_pt[i].pt_firstlbn;
		size = vh->vh_pt[i].pt_nblks;
		type = vh->vh_pt[i].pt_type;

		if (size == 0) continue;

		printf("Part# %2d, start %d, blks %d, type %s\n",
		       i, start, size, ptype2str(type));
	}
}

#define swap_short(s) swap_short_f(&(s));

static void swap_short_f(short *sp)
{
	*sp = ntohs(*sp);
}

#define swap_int(i) swap_int_f(&(i));

static void swap_int_f(int *ip)
{
	*ip = ntohl(*ip);
}

static uint32_t
twos_complement_32bit_sum(uint32_t *base, int size)
{
	int i;
	uint32_t sum = 0;

	size = size / sizeof(uint32_t);
	for (i = 0; i < size; i++)
		sum = sum - ntohl(base[i]);
	return sum;
}

static int
verify_vh(const struct dvh_handle *dvh)
{
	uint32_t csum;

	if (ntohl(dvh->dvh_vh.vh_magic) != VHMAGIC) {
		fprintf(stderr, "Bad magic\n");
		return -1;
	}

	csum = twos_complement_32bit_sum((uint32_t *)&dvh->dvh_vh,
	       	                         sizeof(struct volume_header));

	return csum != 0;
}

static void
recalc_vh_csum(struct dvh_handle *dvh)
{
	dvh->dvh_vh.vh_csum = 0;
	dvh->dvh_vh.vh_csum = htonl(twos_complement_32bit_sum(
		(uint32_t *)&dvh->dvh_vh, sizeof(struct volume_header)));
}

static void
dvh_swap_device_parameters(struct device_parameters *dp)
{
	swap_short(dp->dp_cyls);
	swap_short(dp->dp_shd0);
	swap_short(dp->dp_trks0);

	swap_short(dp->dp_secs);
	swap_short(dp->dp_secbytes);
	swap_short(dp->dp_interleave);
	
	swap_int(dp->dp_flags);
	swap_int(dp->dp_datarate);
	swap_int(dp->dp_nretries);
	swap_int(dp->dp_mspw);

	swap_short(dp->dp_xgap1);
	swap_short(dp->dp_xsync);
	swap_short(dp->dp_xrdly);
	swap_short(dp->dp_xgap2);
	swap_short(dp->dp_xrgate);
	swap_short(dp->dp_xwcont);
}

static void
dvh_swap_volume_directory(struct volume_directory *vd)
{
	swap_int(vd->vd_lbn);
	swap_int(vd->vd_nbytes);
}

static void
dvh_swap_partition_table(struct partition_table *pt)
{
	swap_int(pt->pt_nblks);
	swap_int(pt->pt_firstlbn);
	swap_int(pt->pt_type);
}

static void
dvh_swap_volume_header(struct volume_header *vh)
{
	int i;

	swap_int(vh->vh_magic);
	swap_short(vh->vh_rootpt);
	swap_short(vh->vh_swappt);

	dvh_swap_device_parameters(&vh->vh_dp);

	for (i = 0; i < NVDIR; i++)
		dvh_swap_volume_directory(vh->vh_vd + i);

	for (i = 0; i < NPARTAB; i++)
		dvh_swap_partition_table(vh->vh_pt + i);

	swap_int(vh->vh_csum);
}

struct dvh_handle *
dvh_open(const char *vh, int mode)
{
	struct dvh_handle *dvh = NULL;
	int flags, res, fd = 0;
	
	if (mode == DVH_READONLY)
		flags = O_RDONLY;
	else if (mode == DVH_READWRITE)
		flags = O_RDWR;
	else
		return NULL;
		
	dvh = malloc(sizeof(struct dvh_handle));
	if (dvh == 0)
		return NULL;

	fd = open(vh, flags);
	if (fd == -1) goto out;

	res = pread(fd, &dvh->dvh_vh, sizeof(struct volume_header), 0);
	if (res != sizeof(struct volume_header)) {
		fprintf(stderr, "dvh_open: short read.\n");
		goto out;
	}

	if (verify_vh(dvh)) {
		fprintf(stderr, "dvh_open: Bad volume header\n");
		goto out;
	}

	dvh_swap_volume_header(&dvh->dvh_vh);

	dvh->dvh_fd = fd;

	return dvh;

out:
	if (fd) close(fd);
	if (dvh) free(dvh);

	return NULL;
}

void
dvh_close(struct dvh_handle *dvh)
{
	close(dvh->dvh_fd);
	free(dvh);
}

void
dvh_vh_to_file(const struct dvh_handle *dvh, const char *vh_name,
               const char *u_name)
{
	const struct volume_header *vh = &dvh->dvh_vh;
	const struct volume_directory *vd = vh->vh_vd;
	char *buf;
	int i, res, ofd;

	for (i = 0; i < NVDIR; i++) {
		if (strncmp(vh_name, vd->vd_name, VDNAMESIZE) == 0)
			break;
		vd++;
	}

	if (i == NVDIR)
		die("Not found");

	ofd = open(u_name, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (ofd == -1)
		die("Couldn't open destination fd");

	buf = malloc(vd->vd_nbytes);
	if (buf == NULL)
		die("No memory");

	res = pread(dvh->dvh_fd, buf, vd->vd_nbytes, vd->vd_lbn * blksize);
	if (res != vd->vd_nbytes)
		die("Short read");

	res = pwrite(ofd, buf, vd->vd_nbytes, 0);
	if (res != vd->vd_nbytes) {
		unlink(u_name);
		die("Short write");
	}

	free(buf);
}

void
dvh_file_to_vh(struct dvh_handle *dvh, const char *u_name, const char *vh_name)
{
	struct volume_header *vh = &dvh->dvh_vh;
	struct volume_directory *vd = vh->vh_vd;
	struct stat istat;
	long size;
	int i, res, ifd, dest, num=0, newAdded=0;
	char *buf[NVDIR];

	ifd = open(u_name, O_RDONLY);
	if (ifd == -1)
		die("Couldn't open input fd");

	res = fstat(ifd, &istat);
	if (res == -1)
		die("Couldn't stat source file");

	/* calculate free blocks in vh */
	size = vh->vh_pt[8].pt_nblks				/* total vh size */
		- ( vh->vh_pt[8].pt_firstlbn + 4 )		/* reserved area */
		- (( istat.st_size + blksize - 1 ) / blksize );	/* pad to blocksize */
	/*
	 * Are we replacing an existing file, check for enough space and free
	 * entry in volume header
	 */
	for (i = 0; i < NVDIR; i++) {
		if (strncmp(vh_name, vd->vd_name, VDNAMESIZE) == 0) {
			/* It's an existing file, delete it.  */
			memset(vd->vd_name, 0, VDNAMESIZE);
			vd->vd_nbytes = 0;
		}
		if ( vd->vd_nbytes ) {
			size -= (vd->vd_nbytes + blksize - 1 ) / blksize; /* pad to blocksize */
			num++;
		}
		vd++;
	}

	if ( num == NVDIR )
		die("No more free entries in volume header");
	if ( size <= 0 )
		die("Not enough space left in volume header");
	
	/* copy all the other entries into a buffer */
	vd = vh->vh_vd;
	for (i = 0; i < NVDIR; i++) {
		if ( vd->vd_nbytes ) {
			buf[i] = malloc(vd->vd_nbytes);
			if (buf[i] == NULL)
				die("No memory");
			res = pread(dvh->dvh_fd, buf[i], vd->vd_nbytes,
			            vd->vd_lbn * blksize);
			dprintf("Copying entry %d to tmp buffer\n", i);
			if (res != vd->vd_nbytes)
				die("Short read");
		} else
			buf[i] = NULL;
		vd++;
	}

	/* look for a free entry and add the new one */
	vd = vh->vh_vd;
	/* XXX The number 4 is observed from the IRIX dvh.  */
	dest = vh->vh_pt[8].pt_firstlbn + 4;
	for (i = 0; i < NVDIR; i++) {
		/*
		 * add new entry if we find a free entry and we've not already
		 * done it
		 */
		if ( ! vd->vd_nbytes && ! newAdded ) { 
			dprintf("Adding new entry at position %d\n", i);
			vd->vd_nbytes = istat.st_size; 
			strncpy(vd->vd_name, vh_name, VDNAMESIZE);
			newAdded = 1;

			buf[i] = malloc(vd->vd_nbytes);
			if (buf[i] == NULL)
				die("No memory");

			res = pread(ifd, buf[i], vd->vd_nbytes, 0);
			if (res != vd->vd_nbytes) {
				die("Short read");
			}
			close(ifd);
		}
		if ( vd->vd_nbytes ) { /* write buf to Volume Directory */
			dprintf("Writing buf[%d]\n", i);
			vd->vd_lbn = dest;
			res = pwrite(dvh->dvh_fd, buf[i], vd->vd_nbytes,
			             vd->vd_lbn * blksize);
			if (res != vd->vd_nbytes) {
				fprintf(stderr, "Wrote %d not %d\n", res,
				        vd->vd_nbytes);
				die("Short write");
			}
		}
		dest += (vd->vd_nbytes + blksize - 1) / blksize;
		vd++;
	}

	for ( i = 0; i < NVDIR; i++) {
		if( buf[i] )
			free( buf[i] );
	}

	/* write the new volume header too! */
	dvh_swap_volume_header(vh);
	recalc_vh_csum(dvh);

	dprintf("About to rewrite the volume header: ");
	res = pwrite(dvh->dvh_fd, vh, sizeof(struct volume_header), 0);
	if ( res != sizeof(struct volume_header )) {
		die("Short write of volume header - bye bye\n");
	}
	dprintf("wrote %d bytes\n", res);
}

void
dvh_vh_remove(struct dvh_handle *dvh, const char *vh_name)
{
	struct volume_header *vh = &dvh->dvh_vh;
	struct volume_directory *vd = vh->vh_vd;
	int i, res, dest, found = 0;
	char *buf[NVDIR];

	/* copy all the other entries into a buffer */
	vd = vh->vh_vd;
	for (i = 0; i < NVDIR; i++) {
		if (strncmp(vh_name, vd->vd_name, VDNAMESIZE) == 0) {
			memset(vd->vd_name, 0, VDNAMESIZE);
			vd->vd_nbytes = 0;
			found = 1;
		}
		if ( vd->vd_nbytes ) {
			buf[i] = malloc(vd->vd_nbytes);
			if (buf[i] == NULL)
				die("No memory");
			res = pread(dvh->dvh_fd, buf[i], vd->vd_nbytes,
			            vd->vd_lbn * blksize);
			dprintf("Copying entry %d to tmp buffer\n", i);
			if (res != vd->vd_nbytes)
				die("Short read");
		} else
			buf[i] = NULL;
		vd++;
	}

	if ( ! found )
		die("Not found");

	vd = vh->vh_vd;
	/* XXX The number 4 is observed from the IRIX dvh.  */
	dest = vh->vh_pt[8].pt_firstlbn + 4;
	for (i = 0; i < NVDIR; i++) {
		if ( vd->vd_nbytes ) { /* write buf to Volume Directory */
			dprintf("Writing buf[%d]\n", i);
			vd->vd_lbn = dest;
			res = pwrite(dvh->dvh_fd, buf[i], vd->vd_nbytes,
			             vd->vd_lbn * blksize);
			if (res != vd->vd_nbytes) {
				fprintf(stderr, "Wrote %d not %d\n", res,
				        vd->vd_nbytes);
				die("Short write");
			}
			free( buf[i] );
		}
		dest += (vd->vd_nbytes + 511) / 512;	/* XXX Blocksize  */
		vd++;
	}

	/* write the new volume header too! */
	dvh_swap_volume_header(vh);
	recalc_vh_csum(dvh);

	dprintf("About to rewrite the volume header: ");
	res = pwrite(dvh->dvh_fd, vh, sizeof(struct volume_header), 0);
	if ( res != sizeof(struct volume_header )) {
		die("Short write of volume header - bye bye\n");
	}
	dprintf("wrote %d bytes\n", res);
}
