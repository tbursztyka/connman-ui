AC_PREREQ(2.60)
AC_INIT(connman-ui, 0.1)

AM_INIT_AUTOMAKE([foreign subdir-objects color-tests])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr/local)

PKG_PROG_PKG_CONFIG

AC_SUBST(abs_top_srcdir)
AC_SUBST(abs_top_builddir)

AC_LANG_C

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MKDIR_P

m4_define([_LT_AC_TAGCONFIG], [])
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

IT_PROG_INTLTOOL

GETTEXT_PACKAGE=connman-ui
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [The domain to use with gettext])
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE(optimization, AC_HELP_STRING([--disable-optimization],
			[disable code optimization through compiler]), [
	if (test "${enableval}" = "no"); then
		CFLAGS="$CFLAGS -O0"
	fi
])

AC_ARG_ENABLE(debug, AC_HELP_STRING([--enable-debug],
			[enable compiling with debugging information]), [
	if (test "${enableval}" = "yes" &&
				test "${ac_cv_prog_cc_g}" = "yes"); then
		CFLAGS="$CFLAGS -g -Wall"
	fi
])

PKG_CHECK_MODULES(GTK, gtk+-3.0 >= 3.0, dummy=yes,
				AC_MSG_ERROR(gtk+ >= 3.0 is required))
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.28, dummy=yes,
				AC_MSG_ERROR(GLib >= 2.28 is required))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.4, dummy=yes,
				AC_MSG_ERROR(D-Bus >= 1.4 is required))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

AC_OUTPUT(Makefile
	include/version.h
	po/Makefile.in)

