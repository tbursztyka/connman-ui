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
#include <gtkservice.h>

#define CUI_LEFT_MENU_UI_PATH CUI_UI_PATH "/left_menu.ui"

static GtkMenu *cui_left_menu = NULL;
static GtkMenu *cui_more_menu = NULL;
static GtkMenuItem *cui_list_more_item = NULL;
static GHashTable *service_items = NULL;
static GtkMenuItem *cui_scan_spinner = NULL;

static void add_or_update_service(const char *path, int position)
{
	GtkService *s;

	s = gtk_service_new(path);

	if (position > 9)
		gtk_menu_shell_append(GTK_MENU_SHELL(cui_more_menu),
							(GtkWidget *)s);
	else
		gtk_menu_shell_insert(GTK_MENU_SHELL(cui_left_menu),
						(GtkWidget *)s, position);

	gtk_widget_set_visible((GtkWidget *)s, TRUE);
	gtk_widget_show((GtkWidget *)s);

	g_hash_table_insert(service_items, s->path, s);
}

static void remove_service_cb(const char *path)
{
	g_hash_table_remove(service_items, path);
}

static void get_services_cb(void *user_data)
{
	GSList *services, *list;
	int item_position = 2;

	services = connman_service_get_services();
	if (services == NULL)
		return;

	gtk_widget_hide(GTK_WIDGET(cui_list_more_item));

	for (list = services; list; list = list->next, item_position++)
		add_or_update_service(list->data, item_position);

	if (item_position > 10)
		gtk_widget_show(GTK_WIDGET(cui_list_more_item));

	g_slist_free(services);
}

static void scanning_cb(void *user_data)
{
	GtkSpinner *spin;

	spin = (GtkSpinner *)gtk_bin_get_child(GTK_BIN(cui_scan_spinner));

	gtk_spinner_stop(spin);
	gtk_widget_hide((GtkWidget *)cui_scan_spinner);
	gtk_widget_hide((GtkWidget *)spin);
}

static void delete_service_item(gpointer data)
{
	GtkWidget *item = data;

	gtk_widget_destroy(item);
}

static void cui_popup_left_menu(GtkStatusIcon *trayicon,
						gpointer user_data)
{
	GtkSpinner *spin;

	spin = (GtkSpinner *)gtk_bin_get_child(GTK_BIN(cui_scan_spinner));

	gtk_widget_hide(GTK_WIDGET(cui_list_more_item));
	gtk_widget_show((GtkWidget *)cui_scan_spinner);
	gtk_widget_show((GtkWidget *)spin);

	gtk_spinner_start(spin);

	connman_service_set_removed_callback(remove_service_cb);

	connman_service_refresh_services_list(get_services_cb,
							scanning_cb, user_data);

	gtk_menu_popup(cui_left_menu, NULL, NULL, NULL, NULL, 1, 0);
}

static void cui_popdown_left_menu(GtkMenu *menu, gpointer user_data)
{
	connman_service_set_removed_callback(NULL);
	g_hash_table_remove_all(service_items);
	connman_service_free_services_list();
}

gint cui_load_left_menu(GtkBuilder *builder, GtkStatusIcon *trayicon)
{
	GError *error = NULL;

	gtk_builder_add_from_file(builder, CUI_LEFT_MENU_UI_PATH, &error);
	if (error != NULL) {
		printf("Error: %s\n", error->message);
		g_error_free(error);

		return -EINVAL;
	}

	cui_left_menu = (GtkMenu *) gtk_builder_get_object(builder,
							"cui_left_menu");
	cui_more_menu = (GtkMenu *) gtk_builder_get_object(builder,
							"cui_more_menu");
	cui_list_more_item = (GtkMenuItem *) gtk_builder_get_object(builder,
							"cui_list_more_item");
	cui_scan_spinner = (GtkMenuItem *) gtk_builder_get_object(builder,
							"cui_scan_spinner");

	gtk_container_add(GTK_CONTAINER(cui_scan_spinner), gtk_spinner_new());

	g_signal_connect(cui_left_menu, "deactivate",
				G_CALLBACK(cui_popdown_left_menu), NULL);

	service_items = g_hash_table_new_full(g_str_hash, g_str_equal,
						NULL, delete_service_item);

	cui_tray_hook_left_menu(cui_popup_left_menu);

	return 0;
}
