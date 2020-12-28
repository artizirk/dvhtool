#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include "getopt.h"
#endif

#include "dvh.h"
#include "dvhlib.h"

#if defined(__linux__) && defined(__MIPSEB__) && !defined (DEBUG)
#define DEVICE "/dev/sda"
#else
#if defined(__NetBSD__) && defined(__MIPSEB__)
#define DEVICE "/dev/sd0k"
#else
#define DEVICE "volhdr-1.dat"
#endif
#endif

#define DVH_READONLY	1
#define DVH_READWRITE	2

#define OPT_DEVICE 'd'
#define OPT_PRINT_VH 256
#define OPT_PRINT_VD 257
#define OPT_PRINT_PT 258
#define OPT_PRINT_ALL 259
#define OPT_VH_REMOVE 260
#define OPT_VH_TO_UNIX 261
#define OPT_UNIX_TO_VH 262
#define OPT_HELP 263

static struct option long_options[] = {
	{"device", required_argument, NULL, OPT_DEVICE},
	{"print-volume-header", no_argument, NULL, OPT_PRINT_VH},
	{"print-volume-directory", no_argument, NULL, OPT_PRINT_VD},
	{"print-partitions", no_argument, NULL, OPT_PRINT_PT},
	{"print-all", no_argument, NULL, OPT_PRINT_ALL},
	{"vh-remove", no_argument, NULL, OPT_VH_REMOVE},
	{"vh-to-unix", no_argument, NULL, OPT_VH_TO_UNIX},
	{"unix-to-vh", no_argument, NULL, OPT_UNIX_TO_VH},
	{"help", no_argument, NULL, OPT_HELP},
	{NULL, 0, NULL, 0}
};

enum actions {
	no_action,
	print_vh,
	print_vd,
	print_pt,
	print_all,
	vh_remove,
	vh_to_unix,
	unix_to_vh,
	show_usage
};

int
main(int argc, char *argv[])
{
	void *dvh;
	char *device = DEVICE;
	int c;
	enum actions action = show_usage;

	while(1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "d:", long_options, &option_index);


		if (c == -1)
			break;

		switch(c) {
		case OPT_DEVICE:
			device = optarg;
			break;

		case OPT_PRINT_VH:
			action = print_vh;
			break;

		case OPT_PRINT_VD:
			action = print_vd;
			break;

		case OPT_PRINT_PT:
			action = print_pt;
			break;

		case OPT_PRINT_ALL:
			action = print_all;
			break;

		case OPT_VH_REMOVE:
			action = vh_remove;
			break;

		case OPT_VH_TO_UNIX:
			action = vh_to_unix;
			break;

		case OPT_UNIX_TO_VH:
			action = unix_to_vh;
			break;

		case OPT_HELP:
		default:
			action = show_usage;
			break;
		}
	}

	if (action == show_usage) {
		printf (
"Usage: %s --device DEVNAME [OPTIONS]\n"
"Manipulate the volume header of DEVNAME\n\n"
"Options:\n\n"
"--print-volume-header		Show header data only\n"
"--print-volume-directory	Show volume table of contents\n"
"--print-partitions		Show partition data\n"
"--print-all			Equivalent to all three above options\n"
"--vh-remove NAME		Remove volhdr file NAME from volhdr\n"
"--vh-to-unix NAME FILE		Copy volhdr file NAME to Unix file FILE\n"
"--unix-to-vh FILE NAME		Copy Unix file FILE to volhdr as NAME\n"
"--help				Show usage information\n",
		argv[0]);
		exit (EXIT_SUCCESS);
	}

	dvh = dvh_open(device, DVH_READONLY);
	if (dvh == NULL)
		die("Can't open Disk Volume Header");

	if (action == print_vh)
		dvh_print_vh(dvh);
	if (action == print_vd)
		dvh_print_vd(dvh);
	if (action == print_pt)
		dvh_print_pt(dvh);
	if (action == print_all) {
		dvh_print_pt(dvh);
		dvh_print_vh(dvh);
		dvh_print_vd(dvh);
	}
	if (action == vh_remove) {
		char *vh_file;

		if (optind + 1 > argc)
			die("Missing argument");
		vh_file = argv[optind];

		/* close the dvh and reopen rw */
		dvh_close(dvh);
		dvh = dvh_open(device, DVH_READWRITE);
		if (dvh == NULL)
			die("Can't reopen Disk Volume Header rw");
		
		dvh_vh_remove(dvh, vh_file);
	}
	if (action == vh_to_unix) {
		char *vh_file, *u_file;

		if (optind + 2 > argc)
			die("Missing arguments");

		vh_file = argv[optind++];
		u_file = argv[optind];
		dvh_vh_to_file(dvh, vh_file, u_file);
	}
	if (action == unix_to_vh) {
		char *vh_file, *u_file;

		if (optind + 2 > argc)
			die("Missing arguments");

		u_file = argv[optind++];
		vh_file = argv[optind];

		/* close the dvh and reopen rw */
		dvh_close(dvh);
		dvh = dvh_open(device, DVH_READWRITE);
		if (dvh == NULL)
			die("Can't reopen Disk Volume Header rw");
		
		dvh_file_to_vh(dvh, u_file, vh_file);
	}


	dvh_close(dvh);

	exit(EXIT_SUCCESS);
}
