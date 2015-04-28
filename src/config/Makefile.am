## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(ICONFIG_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g


conf_datadir = $(prefix)/itrain/config
conf_data_DATA = \
	app.yml

PROTOCOL ?= DCTX

if WITH_DCTX_PROJECT
override PROTOCOL = DCTX
endif

if WITH_DTTX_PROJECT
override PROTOCOL = DTTX
endif

install-data-hook:
	sed -i -r "/^itrain:$$/,/^.*:$$/ s/protocol:.*/protocol: $(PROTOCOL)/g" \
		$(DESTDIR)$(prefix)/itrain/config/app.yml
