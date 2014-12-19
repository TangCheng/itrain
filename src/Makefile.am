## Process this file with automake to produce Makefile.in

## Created by Anjuta

AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(ITRAIN_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bindir = $(prefix)/itrain
bin_PROGRAMS = itrain

itrain_SOURCES = \
	main.c \
	itrain.c \
	ipcam-itrain-server.c \
	ipcam-itrain-server.h \
	ipcam-itrain-connection.c \
	ipcam-itrain-connection.h \
	ipcam-itrain-message.c \
	ipcam-itrain-message.h \
	ipcam-proto-handler.c \
	ipcam-proto-handler.h

itrain_LDFLAGS = 

itrain_LDADD = $(ITRAIN_LIBS) 

SUBDIRS = \
	config
