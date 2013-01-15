SUBDIRS = po

AM_MAKEFLAGS = --no-print-directory

includedir = @includedir@

include_HEADERS =

noinst_HEADERS =

local_headers = $(foreach file,$(include_HEADERS) $(nodist_include_HEADERS) \
			$(noinst_HEADERS), include/$(notdir $(file)))

gdbus_sources = gdbus/gdbus.h gdbus/mainloop.c gdbus/watch.c \
					gdbus/object.c gdbus/polkit.c

common_sources = lib/connman-interface.h lib/interface.c \
			lib/dbus.c lib/manager.c lib/technology.c \
			lib/service.c lib/agent.c

if MAINTAINER_MODE
uidir = $(abs_top_srcdir)/data/ui
iconsdir = $(abs_top_srcdir)/data/icons
localdir = $(abs_top_srcdir)/po
else
uidir = $(datadir)/connman_ui_gtk/ui
iconsdir = $(datadir)/connman_ui_gtk/icons
endif

ui_DATA = data/ui/agent.ui data/ui/left_menu.ui data/ui/right_menu.ui \
					data/ui/settings.ui data/ui/tray.ui

icons_DATA = data/icons/bad_signal.png data/icons/cellular.png \
		data/icons/ethernet.png data/icons/good_signal.png \
		data/icons/low_signal.png data/icons/medium_signal.png \
		data/icons/network_disconnected.png \
		data/icons/network_online.png data/icons/network_ready.png \
		data/icons/tethering.png

AM_CFLAGS = @GLIB_CFLAGS@ @DBUS_CFLAGS@ \
	-DCUI_UI_PATH=\""$(uidir)"\" \
	-DCUI_ICON_PATH=\"$(iconsdir)\" \
	-DLOCALEDIR=\"$(localedir)\"

bin_PROGRAMS = src/connman-ui-gtk
src_connman_ui_gtk_SOURCES = $(gdbus_sources) $(common_sources) \
			src/main.c src/tray.c \
			src/left-menu.c src/right-menu.c \
			src/gtktechnology.h src/gtktechnology.c \
			src/gtkservice.h src/gtkservice.c \
			src/agent_dialogs.c src/settings.c \
			src/theme.c src/utils.c
src_connman_ui_gtk_CFLAGS = $(AM_CFLAGS) @GTK_CFLAGS@
src_connman_ui_gtk_LDADD = @GLIB_LIBS@ @DBUS_LIBS@ @GTK_LIBS@

AM_CPPFLAGS = -I$(builddir)/include -I$(builddir)/src \
				-I$(srcdir)/gdbus -I$(srcdir)/lib

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = m4/ChangeLog $(ui_DATA) $(icons_DATA)
