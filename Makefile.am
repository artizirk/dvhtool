## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = foreign

man_MANS = dvhtool.8
EXTRA_DIST = CONTRIBUTORS $(man_MANS)
noinst_HEADERS = getopt.h

sbin_PROGRAMS = dvhtool
dvhtool_SOURCES = dvhtool.c dvhlib.c dvhlib.h dvh.h
dvhtool_LDADD = @LIBOBJS@
