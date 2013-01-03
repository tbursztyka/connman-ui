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

struct _GtkServicePrivate {
	GtkBox *box;

	GtkLabel *name;

	GtkImage *state;
	GtkImage *security;
	GtkImage *signal;

	gboolean selected;
};

static void gtk_service_destroy(GtkWidget *widget);
static void gtk_service_class_init(GtkServiceClass *klass);
static void gtk_service_init(GtkService *service);
static void gtk_service_dispose(GObject *object);
static gboolean gtk_service_button_release_event(GtkWidget *widget,
							GdkEventButton *event);

G_DEFINE_TYPE(GtkService, gtk_service, GTK_TYPE_MENU_ITEM);

static void gtk_service_class_init(GtkServiceClass *klass)
{
	GObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkMenuItemClass *menu_item_class;

	object_class = G_OBJECT_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);
	menu_item_class = GTK_MENU_ITEM_CLASS(klass);

	object_class->dispose = gtk_service_dispose;

	widget_class->destroy = gtk_service_destroy;
	widget_class->button_release_event =
			gtk_service_button_release_event;

	menu_item_class->hide_on_activate = FALSE;

	g_type_class_add_private(object_class,
					sizeof(GtkServicePrivate));
}

static void gtk_service_init(GtkService *service)
{
	GtkServicePrivate *priv;

	priv =  G_TYPE_INSTANCE_GET_PRIVATE(service,
						GTK_TYPE_SERVICE,
						GtkServicePrivate);
	service->priv = priv;

	priv->box = (GtkBox *) gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

	priv->name = (GtkLabel *) gtk_label_new(NULL);
	priv->state = (GtkImage *) gtk_image_new();
	priv->security = (GtkImage *) gtk_image_new();
	priv->signal = (GtkImage *) gtk_image_new();

	priv->selected = FALSE;

	//gtk_widget_set_halign((GtkWidget *)priv->box, GTK_ALIGN_START);
	gtk_widget_set_halign((GtkWidget *)priv->name, GTK_ALIGN_START);

	gtk_widget_set_halign((GtkWidget *)priv->state, GTK_ALIGN_END);
	gtk_widget_set_halign((GtkWidget *)priv->security, GTK_ALIGN_END);
	gtk_widget_set_halign((GtkWidget *)priv->signal, GTK_ALIGN_END);

	gtk_box_set_homogeneous(priv->box, FALSE);

	gtk_box_pack_start(priv->box,
			(GtkWidget *)priv->name, TRUE, TRUE, 0);

	gtk_box_pack_start(priv->box,
			(GtkWidget *) priv->state, TRUE, TRUE, 0);
	gtk_box_pack_start(priv->box,
			(GtkWidget *) priv->security, TRUE, TRUE, 0);
	gtk_box_pack_start(priv->box,
			(GtkWidget *) priv->signal, TRUE, TRUE, 0);

	gtk_widget_set_visible((GtkWidget *)priv->box, TRUE);
	gtk_widget_set_visible((GtkWidget *)priv->name, TRUE);

	gtk_widget_set_visible((GtkWidget *)priv->state, FALSE);
	gtk_widget_set_visible((GtkWidget *)priv->security, FALSE);
	gtk_widget_set_visible((GtkWidget *)priv->signal, FALSE);

	gtk_container_add(GTK_CONTAINER(service), (GtkWidget *)priv->box);

	gtk_widget_set_can_focus((GtkWidget *)priv->box, TRUE);
}

static void gtk_service_dispose(GObject *object)
{
	(*G_OBJECT_CLASS(gtk_service_parent_class)->dispose)(object);
}

static void gtk_service_destroy(GtkWidget *widget)
{
	GtkService *service = GTK_SERVICE(widget);
	GtkServicePrivate *priv = service->priv;

	if (priv != NULL && priv->selected == FALSE) {

		connman_service_set_property_changed_callback(service->path,
								NULL, service);
		connman_technology_set_property_error_callback(service->path,
								NULL, service);
	}

	GTK_WIDGET_CLASS(gtk_service_parent_class)->destroy(widget);
}

static gboolean gtk_service_button_release_event(GtkWidget *widget,
							GdkEventButton *event)
{
	GtkService *service = GTK_SERVICE(widget);
	GtkWidget *parent;

	if (event->button != 1 && event->button != 3)
		goto activate;

	if (event->button == 1) {
		if (connman_service_is_connected(service->path) == TRUE)
			connman_service_disconnect(service->path);
		else {
			cui_agent_set_selected_service(service->path,
				connman_service_get_name(service->path));
			connman_service_connect(service->path);
		}
	} else if (event->button == 3) {
		service->priv->selected = TRUE;

		cui_settings_popup(service->path);
	}

activate:
	parent = gtk_widget_get_parent (widget);
	if (parent != NULL && GTK_IS_MENU_SHELL(parent) == TRUE) {
		GtkMenuShell *menu_shell = GTK_MENU_SHELL(parent);
		gtk_menu_shell_activate_item(menu_shell, widget, TRUE);
	}

	return TRUE;
}

static void service_property_changed_cb(const char *path,
					const char *property, void *user_data)
{
	return;
}

static void service_set_state(GtkService *service)
{
	GtkServicePrivate *priv = service->priv;
	const struct connman_ipv4 *ipv4;
	enum connman_state state;
	GdkPixbuf *image = NULL;
	const char *ip = NULL;
	const char *info;

	if (connman_service_is_connected(service->path) == FALSE) {
		gtk_widget_set_tooltip_text((GtkWidget *)priv->name, "");
		return;
	}

	ipv4 = connman_service_get_ipv4(service->path);
	if (ipv4 == NULL) {
		const struct connman_ipv6 *ipv6;

		ipv6 = connman_service_get_ipv6(service->path);
		if (ipv6 != NULL)
			ip = ipv6->address;
	} else
		ip = ipv4->address;

	if (ip == NULL)
		ip = "";

	gtk_widget_set_tooltip_text((GtkWidget *)priv->name, ip);

	state = connman_service_get_state(service->path);
	cui_theme_get_state_icone_and_info(state, &image, &info);

	if (image == NULL)
		return;

	gtk_widget_set_visible((GtkWidget *)priv->state, TRUE);
	gtk_widget_set_tooltip_text((GtkWidget *)priv->state, info);
	gtk_image_set_from_pixbuf(priv->state, image);
}

static void service_set_signal(GtkService *service)
{
	GtkServicePrivate *priv = service->priv;
	GdkPixbuf *image = NULL;
	const char *type, *info;

	type = connman_service_get_type(service->path);

	if (g_strcmp0(type, "wifi") == 0) {
		uint8_t strength;

		strength = connman_service_get_strength(service->path);

		cui_theme_get_signal_icone_and_info(strength, &image, &info);
	} else
		cui_theme_get_type_icone_and_info(type, &image, &info);

	if (image == NULL)
		return;

	gtk_widget_set_visible((GtkWidget *)priv->signal, TRUE);
	gtk_widget_set_tooltip_text((GtkWidget *)priv->signal, info);
	gtk_image_set_from_pixbuf(priv->signal, image);
}

static void service_set_name(GtkService *service)
{
	const char *name;
	char *markup;

	name = connman_service_get_name(service->path);
	if (name == NULL)
		name = "- Hidden -";

	if (connman_service_is_favorite(service->path) == TRUE) {
		if (g_strcmp0(connman_service_get_type(service->path),
							"wifi") == 0) {
			markup = g_markup_printf_escaped(
				"<b>%s</b> <i> (%s) </i>",
				name,
				connman_service_get_security(service->path));
		} else
			markup = g_markup_printf_escaped("<b>%s</b>", name);
	} else {
		if (g_strcmp0(connman_service_get_type(service->path),
							"wifi") == 0) {
			markup = g_markup_printf_escaped(
				"%s  <i> (%s) </i>", name,
				connman_service_get_security(service->path));
		} else
			markup = g_markup_printf_escaped("%s", name);
	}

	gtk_label_set_markup(service->priv->name, markup);

	g_free(markup);
}

GtkService *gtk_service_new(const char *path)
{
	GtkService *service;
	char *path_copy;

	if (path == NULL)
		return NULL;

	path_copy = g_strdup(path);
	if (path_copy == NULL)
		return NULL;

	service = g_object_new(GTK_TYPE_SERVICE, NULL);
	if (service == NULL) {
		g_free(path_copy);
		return NULL;
	}

	service->path = path_copy;

	connman_service_set_property_changed_callback(path_copy,
					service_property_changed_cb,
					service);


	service_set_name(service);
	service_set_state(service);
	service_set_signal(service);

	return service;
}


