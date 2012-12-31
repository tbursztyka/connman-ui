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
#include <gtktechnology.h>

#define CUI_RIGHT_MENU_UI_PATH CUI_UI_PATH "/right_menu.ui"

static GtkMenu *cui_right_menu = NULL;
static GtkWidget *cui_item_mode_off = NULL;
static GtkWidget *cui_item_mode_on = NULL;
static GtkMenu *cui_tethering_menu = NULL;
static GHashTable *tech_items = NULL;
static int item_position = 3;
static gboolean disabled = TRUE;

static void add_technology(const char *path)
{
	GtkTechnology *t;

	t = gtk_technology_new(path);
	gtk_menu_shell_insert(GTK_MENU_SHELL(cui_right_menu),
					(GtkWidget *)t, item_position);
	item_position++;

	gtk_widget_set_visible((GtkWidget *)t, TRUE);
	gtk_widget_show((GtkWidget *)t);

	gtk_menu_shell_append(GTK_MENU_SHELL(cui_tethering_menu),
			(GtkWidget *)gtk_technology_get_tethering_item(t));

	g_hash_table_insert(tech_items, t->path, t);
}

static void technology_added_cb(const char *path)
{
	add_technology(path);
}

static void technology_removed_cb(const char *path)
{
	g_hash_table_remove(tech_items, path);
}

static void delete_technology_item(gpointer data)
{
	GtkWidget *item = data;

	gtk_widget_destroy(item);
	item_position--;
}

static void cui_item_offlinemode_activate(GtkMenuItem *menuitem,
						gpointer user_data)
{
	gboolean offlinemode;

	offlinemode = !connman_manager_get_offlinemode();

	connman_manager_set_offlinemode(offlinemode);
}

static void cui_item_quit_activate(GtkMenuItem *menuitem,
						gpointer user_data)
{
	gtk_main_quit();
}

static void cui_popup_right_menu(GtkStatusIcon *trayicon,
						guint button,
						guint activate_time,
						gpointer user_data)
{
	GList *tech_list, *list;

	if (disabled == TRUE)
		goto popup;

	if (connman_manager_get_offlinemode()) {
		gtk_widget_show(cui_item_mode_on);
		gtk_widget_hide(cui_item_mode_off);
	} else {
		gtk_widget_show(cui_item_mode_off);
		gtk_widget_hide(cui_item_mode_on);
	}

	tech_list = connman_technology_get_technologies();
	if (tech_list == NULL)
		goto popup;

	for (list = tech_list; list != NULL; list = list->next)
		add_technology((const char *)list->data);

	g_list_free(tech_list);

popup:
	connman_technology_set_removed_callback(technology_removed_cb);
	connman_technology_set_added_callback(technology_added_cb);

	gtk_menu_popup(cui_right_menu, NULL, NULL,
				NULL, NULL, button, activate_time);
}

static void cui_popdown_right_menu(GtkMenu *menu, gpointer user_data)
{
	connman_technology_set_removed_callback(NULL);
	connman_technology_set_added_callback(NULL);

	g_hash_table_remove_all(tech_items);
}

static void cui_enable_item(gboolean enable)
{
	set_widget_hidden(cui_builder, "cui_item_offlinemode_on", enable);
	set_widget_hidden(cui_builder, "cui_item_offlinemode_off", enable);
	set_widget_hidden(cui_builder, "cui_sep_technology_up", enable);
	set_widget_hidden(cui_builder, "cui_sep_technology_down", enable);
	set_widget_hidden(cui_builder, "cui_item_tethering", enable);
	set_widget_hidden(cui_builder, "cui_sep_tethering", enable);
	set_widget_hidden(cui_builder, "cui_item_quit", FALSE);

	disabled = enable;
}

void cui_right_menu_enable_only_quit(void)
{
	cui_enable_item(TRUE);
}

void cui_right_menu_enable_all(void)
{
	cui_enable_item(FALSE);
}

gint cui_load_right_menu(GtkBuilder *builder, GtkStatusIcon *trayicon)
{
	GtkImageMenuItem *cui_item_tethering;
	GtkMenuItem *cui_item_quit;
	GdkPixbuf *image = NULL;
	GError *error = NULL;

	gtk_builder_add_from_file(builder, CUI_RIGHT_MENU_UI_PATH, &error);
	if (error != NULL) {
		printf("Error: %s\n", error->message);
		g_error_free(error);

		return -EINVAL;
	}

	cui_builder = builder;

	cui_right_menu = (GtkMenu *) gtk_builder_get_object(builder,
							"cui_right_menu");
	cui_item_quit = (GtkMenuItem *) gtk_builder_get_object(builder,
							"cui_item_quit");
	cui_item_mode_off = (GtkWidget *) gtk_builder_get_object(builder,
						"cui_item_offlinemode_off");
	cui_item_mode_on = (GtkWidget *) gtk_builder_get_object(builder,
						"cui_item_offlinemode_on");
	cui_tethering_menu = (GtkMenu *) gtk_builder_get_object(builder,
						"cui_tethering_menu");

	g_signal_connect(cui_right_menu, "deactivate",
				G_CALLBACK(cui_popdown_right_menu), NULL);
	g_signal_connect(cui_item_quit, "activate",
				G_CALLBACK(cui_item_quit_activate), NULL);
	g_signal_connect(cui_item_mode_off, "activate",
			G_CALLBACK(cui_item_offlinemode_activate), NULL);
	g_signal_connect(cui_item_mode_on, "activate",
			G_CALLBACK(cui_item_offlinemode_activate), NULL);

	tech_items = g_hash_table_new_full(g_str_hash, g_str_equal,
						NULL, delete_technology_item);

	cui_item_tethering = (GtkImageMenuItem *) gtk_builder_get_object(
						builder, "cui_item_tethering");

	cui_theme_get_tethering_icone_and_info(&image, NULL);
	if (image != NULL)
		gtk_image_menu_item_set_image(cui_item_tethering,
					gtk_image_new_from_pixbuf(image));

	cui_tray_hook_right_menu(cui_popup_right_menu);

	return 0;
}
