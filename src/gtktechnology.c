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

#include <gtktechnology.h>

#include <connman-ui-gtk.h>

struct _GtkTechnologyPrivate {
	GtkBox *box;

	GtkSwitch *enabler;
	GtkLabel *name;

	GtkCheckMenuItem *tethering;
};

static void gtk_technology_destroy(GtkWidget *widget);
static void gtk_technology_class_init(GtkTechnologyClass *klass);
static void gtk_technology_init(GtkTechnology *technology);
static void gtk_technology_dispose(GObject *object);
static gboolean gtk_technology_button_release_event(GtkWidget *widget,
							GdkEventButton *event);

G_DEFINE_TYPE(GtkTechnology, gtk_technology, GTK_TYPE_MENU_ITEM);

static void gtk_technology_class_init(GtkTechnologyClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkMenuItemClass *menu_item_class;

	object_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);
	menu_item_class = GTK_MENU_ITEM_CLASS(klass);

	object_class->dispose = gtk_technology_dispose;

	widget_class->destroy = gtk_technology_destroy;
	widget_class->button_release_event =
			gtk_technology_button_release_event;

	menu_item_class->hide_on_activate = FALSE;

	g_type_class_add_private(object_class,
					sizeof(GtkTechnologyPrivate));
}

static void gtk_technology_init(GtkTechnology *technology)
{
	GtkTechnologyPrivate *priv;

	priv =  G_TYPE_INSTANCE_GET_PRIVATE(technology,
						GTK_TYPE_TECHNOLOGY,
						GtkTechnologyPrivate);
	technology->priv = priv;

	priv->box = (GtkBox *) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	priv->enabler = (GtkSwitch *) gtk_switch_new();
	priv->name = (GtkLabel *) gtk_label_new(NULL);


	gtk_widget_set_margin_left((GtkWidget *)priv->enabler, 0);
	gtk_widget_set_margin_right((GtkWidget *)priv->enabler, 0);
	gtk_widget_set_margin_top((GtkWidget *)priv->enabler, 0);
	gtk_widget_set_margin_bottom((GtkWidget *)priv->enabler, 0);

	gtk_widget_set_margin_left((GtkWidget *)priv->name, 0);
	gtk_widget_set_margin_right((GtkWidget *)priv->name, 0);
	gtk_widget_set_margin_top((GtkWidget *)priv->name, 0);
	gtk_widget_set_margin_bottom((GtkWidget *)priv->name, 0);

	gtk_box_set_spacing(priv->box, 0);
	gtk_box_set_homogeneous(priv->box, TRUE);

	//gtk_widget_set_halign((GtkWidget *)priv->box, GTK_ALIGN_START);
	gtk_widget_set_halign((GtkWidget *)priv->name, GTK_ALIGN_START);
	//gtk_widget_set_halign((GtkWidget *)technology, GTK_ALIGN_START);

	gtk_box_pack_start(priv->box,
			(GtkWidget *)priv->enabler, FALSE, FALSE, 0);
	gtk_box_pack_start(priv->box,
			(GtkWidget *)priv->name, FALSE, FALSE, 0);

	gtk_widget_set_visible((GtkWidget *)priv->box, TRUE);
	gtk_widget_set_visible((GtkWidget *)priv->enabler, TRUE);
	gtk_widget_set_visible((GtkWidget *)priv->name, TRUE);

	gtk_container_add(GTK_CONTAINER(technology), (GtkWidget *)priv->box);

	gtk_widget_set_can_focus((GtkWidget *)priv->box, TRUE);
	gtk_widget_set_can_focus((GtkWidget *)priv->enabler, TRUE);
}

static void gtk_technology_dispose(GObject *object)
{
	(*G_OBJECT_CLASS(gtk_technology_parent_class)->dispose)(object);
}

static void gtk_technology_destroy(GtkWidget *widget)
{
	GtkTechnology *technology = GTK_TECHNOLOGY(widget);
	GtkTechnologyPrivate *priv = technology->priv;

	connman_technology_set_property_changed_callback(technology->path,
							NULL, technology);
	connman_technology_set_property_error_callback(technology->path,
							NULL, technology);

	if (priv->tethering != NULL) {
		gtk_widget_destroy((GtkWidget *)priv->tethering);
		priv->tethering = NULL;
	}

	GTK_WIDGET_CLASS(gtk_technology_parent_class)->destroy(widget);
}

static gboolean gtk_technology_button_release_event(GtkWidget *widget,
							GdkEventButton *event)
{
	GtkTechnology *technology = GTK_TECHNOLOGY(widget);
	GtkTechnologyPrivate *priv = technology->priv;
	gboolean enable;

	if (event->button != 1 && event->button != 3)
		return TRUE;

	if (event->button == 3) {
		GtkWidget *parent;
		const char *type;

		type = connman_technology_get_type(technology->path);
		if (g_strcmp0(type, "wifi") == 0)
			cui_agent_set_wifi_tethering_settings(technology->path,
									FALSE);
		parent = gtk_widget_get_parent (widget);
		if (parent != NULL && GTK_IS_MENU_SHELL(parent) == TRUE) {
			GtkMenuShell *menu_shell = GTK_MENU_SHELL(parent);
			gtk_menu_shell_activate_item(menu_shell, widget, TRUE);
		}

		return TRUE;
	}

	enable = !gtk_switch_get_active(priv->enabler);

	if (connman_technology_enable(technology->path, enable) == 0)
		gtk_widget_set_sensitive((GtkWidget *)priv->enabler, FALSE);

	return TRUE;
}

static void technology_property_error_cb(const char *path,
			const char *property, int error, void *user_data)
{
	if (error != 0)
		printf("Could not set property %s\n", property);
}

static gboolean gtk_technology_tethering_button(GtkWidget *widget,
				GdkEventButton *event, gpointer user_data)
{
	GtkTechnology *technology = user_data;
	GtkTechnologyPrivate *priv = technology->priv;
	gboolean tethering;

	if (event->button != 1)
		return TRUE;

	gtk_widget_set_sensitive((GtkWidget *)priv->tethering, FALSE);

	tethering = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), !tethering);

	connman_technology_tether(technology->path, !tethering);

	return TRUE;
}

static void set_technology_name(GtkTechnology *technology, gboolean enabled)
{
	const char *name = connman_technology_get_name(technology->path);

	if (enabled == TRUE) {
		char *markup;

		markup = g_markup_printf_escaped("<b>%s</b>", name);
		gtk_label_set_markup(technology->priv->name, markup);
		g_free(markup);
	} else
		gtk_label_set_text(technology->priv->name, name);
}

static void technology_property_changed_cb(const char *path,
					const char *property, void *user_data)
{
	GtkTechnology *technology = user_data;
	GtkTechnologyPrivate *priv = technology->priv;

	if (g_strcmp0(property, "Powered") == 0) {
		gboolean enabled;

		enabled = connman_technology_is_enabled(path);

		gtk_switch_set_active(priv->enabler, enabled);
		gtk_widget_set_sensitive((GtkWidget *)priv->enabler, TRUE);

		gtk_widget_set_sensitive((GtkWidget *)priv->tethering,
								enabled);

		set_technology_name(technology, enabled);
	} else if (g_strcmp0(property, "Tethering") == 0) {
		gtk_check_menu_item_set_active(priv->tethering,
					connman_technology_is_tethering(path));
		gtk_widget_set_sensitive((GtkWidget *)priv->tethering,
					connman_technology_is_enabled(path));
	}
}

GtkTechnology *gtk_technology_new(const gchar *path)
{
	GtkTechnology *technology;
	GtkTechnologyPrivate *priv;
	gchar *path_copy, *label;
	gboolean enabled;

	if (path == NULL)
		return NULL;

	path_copy = g_strdup(path);
	if (path_copy == NULL)
		return NULL;

	technology = g_object_new(GTK_TYPE_TECHNOLOGY, NULL);
	if (technology == NULL) {
		g_free(path_copy);
		return NULL;
	}

	priv = technology->priv;
	technology->path = path_copy;

	connman_technology_set_property_changed_callback(path_copy,
					technology_property_changed_cb,
					technology);
	connman_technology_set_property_error_callback(technology->path,
				technology_property_error_cb, technology);

	if (g_strcmp0(connman_technology_get_type(path_copy), "wifi") == 0) {
		gtk_widget_set_tooltip_text(GTK_WIDGET(technology),
				_("Left click to enable/disable\n"
				"Right click to set tethering information"));
	} else
		gtk_widget_set_tooltip_text(GTK_WIDGET(technology),
						_("Left to enable/disable"));

	enabled = connman_technology_is_enabled(path_copy);
	gtk_switch_set_active(priv->enabler, enabled);

	label = g_strdup_printf("via: %s",
				connman_technology_get_name(path_copy));

	priv->tethering = (GtkCheckMenuItem *)
				gtk_check_menu_item_new_with_label(label);
	if (enabled == FALSE)
		gtk_widget_set_sensitive((GtkWidget *)priv->tethering, FALSE);

	set_technology_name(technology, enabled);

	gtk_check_menu_item_set_active(priv->tethering,
				connman_technology_is_tethering(path_copy));

	gtk_widget_set_visible((GtkWidget *)priv->tethering, TRUE);

	g_signal_connect(priv->tethering, "button-release-event",
				G_CALLBACK(gtk_technology_tethering_button),
				technology);

	return technology;
}

GtkMenuItem *gtk_technology_get_tethering_item(GtkTechnology *technology)
{
	GtkTechnologyPrivate *priv = technology->priv;

	return GTK_MENU_ITEM(priv->tethering);
}
