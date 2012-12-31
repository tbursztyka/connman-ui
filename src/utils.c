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

GtkLabel *set_label(GtkBuilder *builder, const char *name,
				const char *value, const char *default_value)
{
	GtkLabel *label;

	label = (GtkLabel *) gtk_builder_get_object(builder, name);
	if (label == NULL)
		return NULL;

	if (value == NULL)
		value = default_value;

	gtk_label_set_text(label, value);

	return label;
}

GtkEntry *set_entry(GtkBuilder *builder, const char *name,
				const char *value, const char *default_value)
{
	GtkEntry *entry;

	entry = (GtkEntry *) gtk_builder_get_object(builder, name);
	if (entry == NULL)
		return NULL;

	if (value == NULL)
		value = default_value;

	gtk_entry_set_text(entry, value);

	return entry;
}

GtkWidget *set_widget_sensitive(GtkBuilder *builder,
					const char *name, gboolean value)
{
	GtkWidget *widget;

	widget = (GtkWidget *) gtk_builder_get_object(builder, name);
	if (widget == NULL)
		return NULL;

	gtk_widget_set_sensitive(widget, value);

	return widget;
}

GtkWidget *set_widget_hidden(GtkBuilder *builder,
					const char *name, gboolean value)
{
	GtkWidget *widget;

	widget = (GtkWidget *) gtk_builder_get_object(builder, name);
	if (widget == NULL)
		return NULL;

	gtk_widget_set_visible(widget, value);

	if (value == TRUE)
		gtk_widget_hide(widget);
	else
		gtk_widget_show(widget);

	return widget;
}

GtkWidget *set_button_toggle(GtkBuilder *builder,
					const char *name, gboolean active)
{
	GtkWidget *widget;

	widget = (GtkWidget *) gtk_builder_get_object(builder, name);
	if (widget == NULL)
		return NULL;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), active);

	return widget;
}

GtkImage *set_image(GtkBuilder *builder,
			const char *name, GdkPixbuf *image, const char *info)
{
	GtkImage *img_widget;

	img_widget = (GtkImage *) gtk_builder_get_object(builder, name);
	if (img_widget == NULL)
		return NULL;

	if (image != NULL) {
		gtk_widget_set_visible(GTK_WIDGET(img_widget), TRUE);
		gtk_widget_set_tooltip_text(GTK_WIDGET(img_widget), info);

		gtk_image_set_from_pixbuf(img_widget, image);
	} else
		gtk_widget_set_visible(GTK_WIDGET(img_widget), FALSE);

	return img_widget;
}

void set_signal_callback(GtkBuilder *builder,
				const char *name, const char *signal_name,
				GCallback handler, gpointer user_data)
{
	GtkWidget *widget;

	widget = (GtkWidget *) gtk_builder_get_object(builder, name);
	if (widget == NULL)
		return;

	g_signal_connect(widget, signal_name, handler, user_data);
}

const char *get_entry_text(GtkBuilder *builder, const char *name)
{
	GtkEntry *entry;

	entry = (GtkEntry *) gtk_builder_get_object(builder, name);
	if (entry == NULL)
		return NULL;

	return gtk_entry_get_text(entry);
}
