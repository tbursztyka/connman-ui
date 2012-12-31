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

#ifndef __CONNMAN_UI_GTK_H__
#define __CONNMAN_UI_GTK_H__

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <locale.h>
#include <libintl.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <connman-interface.h>

struct cui_selected_service {
	char *path;
	char *name;
};

extern GtkBuilder *cui_builder;

void cui_load_theme(void);
void cui_theme_get_type_icone_and_info(const char *type,
					GdkPixbuf **image, const char **info);
void cui_theme_get_signal_icone_and_info(uint8_t signal_strength,
					GdkPixbuf **image, const char **info);
void cui_theme_get_state_icone_and_info(enum connman_state state,
					GdkPixbuf **image, const char **info);
void cui_theme_get_tethering_icone_and_info(GdkPixbuf **image,
							const char **info);

gint cui_load_trayicon(GtkBuilder *builder);
void cui_trayicon_update_icon(void);
void cui_tray_hook_left_menu(gpointer callback);
void cui_tray_hook_right_menu(gpointer callback);
void cui_tray_left_menu_disable(void);
void cui_tray_enable(void);
void cui_tray_disable(void);

gint cui_load_agent_dialogs(void);
void cui_agent_init_callbacks(void);
void cui_agent_set_selected_service(const char *path, const char *name);
void cui_agent_set_wifi_tethering_settings(const char *path, gboolean tether);
gint cui_load_left_menu(GtkBuilder *builder, GtkStatusIcon *trayicon);
gint cui_load_right_menu(GtkBuilder *builder, GtkStatusIcon *trayicon);
void cui_right_menu_enable_only_quit(void);
void cui_right_menu_enable_all(void);

gint cui_settings_popup(const char *path);

GtkLabel *set_label(GtkBuilder *builder, const char *name,
				const char *value, const char *default_value);
GtkEntry *set_entry(GtkBuilder *builder, const char *name,
				const char *value, const char *default_value);
GtkWidget *set_widget_sensitive(GtkBuilder *builder,
					const char *name, gboolean value);
GtkWidget *set_widget_hidden(GtkBuilder *builder,
					const char *name, gboolean value);
GtkWidget *set_button_toggle(GtkBuilder *builder,
					const char *name, gboolean active);
GtkImage *set_image(GtkBuilder *builder,
			const char *name, GdkPixbuf *image, const char *info);
void set_signal_callback(GtkBuilder *builder,
				const char *name, const char *signal_name,
				GCallback handler, gpointer user_data);
const char *get_entry_text(GtkBuilder *builder, const char *name);

#endif /* __CONNMAN_UI_GTK_H__ */
