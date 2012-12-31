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
#include <cui-dbus.h>

#define CONNMAN_MANAGER_PATH "/"
#define CONNMAN_MANAGER_INTERFACE CONNMAN_DBUS_NAME ".Manager"

#define PROPERTY(n) Manager_updatable_properties[n]

enum connman_manage_property {
	MANAGER_STATE       = 0,
	MANAGER_OFFLINEMODE = 1,
	MANAGER_MAX         = 2,
};

static const char *Manager_updatable_properties[] = {
	"State",
	"OfflineMode",
};

struct connman_manager {
	DBusConnection *dbus_cnx;

	DBusPendingCall *get_properties_call;
	guint property_changed_wid;

	connman_property_changed_cb_f property_changed_cb;
	void *property_user_data;

	enum connman_state state;
	gboolean offlinemode;

	guint source[MANAGER_MAX];

	/* Technology part */
	DBusPendingCall *get_technologies_call;
	guint technology_added_wid;
	guint technology_removed_wid;

	connman_manager_get_technologies_cb_f get_technologies_cb;
	connman_manager_technology_added_cb_f technology_added_cb;
	connman_manager_technology_removed_cb_f technology_removed_cb;

	/* Service part */
	DBusPendingCall *get_services_call;
	guint services_changed_wid;

	connman_manager_get_services_cb_f get_services_cb;
	connman_manager_service_changed_cb services_changed_cb;
};

static struct connman_manager *manager = NULL;

static void destroy_property(gpointer user_data)
{
	struct property_change *property = user_data;

	if (property == NULL)
		return;

	if (manager != NULL)
		manager->source[property->index] = 0;

	g_free(property);
}

static gboolean property_changed(gpointer user_data)
{
	struct property_change *property = user_data;

	if (manager == NULL || manager->property_changed_cb == NULL)
		return FALSE;

	manager->property_changed_cb(property->path, property->name,
						manager->property_user_data);

	return FALSE;
}

enum connman_state string2enum_state(const char *state)
{
	if (g_strcmp0(state, "offline") == 0)
		return CONNMAN_STATE_OFFLINE;
	else if (g_strcmp0(state, "idle") == 0)
		return CONNMAN_STATE_IDLE;
	else if (g_strcmp0(state, "ready") == 0)
		return CONNMAN_STATE_READY;
	else if (g_strcmp0(state, "online") == 0)
		return CONNMAN_STATE_ONLINE;
	else if (g_strcmp0(state, "failure") == 0)
		return CONNMAN_STATE_FAILURE;

	return CONNMAN_STATE_OFFLINE;
}

static void update_manager_property(int index)
{
	struct property_change *property;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		return;

	if (manager->source[index] != 0)
		g_source_remove(manager->source[index]);

	property->index = index;
	property->name = PROPERTY(index);

	manager->source[index] = g_timeout_add_full(G_PRIORITY_DEFAULT, 0,
				property_changed, property, destroy_property);
}

static void get_properties_callback(DBusPendingCall *pending, void *user_data)
{
	dbus_bool_t offlinemode;
	DBusMessageIter arg;
	DBusMessage *reply;
	const char *state;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	manager->get_properties_call = NULL;

	reply = dbus_pending_call_steal_reply(pending);
	if (reply == NULL)
		goto error;

	if (dbus_message_iter_init(reply, &arg) == FALSE)
		goto error;

	if (cui_dbus_get_dict_entry_basic(&arg,
				PROPERTY(MANAGER_STATE),
				DBUS_TYPE_STRING, &state) == 0) {
		manager->state = string2enum_state(state);
		update_manager_property(MANAGER_STATE);
	}

	if (cui_dbus_get_dict_entry_basic(&arg,
				PROPERTY(MANAGER_OFFLINEMODE),
				DBUS_TYPE_BOOLEAN, &offlinemode) == 0) {
		manager->offlinemode = offlinemode;
		update_manager_property(MANAGER_OFFLINEMODE);
	}

error:
	if (reply != NULL)
		dbus_message_unref(reply);

	dbus_pending_call_unref(pending);
}

static gboolean property_changed_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	DBusMessageIter arg;
	const char *name;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_STRING, &name) != 0)
		return TRUE;

	dbus_message_iter_next(&arg);

	if (g_strcmp0(name, PROPERTY(MANAGER_STATE)) == 0) {
		const char *state;

		if (cui_dbus_get_basic_variant(&arg,
					DBUS_TYPE_STRING, &state) == 0) {
			manager->state = string2enum_state(state);
			update_manager_property(MANAGER_STATE);
		}
	} else if (g_strcmp0(name, PROPERTY(MANAGER_OFFLINEMODE)) == 0) {
		dbus_bool_t offlinemode;

		if (cui_dbus_get_basic_variant(&arg,
				DBUS_TYPE_BOOLEAN, &offlinemode) == 0) {
			manager->offlinemode = offlinemode;
			update_manager_property(MANAGER_OFFLINEMODE);
		}
	}

	return TRUE;
}

static void get_technologies_callback(DBusPendingCall *pending, void *user_data)
{
	connman_manager_get_technologies_cb_f callback;
	DBusMessageIter arg;
	DBusMessage *reply;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	manager->get_technologies_call = NULL;
	callback = manager->get_technologies_cb;
	manager->get_technologies_cb = NULL;

	reply = dbus_pending_call_steal_reply(pending);
	if (reply == NULL)
		goto error;

	if (dbus_message_iter_init(reply, &arg) == FALSE)
		goto error;

	if (callback != NULL)
		callback(&arg);
error:
	if (reply != NULL)
		dbus_message_unref(reply);

	dbus_pending_call_unref(pending);
}

static gboolean technology_added_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	DBusMessageIter arg;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	if (manager->technology_added_cb != NULL)
		manager->technology_added_cb(&arg);

	return TRUE;
}

static gboolean technology_removed_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	DBusMessageIter arg;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	if (manager->technology_removed_cb != NULL)
		manager->technology_removed_cb(&arg);

	return TRUE;
}

static void get_services_callback(DBusPendingCall *pending, void *user_data)
{
	connman_manager_get_services_cb_f callback;
	DBusMessageIter arg;
	DBusMessage *reply;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	manager->get_services_call = NULL;
	callback = manager->get_services_cb;
	manager->get_services_cb = NULL;

	reply = dbus_pending_call_steal_reply(pending);
	if (reply == NULL)
		goto error;

	if (dbus_message_iter_init(reply, &arg) == FALSE)
		goto error;

	if (callback != NULL)
		callback(&arg);
error:
	if (reply != NULL)
		dbus_message_unref(reply);

	dbus_pending_call_unref(pending);
}

static gboolean services_changed_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	DBusMessageIter arg;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	if (manager->services_changed_cb != NULL)
		manager->services_changed_cb(&arg);

	return TRUE;
}

int __connman_manager_get_technologies(connman_manager_get_technologies_cb_f cb)
{
	DBusMessage *message;

	if (manager == NULL)
		return -EINVAL;

	if (manager->get_technologies_call != NULL) {
		dbus_pending_call_cancel(manager->get_technologies_call);
		dbus_pending_call_unref(manager->get_technologies_call);

		manager->get_technologies_call = NULL;
	}

	manager->get_technologies_cb = NULL;

	if (cb == NULL)
		return 0;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"GetTechnologies");
	if (message == NULL)
		return -ENOMEM;

	manager->get_technologies_cb = cb;

	if (dbus_connection_send_with_reply(manager->dbus_cnx, message,
					&manager->get_technologies_call,
					DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(manager->get_technologies_call,
				get_technologies_callback, NULL, NULL) == FALSE)
		goto error;

	dbus_message_unref(message);

	return 0;

error:
	dbus_message_unref(message);

	return -EINVAL;
}

int __connman_manager_register_technology_signals(connman_manager_technology_added_cb_f added_cb,
			connman_manager_technology_removed_cb_f removed_cb)
{
	if (manager == NULL)
		return -EINVAL;

	manager->technology_added_cb = NULL;
	manager->technology_removed_cb = NULL;

	if (manager->technology_added_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->technology_added_wid);
	if (manager->technology_removed_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->technology_removed_wid);

	manager->technology_added_wid = 0;
	manager->technology_removed_wid = 0;

	if (added_cb == NULL && removed_cb == NULL)
		return 0;

	manager->technology_added_wid = g_dbus_add_signal_watch(
						manager->dbus_cnx,
						CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"TechnologyAdded",
						technology_added_signal_cb,
						NULL, NULL);
	if (manager->technology_added_wid == 0)
		goto error;

	manager->technology_removed_wid = g_dbus_add_signal_watch(
						manager->dbus_cnx,
						CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"TechnologyRemoved",
						technology_removed_signal_cb,
						NULL, NULL);
	if (manager->technology_removed_wid == 0)
		goto error;

	manager->technology_added_cb = added_cb;
	manager->technology_removed_cb = removed_cb;

	return 0;

error:
	if (manager->technology_added_wid != 0) {
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->technology_added_wid);
		manager->technology_added_wid = 0;
	}

	return -EINVAL;
}

int __connman_manager_get_services(connman_manager_get_services_cb_f cb)
{
	DBusMessage *message;

	if (manager == NULL)
		return -EINVAL;

	manager->get_services_cb = NULL;

	if (manager->get_services_call != NULL) {
		dbus_pending_call_cancel(manager->get_services_call);
		dbus_pending_call_unref(manager->get_services_call);

		manager->get_services_call = NULL;
	}

	if (cb == NULL)
		return 0;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"GetServices");
	if (message == NULL)
		return -ENOMEM;

	manager->get_services_cb = cb;

	if (dbus_connection_send_with_reply(manager->dbus_cnx, message,
					&manager->get_services_call,
					DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(manager->get_services_call,
				get_services_callback, NULL, NULL) == FALSE)
		goto error;

	dbus_message_unref(message);

	return 0;

error:
	dbus_message_unref(message);

	return -EINVAL;
}

int __connman_manager_register_service_signal(connman_manager_service_changed_cb cb)
{
	if (manager == NULL)
		return -EINVAL;

	manager->services_changed_cb = NULL;

	if (manager->services_changed_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->services_changed_wid);

	manager->services_changed_wid = 0;

	if (cb == NULL)
		return 0;

	manager->services_changed_wid = g_dbus_add_signal_watch(
						manager->dbus_cnx,
						CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"ServicesChanged",
						services_changed_signal_cb,
						NULL, NULL);
	if (manager->services_changed_wid == 0)
		return -EINVAL;

	manager->services_changed_cb = cb;

	return 0;
}

int connman_manager_init(connman_property_changed_cb_f property_changed_cb,
							void *user_data)
{
	DBusMessage *message = NULL;
	int err = -EINVAL;

	if (connman == NULL)
		return -EINVAL;

	if (manager != NULL)
		return 0;

	manager = g_try_malloc0(sizeof(struct connman_manager));
	if (manager == NULL)
		return -ENOMEM;

	manager->dbus_cnx = dbus_connection_ref(connman->dbus_cnx);

	manager->property_changed_cb = property_changed_cb;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"GetProperties");
	if (message == NULL) {
		err = -ENOMEM;
		goto error;
	}

	if (dbus_connection_send_with_reply(manager->dbus_cnx, message,
					&manager->get_properties_call,
					DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(manager->get_properties_call,
				get_properties_callback, NULL, NULL) == FALSE)
		goto error;

	manager->property_changed_wid = g_dbus_add_signal_watch(
						manager->dbus_cnx,
						CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"PropertyChanged",
						property_changed_signal_cb,
						NULL, NULL);
	if (manager->property_changed_wid == 0)
		goto error;

	manager->property_user_data = user_data;

	dbus_message_unref(message);

	return 0;

error:
	if (message != NULL)
		dbus_message_unref(message);

	connman_manager_finalize();

	return err;
}

void connman_manager_finalize(void)
{
	int i;

	if (manager == NULL)
		return;

	manager->property_changed_cb = NULL;

	for (i = 0; i < MANAGER_MAX; i++) {
		if (manager->source[i] != 0)
			g_source_remove(manager->source[i]);
	}

	if (manager->get_properties_call != NULL) {
		dbus_pending_call_cancel(manager->get_properties_call);
		dbus_pending_call_unref(manager->get_properties_call);
	}

	if (manager->property_changed_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->property_changed_wid);

	if (manager->get_technologies_call != NULL) {
		dbus_pending_call_cancel(manager->get_technologies_call);
		dbus_pending_call_unref(manager->get_technologies_call);
	}

	if (manager->technology_added_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->technology_added_wid);
	if (manager->technology_removed_wid != 0)
		g_dbus_remove_watch(manager->dbus_cnx,
					manager->technology_removed_wid);

	if (manager->get_services_call != NULL) {
		dbus_pending_call_cancel(manager->get_services_call);
		dbus_pending_call_unref(manager->get_services_call);
	}

	dbus_connection_unref(manager->dbus_cnx);

	g_free(manager);
	manager = NULL;
}

int connman_manager_set_offlinemode(gboolean offlinemode)
{
	DBusMessage *message;
	DBusMessageIter arg;

	if (manager == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"SetProperty");
	if (message == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(message, &arg);

	cui_dbus_append_basic(&arg, PROPERTY(MANAGER_OFFLINEMODE),
					DBUS_TYPE_BOOLEAN, &offlinemode);

	if (g_dbus_send_message(manager->dbus_cnx, message) == FALSE)
		return -EINVAL;

	return 0;
}

enum connman_state connman_manager_get_state(void)
{
	if (manager == NULL)
		return CONNMAN_STATE_UNKNOWN;

	return manager->state;
}

gboolean connman_manager_get_offlinemode(void)
{
	if (manager == NULL)
		return TRUE;

	return manager->offlinemode;
}

void connman_manager_register_agent(const char *path)
{
	DBusMessage *message;
	DBusMessageIter arg;

	if (manager == NULL)
		return;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"RegisterAgent");
	if (message == NULL)
		return;

	dbus_message_iter_init_append(message, &arg);

	cui_dbus_append_basic(&arg, NULL, DBUS_TYPE_OBJECT_PATH, &path);

	g_dbus_send_message(manager->dbus_cnx, message);
}

void connman_manager_unregister_agent(const char *path)
{
	DBusMessage *message;
	DBusMessageIter arg;

	if (manager == NULL)
		return;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						CONNMAN_MANAGER_PATH,
						CONNMAN_MANAGER_INTERFACE,
						"UnregisterAgent");
	if (message == NULL)
		return;

	dbus_message_iter_init_append(message, &arg);

	cui_dbus_append_basic(&arg, NULL, DBUS_TYPE_OBJECT_PATH, &path);

	g_dbus_send_message(manager->dbus_cnx, message);

}

