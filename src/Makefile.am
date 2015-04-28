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
	ipcam-itrain.c \
	ipcam-itrain.h \
	ipcam-proto-interface.h \
	ipcam-itrain-server.c \
	ipcam-itrain-server.h \
	ipcam-itrain-message.c \
	ipcam-itrain-message.h \
	ipcam-itrain-event-handler.c \
	ipcam-itrain-event-handler.h \
	ipcam-dctx-proto-handler.c \
	ipcam-dctx-proto-handler.h \
	ipcam-dttx-proto-handler.c \
	ipcam-dttx-proto-handler.h

itrain_LDFLAGS = 

itrain_LDADD = $(ITRAIN_LIBS) 

SUBDIRS = \
	config
