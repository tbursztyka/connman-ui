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

#ifndef __CONNMAN_PRIVATE_H__
#define __CONNMAN_PRIVATE_H__

#include <connman-interface.h>

#define CONNMAN_DBUS_NAME "net.connman"
#define CONNMAN_ERROR CONNMAN_DBUS_NAME ".Error"

struct connman_interface {
	DBusConnection *dbus_cnx;
	guint interface_watch_id;

	connman_interface_cb_f interface_connected_cb;
	connman_interface_cb_f interface_disconnected_cb;

	void *user_data;
};

struct property_change {
	int index;

	const char *path;
	const char *name;

	int error;
};

struct property_setting {
	void *data;
	int index;
};

extern struct connman_interface *connman;

typedef void (*connman_manager_get_technologies_cb_f)(DBusMessageIter *iter);
typedef void (*connman_manager_technology_added_cb_f)(DBusMessageIter *iter);
typedef void (*connman_manager_technology_removed_cb_f)(DBusMessageIter *iter);

typedef void (*connman_manager_get_services_cb_f)(DBusMessageIter *iter);
typedef void (*connman_manager_service_changed_cb)(DBusMessageIter *iter);

enum connman_state string2enum_state(const char *state);

int __connman_manager_get_technologies(connman_manager_get_technologies_cb_f cb);

int __connman_manager_register_technology_signals(connman_manager_technology_added_cb_f added_cb,
			connman_manager_technology_removed_cb_f removed_cb);

int __connman_manager_get_services(connman_manager_get_services_cb_f cb);

int __connman_manager_register_service_signal(connman_manager_service_changed_cb cb);

#endif /* __CONNMAN_PRIVATE_H__ */
