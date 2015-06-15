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

	/* Reposition left menu after updating the list */
	gtk_menu_reposition(cui_left_menu);
}

static void accumulate_menu_size(GtkWidget* widget, gpointer data)
{
	GtkRequisition *menu_size = (GtkRequisition *)data;
	GtkRequisition item_size;
	gtk_widget_get_preferred_size(widget, NULL, &item_size);
	menu_size->width = MAX(item_size.width, menu_size->width);
	menu_size->height += item_size.height;
}

static void menu_position_func(GtkMenu *menu, gint *out_x, gint *out_y,
		gboolean *push_in, gpointer user_data)
{
	/* Placement logic comes from GTK+'s gtk_menu_position() in gtkmenu.c */
	GtkWidget *widget = GTK_WIDGET(menu);
	GtkStatusIcon *trayicon = GTK_STATUS_ICON(user_data);
	GtkRequisition requisition;
	gint x, y;
	gint space_left, space_right, space_above, space_below;
	gint needed_width, needed_height, monitor_num;
	GdkScreen *screen;
	GdkRectangle area, monitor;
	GtkBorder padding, margin;
	GtkStyleContext *context;
	GtkStateFlags state;
	gboolean rtl = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

	/* Always offset from tray icon */
	if (gtk_status_icon_get_geometry(trayicon, NULL, &area, NULL))
	{
		x = area.x + area.width / 2;
		y = area.y + area.height / 2;
	}
	else
	{
		x = 0;
		y = 0;
	}

	/* Resize menu */
	requisition.width = 0;
	requisition.height = 0;
	gtk_container_foreach(GTK_CONTAINER(cui_left_menu),
			accumulate_menu_size, &requisition);
	gtk_widget_set_size_request(GTK_WIDGET(cui_left_menu),
			requisition.width, requisition.height);

	/* Start actual layout */
	context = gtk_widget_get_style_context(widget);
	state = gtk_widget_get_state_flags(widget);
	gtk_style_context_get_padding(context, state, &padding);
	gtk_style_context_get_margin(context, state, &margin);

	screen = gtk_widget_get_screen(widget);
	monitor_num = gdk_screen_get_monitor_at_point(screen, x, y);
	gdk_screen_get_monitor_workarea(screen, monitor_num, &monitor);

	space_left = x - monitor.x;
	space_right = monitor.x + monitor.width - x - 1;
	space_above = y - monitor.y;
	space_below = monitor.y + monitor.height - y - 1;

	/* Position horizontally. */
	needed_width = requisition.width - padding.left;
	if (needed_width <= space_left ||
			needed_width <= space_right)
	{
		if ((rtl  && needed_width <= space_left) ||
				(!rtl && needed_width >  space_right))
			x = x - margin.left + padding.left - requisition.width + 1;
		else
			x = x + margin.right - padding.right;
	}
	else if (requisition.width <= monitor.width)
	{
		if (space_left > space_right)
			x = monitor.x;
		else
			x = monitor.x + monitor.width - requisition.width;
	}
	else
	{
		if (rtl)
			x = monitor.x + monitor.width - requisition.width;
		else
			x = monitor.x;
	}

	/* Position vertically */
	needed_height = requisition.height - padding.top;
	if (needed_height <= space_above ||
			needed_height <= space_below)
	{
		if (needed_height <= space_below)
			y = y + margin.top - padding.top;
		else
			y = y - margin.bottom + padding.bottom - requisition.height + 1;
		y = CLAMP(y, monitor.y,
				monitor.y + monitor.height - requisition.height);
	}
	else if (needed_height > space_below && needed_height > space_above)
	{
		if (space_below >= space_above)
			y = monitor.y + monitor.height - requisition.height;
		else
			y = monitor.y;
	}
	else
	{
		y = monitor.y;
	}

	*out_x = x;
	*out_y = y;
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

	/* Reposition left menu after updating the list */
	gtk_menu_reposition(cui_left_menu);
}

static void scanning_cb(void *user_data)
{
	GtkSpinner *spin;

	spin = (GtkSpinner *)gtk_bin_get_child(GTK_BIN(cui_scan_spinner));

	gtk_spinner_stop(spin);
	gtk_widget_hide((GtkWidget *)cui_scan_spinner);
	gtk_widget_hide((GtkWidget *)spin);

	/* Reposition left menu after hidding the spinner */
	gtk_menu_reposition(cui_left_menu);
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

	gtk_menu_popup(cui_left_menu, NULL, NULL,
			menu_position_func, trayicon, 1, 0);
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
