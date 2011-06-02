# $Id: Makefile.am 3136 2009-10-11 16:52:33Z chris $

lib_LTLIBRARIES = libdvdid.la

AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST = \
	dvdid.def \
	dvdid.dll.vcproj \
	dvdid.lib.vcproj

libdvdid_la_LDFLAGS = -version-info 1:0:1
libdvdid_la_SOURCES = \
	dvdid.c \
	crc64_table.c \
	crc64_table.h
