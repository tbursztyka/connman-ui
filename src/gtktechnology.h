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

#ifndef __GTK_TECHNOLOGY_H__
#define __GTK_TECHNOLOGY_H__

#include <gtk/gtk.h>

#include <connman-interface.h>

G_BEGIN_DECLS

#define GTK_TYPE_TECHNOLOGY \
	(gtk_technology_get_type())
#define GTK_TECHNOLOGY(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
			GTK_TYPE_TECHNOLOGY, GtkTechnology))
#define GTK_TECHNOLOGY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), \
			GTK_TYPE_TECHNOLOGY, GtkTechnologyClass))
#define GTK_IS_TECHNOLOGY(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_TECHNOLOGY))
#define GTK_IS_TECHNOLOGY_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_TECHNOLOGY))
#define GTK_TECHNOLOGY_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), \
			GTK_TYPE_TECHNOLOGY, GtkTechnologyClass))

typedef struct _GtkTechnology GtkTechnology;
typedef struct _GtkTechnologyClass GtkTechnologyClass;
typedef struct _GtkTechnologyPrivate GtkTechnologyPrivate;

struct _GtkTechnology {
	GtkMenuItem parent_class;

	GtkTechnologyPrivate *priv;

	gchar *path;
};

struct _GtkTechnologyClass {
	GtkMenuItemClass parent_class;
};

GType gtk_technology_get_type(void) G_GNUC_CONST;
GtkTechnology *gtk_technology_new(const gchar *path);

GtkMenuItem *gtk_technology_get_tethering_item(GtkTechnology *technology);

G_END_DECLS

#endif /* __GTK_TECHNOLOGY_H__ */

