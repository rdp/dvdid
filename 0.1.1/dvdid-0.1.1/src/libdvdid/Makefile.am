# $Id: Makefile.am 1742 2009-03-21 19:56:28Z chris $

lib_LTLIBRARIES = libdvdid.la

AM_CPPFLAGS = -I$(top_srcdir)/include

EXTRA_DIST = \
	dvdid.def \
	dvdid.dll.vcproj \
	dvdid.lib.vcproj

libdvdid_la_LDFLAGS = -version-info 0:0:0
libdvdid_la_SOURCES = \
	dvdid.c \
	crc64_table.c \
	crc64_table.h
