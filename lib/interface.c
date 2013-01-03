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

#include <gdbus/gdbus.h>

#include <connman-private.h>

DBusConnection *dbus_cnx_session = NULL;
struct connman_interface *connman = NULL;

static void connman_watch_interface_connected(DBusConnection *dbus_cnx,
							void *user_data)
{
	if (connman == NULL || connman->interface_connected_cb == NULL)
		return;

	connman->interface_connected_cb(connman->user_data);
}

static void connman_watch_interface_disconnected(DBusConnection *dbus_cnx,
							void *user_data)
{
	if (connman == NULL || connman->interface_disconnected_cb == NULL)
		return;

	connman->interface_disconnected_cb(connman->user_data);
}

int connman_interface_init(connman_interface_cb_f interface_connected_cb,
			connman_interface_cb_f interface_disconnected_cb,
			void *user_data)
{
	DBusConnection *dbus_cnx;
	DBusError error;

	if (connman != NULL)
		return 0;

	if (interface_connected_cb == NULL ||
			interface_disconnected_cb == NULL)
		return -EINVAL;

	dbus_cnx = g_dbus_setup_bus(DBUS_BUS_SYSTEM, NULL, NULL);
	if (dbus_cnx == NULL)
		return -ENOMEM;

	dbus_cnx_session = g_dbus_setup_bus(DBUS_BUS_SESSION, NULL, NULL);
	if (dbus_cnx_session == NULL) {
		dbus_connection_unref(dbus_cnx);
		return -ENOMEM;
	}

	dbus_error_init(&error);
	if (g_dbus_request_name(dbus_cnx_session,
				"net.connman.ConnmanUI", &error) == FALSE) {
		if (dbus_error_is_set(&error) == TRUE) {
			printf("Error: %s\n", error.message);
			dbus_error_free(&error);
		}

		dbus_connection_unref(dbus_cnx);
		dbus_connection_unref(dbus_cnx_session);

		return -EALREADY;
	}

	connman = g_try_malloc0(sizeof(struct connman_interface));
	if (connman == NULL) {
		dbus_connection_unref(dbus_cnx);
		dbus_connection_unref(dbus_cnx_session);

		return -ENOMEM;
	}

	connman->dbus_cnx = dbus_cnx;

	connman->interface_connected_cb = interface_connected_cb;
	connman->interface_disconnected_cb = interface_disconnected_cb;

	connman->interface_watch_id = g_dbus_add_service_watch(dbus_cnx,
					CONNMAN_DBUS_NAME,
					connman_watch_interface_connected,
					connman_watch_interface_disconnected,
					NULL, NULL);
	if (connman->interface_watch_id == 0) {
		dbus_connection_unref(dbus_cnx);
		dbus_connection_unref(dbus_cnx_session);
		g_free(connman);

		return -EINVAL;
	}

	/* We don't know yet if connman is present */
	connman_watch_interface_disconnected(dbus_cnx, NULL);

	connman->user_data = user_data;

	return 0;
}

void connman_interface_finalize(void)
{
	if (connman == NULL)
		return;

	g_dbus_remove_all_watches(connman->dbus_cnx);

	dbus_connection_unref(connman->dbus_cnx);

	if (dbus_cnx_session != NULL)
		dbus_connection_unref(dbus_cnx_session);

	dbus_cnx_session = NULL;

	g_free(connman);

	connman = NULL;
}
