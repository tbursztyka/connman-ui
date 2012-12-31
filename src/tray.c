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

#define CUI_TRAYICON_UI_PATH CUI_UI_PATH "/tray.ui"

static GtkStatusIcon *cui_trayicon = NULL;
static void (*popup_left_menu_f)(GtkStatusIcon *, gpointer);
static void (*popup_rigt_menu_f)(GtkStatusIcon *, guint, guint, gpointer);
static int left_menu_handler_id = 0;
static int right_menu_handler_id = 0;

void cui_trayicon_update_icon(void)
{
	enum connman_state state;
	GdkPixbuf *image = NULL;
	const char *info = NULL;

	state = connman_manager_get_state();

	cui_theme_get_state_icone_and_info(state, &image, &info);

	gtk_status_icon_set_from_pixbuf(cui_trayicon, image);

	gtk_status_icon_set_tooltip_text(cui_trayicon, info);
	gtk_status_icon_set_visible(cui_trayicon, TRUE);
}

void cui_tray_hook_left_menu(gpointer callback)
{
	popup_left_menu_f = callback;
}

void cui_tray_hook_right_menu(gpointer callback)
{
	popup_rigt_menu_f = callback;
}

void cui_tray_left_menu_disable(void)
{
	if (left_menu_handler_id != 0) {
		g_signal_handler_disconnect(cui_trayicon, left_menu_handler_id);
		left_menu_handler_id = 0;
	}
}

void cui_tray_enable(void)
{
	if (left_menu_handler_id == 0)
		left_menu_handler_id = g_signal_connect(cui_trayicon,
			"activate", G_CALLBACK(popup_left_menu_f), NULL);
	if (right_menu_handler_id == 0)
		right_menu_handler_id = g_signal_connect(cui_trayicon,
			"popup-menu", G_CALLBACK(popup_rigt_menu_f), NULL);
}

void cui_tray_disable(void)
{
	if (left_menu_handler_id != 0)
		g_signal_handler_disconnect(cui_trayicon,
					left_menu_handler_id);
	if (right_menu_handler_id != 0)
		g_signal_handler_disconnect(cui_trayicon,
					right_menu_handler_id);

	left_menu_handler_id = 0;
	right_menu_handler_id = 0;
}

gint cui_load_trayicon(GtkBuilder *builder)
{
	GError *error = NULL;

	gtk_builder_add_from_file(builder, CUI_TRAYICON_UI_PATH, &error);
	if (error != NULL) {
		printf("Error: %s\n", error->message);
		g_error_free(error);

		return -EINVAL;
	}

	cui_trayicon = (GtkStatusIcon *) gtk_builder_get_object(builder,
							"cui_trayicon");

	cui_trayicon_update_icon();

	cui_load_left_menu(builder, cui_trayicon);
	cui_load_right_menu(builder, cui_trayicon);

	return 0;
}
