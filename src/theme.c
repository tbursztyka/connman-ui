/*
 *
 *  Connection Manager UI
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <connman-ui-gtk.h>

static GtkIconTheme *icon_theme = NULL;

void cui_theme_get_tethering_icone_and_info(GdkPixbuf **image,
							const char **info)
{
	if (image != NULL)
		*image = gtk_icon_theme_load_icon(icon_theme,
					"nm-adhoc", 24, 0, NULL);
	if (info != NULL)
		*info = _("Tethering");
}

void cui_theme_get_type_icone_and_info(const char *type,
					GdkPixbuf **image, const char **info)
{
	const char *nfo = NULL;
	GdkPixbuf *img = NULL;

	if (g_strcmp0(type, "ethernet") == 0) {
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-wired-symbolic", 22, 0, NULL);
		nfo = _("Ethernet");
	} else if (g_strcmp0(type, "cellular") == 0) {
		img = gtk_icon_theme_load_icon(icon_theme,
					"gsm-3g-full", 22, 0, NULL);
		nfo = _("Cellular");
	}

	if (image != NULL)
		*image = img;

	if (info != NULL)
		*info = nfo;
}

void cui_theme_get_signal_icone_and_info(uint8_t signal_strength,
					GdkPixbuf **image, const char **info)
{
	const char *nfo;
	GdkPixbuf *img;

	if (signal_strength >= 80) {
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-wireless-signal-excellent-symbolic", 22, 0, NULL);
		nfo = _("Very good signal");
	} else if (signal_strength >= 60 && signal_strength < 80) {
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-wireless-signal-good-symbolic", 22, 0, NULL);
		nfo = _("Good signal");
	} else if (signal_strength >= 40 && signal_strength < 60) {
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-wireless-signal-ok-symbolic", 22, 0, NULL);
		nfo = _("Low signal");
	} else {
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-wireless-signal-weak-symbolic", 22, 0, NULL);
		nfo = _("Very low signal");
	}

	if (image != NULL)
		*image = img;

	if (info != NULL)
		*info = nfo;

}

void cui_theme_get_state_icone_and_info(enum connman_state state,
					GdkPixbuf **image, const char **info)
{
	const char *nfo = NULL;
	GdkPixbuf *img = NULL;


	switch (state) {
	case CONNMAN_STATE_UNKNOWN:
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-offline-symbolic", 24, 0, NULL);
		nfo = _("Connman is not running");
		break;
	case CONNMAN_STATE_READY:
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-idle-symbolic", 24, 0, NULL);
		nfo = _("Connected");
		break;
	case CONNMAN_STATE_ONLINE:
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-transmit-receive-symbolic", 24, 0, NULL);
		nfo = _("Online");
		break;
	default:
		img = gtk_icon_theme_load_icon(icon_theme,
					"network-offline-symbolic", 24, 0, NULL);
		nfo = _("Disconnected");

		break;
	}

	if (image != NULL)
		*image = img;

	if (info != NULL)
		*info = nfo;
}

void cui_load_theme(void)
{
	icon_theme = gtk_icon_theme_get_default ();
	gtk_icon_theme_append_search_path(icon_theme, CUI_ICON_PATH);
}
