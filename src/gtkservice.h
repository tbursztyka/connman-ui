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

#ifndef __GTK_SERVICE_H__
#define __GTK_SERVICE_H__

#include <gtk/gtk.h>

#include <connman-interface.h>

G_BEGIN_DECLS

#define GTK_TYPE_SERVICE \
	(gtk_service_get_type())
#define GTK_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), \
			GTK_TYPE_SERVICE, GtkService))
#define GTK_SERVICE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), \
			GTK_TYPE_SERVICE, GtkServiceClass))
#define GTK_IS_SERVICE(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_SERVICE))
#define GTK_IS_SERVICE_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_TYPE((klass), GTK_TYPE_SERVICE))
#define GTK_SERVICE_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS((obj), \
			GTK_TYPE_SERVICE, GtkServiceClass))

typedef struct _GtkService GtkService;
typedef struct _GtkServiceClass GtkServiceClass;
typedef struct _GtkServicePrivate GtkServicePrivate;

struct _GtkService {
	GtkMenuItem parent_class;

	GtkServicePrivate *priv;

	gchar *path;
};

struct _GtkServiceClass {
	GtkMenuItemClass parent_class;
};

GType gtk_service_get_type(void) G_GNUC_CONST;
GtkService *gtk_service_new(const gchar *path);

G_END_DECLS

#endif /* __GTK_SERVICE_H__ */
