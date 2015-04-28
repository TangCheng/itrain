dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(itrain, 1.0.0)

dnl AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AC_PROG_CC


AC_ARG_WITH(project,
    AS_HELP_STRING([--with-project=PROJECT],
                   [project name [dctx/dttx]]),
    [project=$withval],
    [project=dctx])

AM_CONDITIONAL(WITH_DCTX_PROJECT, test $project = dctx)
AM_CONDITIONAL(WITH_DTTX_PROJECT, test $project = dttx)

case $project in
  dctx)
    AC_DEFINE_UNQUOTED([WITH_DCTX_PROJECT], [1], [DCTX Project])
    ;;
  dttx)
    AC_DEFINE_UNQUOTED([WITH_DTTX_PROJECT], [1], [DTTX Project])
    ;;
esac


PKG_CHECK_MODULES(ITRAIN, [zlib libzmq libczmq libffi gobject-2.0 gmodule-2.0 json-glib-1.0 gio-2.0 yaml-0.1 libipcam_base-0.1.0])


AC_OUTPUT([
Makefile
src/Makefile
src/config/Makefile

])
