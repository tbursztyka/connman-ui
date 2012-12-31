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

#define CONNMAN_TECHNOLOGY_INTERFACE CONNMAN_DBUS_NAME ".Technology"

#define PROPERTY(n) Technology_updatable_properties[n]

static void get_technologies_cb(DBusMessageIter *iter);

enum connman_technology_property {
	TECHNOLOGY_POWERED              = 0,
	TECHNOLOGY_CONNECTED            = 1,
	TECHNOLOGY_TETHERING            = 2,
	TECHNOLOGY_TETHERING_IDENTIFIER = 3,
	TECHNOLOGY_TETHERING_PASSPHRASE = 4,
	TECHNOLOGY_MAX                  = 5,
};

static const char *Technology_updatable_properties[] = {
	"Powered",
	"Connected",
	"Tethering",
	"TetheringIdentifier",
	"TetheringPassphrase",
};

struct connman_technology {
	char *path;
	char *name;
	char *type;

	gboolean powered;
	gboolean connected;

	gboolean tethering;
	char *tethering_identifier;
	char *tethering_passphrase;

	int update_index;

	guint property_changed_wid;
	guint to_update[TECHNOLOGY_MAX];
	connman_property_changed_cb_f property_changed_cb;
	void *property_changed_user_data;

	connman_scan_cb_f scan_cb;
	DBusPendingCall *scan_call;
	void *scan_user_data;

	DBusPendingCall *call_modify[TECHNOLOGY_MAX];
	guint to_error[TECHNOLOGY_MAX];
	connman_property_set_cb_f property_set_error_cb;
	void *property_set_error_user_data;
};

struct connman_technology_interface {
	DBusConnection *dbus_cnx;

	GHashTable *techs;
	gboolean set;

	connman_property_changed_cb_f property_changed_cb;
	void *property_user_data;

	connman_path_changed_cb_f added_cb;
	connman_path_changed_cb_f removed_cb;
};

static struct connman_technology_interface *tech_if = NULL;

static void technology_free(gpointer data)
{
	struct connman_technology *technology = data;
	int i;

	if (technology->property_changed_wid != 0)
		g_dbus_remove_watch(tech_if->dbus_cnx,
					technology->property_changed_wid);

	for (i = 0; i < TECHNOLOGY_MAX; i++) {
		if (technology->to_update[i] != 0)
			g_source_remove(technology->to_update[i]);

		if (technology->call_modify[i] != 0) {
			dbus_pending_call_cancel(technology->call_modify[i]);
			dbus_pending_call_unref(technology->call_modify[i]);
		}

		if (technology->to_error[i] != 0)
			g_source_remove(technology->to_error[i]);
	}

	if (technology->scan_call != NULL) {
		dbus_pending_call_cancel(technology->scan_call);
		dbus_pending_call_unref(technology->scan_call);
	}

	g_free(technology->path);
	g_free(technology->name);
	g_free(technology->type);
	g_free(technology->tethering_identifier);
	g_free(technology->tethering_passphrase);

	g_free(technology);
}

static struct connman_technology *get_technology(const char *path)
{
	if (path == NULL || tech_if == NULL)
		return NULL;

	return g_hash_table_lookup(tech_if->techs, path);
}

static void destroy_property(gpointer user_data)
{
	struct property_change *property = user_data;
	struct connman_technology *technology;

	technology = get_technology(property->path);
	if (technology != NULL) {
		if (property->error != 0)
			technology->to_error[property->index] = 0;
		else
			technology->to_update[property->index] = 0;
	}

	g_free(property);
}

static gboolean property_changed_or_error(gpointer user_data)
{
	struct property_change *property = user_data;
	struct connman_technology *technology;

	technology = get_technology(property->path);
	if (technology == NULL)
		return FALSE;

	if (property->error != 0) {
		if (technology->property_set_error_cb == NULL)
			return FALSE;

		technology->property_set_error_cb(property->path,
				property->name, property->error,
				technology->property_set_error_user_data);
		return FALSE;
	}

	if (tech_if->property_changed_cb != NULL) {
		tech_if->property_changed_cb(property->path, property->name,
						tech_if->property_user_data);
	}

	if (technology->property_changed_cb != NULL) {
		technology->property_changed_cb(property->path, property->name,
				technology->property_changed_user_data);
	}

	return FALSE;
}

static void property_update(struct connman_technology *technology, int index)
{
	struct property_change *property;

	if (technology->property_changed_cb == NULL)
		return;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		return;

	property->index = index;
	property->name = PROPERTY(index);
	property->path = technology->path;

	if (technology->to_update[index] != 0)
		g_source_remove(technology->to_update[index]);

	technology->to_update[index] = g_timeout_add_full(G_PRIORITY_DEFAULT,
						0, property_changed_or_error,
						property, destroy_property);
}

static bool update_technology_property(DBusMessageIter *arg, void *user_data)
{
	struct connman_technology *technology = user_data;
	const char *name, *value;

	if (cui_dbus_get_basic(arg, DBUS_TYPE_STRING, &name) != 0)
		return FALSE;

	dbus_message_iter_next(arg);

	if (g_strcmp0(name, "Name") == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);
		technology->name = g_strdup(value);
	} else if (g_strcmp0(name, "Type") == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);
		technology->type = g_strdup(value);
	} else if (g_strcmp0(name, PROPERTY(TECHNOLOGY_POWERED)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &technology->powered);
		technology->update_index = TECHNOLOGY_POWERED;
	} else if (g_strcmp0(name, PROPERTY(TECHNOLOGY_CONNECTED)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &technology->connected);
		technology->update_index = TECHNOLOGY_CONNECTED;
	} else if (g_strcmp0(name, PROPERTY(TECHNOLOGY_TETHERING)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &technology->tethering);
		technology->update_index = TECHNOLOGY_TETHERING;;
	} else if (g_strcmp0(name,
			PROPERTY(TECHNOLOGY_TETHERING_IDENTIFIER)) == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);

		g_free(technology->tethering_identifier);
		technology->tethering_identifier = g_strdup(value);

		technology->update_index = TECHNOLOGY_TETHERING_IDENTIFIER;
	} else if (g_strcmp0(name,
			PROPERTY(TECHNOLOGY_TETHERING_PASSPHRASE)) == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);

		g_free(technology->tethering_passphrase);
		technology->tethering_passphrase = g_strdup(value);

		technology->update_index = TECHNOLOGY_TETHERING_PASSPHRASE;
	}

	if (technology->update_index < TECHNOLOGY_MAX)
		property_update(technology, technology->update_index);

	return FALSE;
}

static gboolean property_changed_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	struct connman_technology *technology = user_data;
	DBusMessageIter arg;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	technology->update_index = TECHNOLOGY_MAX;

	update_technology_property(&arg, technology);

	return TRUE;
}

static void update_or_create_technology(const char *obj_path,
					DBusMessageIter *dict)
{
	struct connman_technology *technology;

	if (tech_if == NULL)
		return;

	technology = g_hash_table_lookup(tech_if->techs, obj_path);
	if (technology == NULL) {
		technology = g_try_malloc0(sizeof(struct connman_technology));
		if (technology == NULL)
			return;

		technology->path = g_strdup(obj_path);

		g_hash_table_insert(tech_if->techs,
					technology->path, technology);

		technology->property_changed_wid = g_dbus_add_signal_watch(
						tech_if->dbus_cnx,
						CONNMAN_DBUS_NAME,
						technology->path,
						CONNMAN_TECHNOLOGY_INTERFACE,
						"PropertyChanged",
						property_changed_signal_cb,
						technology, NULL);
	}

	cui_dbus_foreach_dict_entry(dict,
			update_technology_property, technology);
}

static void technology_added_cb(DBusMessageIter *iter)
{
	char *obj_path;

	if (iter == NULL)
		return;

	cui_dbus_get_basic(iter, DBUS_TYPE_OBJECT_PATH, &obj_path);

	dbus_message_iter_next(iter);

	update_or_create_technology(obj_path, iter);

	if (tech_if != NULL && tech_if->added_cb != NULL)
		tech_if->added_cb(obj_path);

	if (tech_if->set == TRUE)
		__connman_manager_get_technologies(get_technologies_cb);
}

static void technology_removed_cb(DBusMessageIter *iter)
{
	char *obj_path;

	if (iter == NULL)
		return;

	cui_dbus_get_basic(iter, DBUS_TYPE_OBJECT_PATH, &obj_path);

	if (tech_if != NULL && tech_if->removed_cb != NULL)
		tech_if->removed_cb(obj_path);

	g_hash_table_remove(tech_if->techs, obj_path);
}

static void get_technologies_cb(DBusMessageIter *iter)
{
	DBusMessageIter array;
	DBusMessageIter strt;
	char *obj_path;
	int arg_type;

	if (iter == NULL || tech_if == NULL)
		return;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return;

	dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		dbus_message_iter_recurse(&array, &strt);

		cui_dbus_get_basic(&strt, DBUS_TYPE_OBJECT_PATH, &obj_path);

		dbus_message_iter_next(&strt);
		update_or_create_technology(obj_path, &strt);

		dbus_message_iter_next(&array);
		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	if (tech_if->set == FALSE) {
		if (__connman_manager_register_technology_signals(
						technology_added_cb,
						technology_removed_cb) == 0)
			tech_if->set = TRUE;

		__connman_manager_get_technologies(get_technologies_cb);
	}
}

static void scan_callback(DBusPendingCall *pending, void *user_data)
{
	struct connman_technology *technology = user_data;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	dbus_pending_call_unref(pending);
	technology->scan_call = NULL;

	if (technology->scan_cb != NULL)
		technology->scan_cb(technology->scan_user_data);
}

static void set_property_cb(DBusPendingCall *pending, void *user_data)
{
	struct property_setting *set = user_data;
	struct connman_technology *technology;
	struct property_change *property;
	DBusMessage *reply;
	DBusError error;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	if (set == NULL)
		return;

	technology = set->data;

	technology->call_modify[set->index] = NULL;

	if (technology->property_set_error_cb == NULL)
		return;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		return;

	reply = dbus_pending_call_steal_reply(pending);
	if (reply == NULL)
		return;

	if (dbus_set_error_from_message(&error, reply) == FALSE)
		goto done;

	printf("SetProperty Error: %s\n", error.message);
	dbus_error_free(&error);

	if (technology->property_set_error_cb == NULL)
		goto done;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		goto done;

	property->path = technology->path;
	property->name = PROPERTY(set->index);
	property->error = -1;

	if (technology->to_error[set->index] != 0)
		g_source_remove(technology->to_error[set->index]);

	technology->to_error[set->index] = g_timeout_add_full(
						G_PRIORITY_DEFAULT,
						0, property_changed_or_error,
						property, destroy_property);

done:
	dbus_message_unref(reply);
}

static int set_technology_property(struct connman_technology *technology,
				enum connman_technology_property property,
				int dbus_type, void *data)
{
	struct property_setting *set = NULL;
	const char *property_name;
	DBusMessage *message;
	DBusMessageIter arg;

	if (connman == NULL)
		return -EINVAL;

	if (technology->call_modify[property] != NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						technology->path,
						CONNMAN_TECHNOLOGY_INTERFACE,
						"SetProperty");
	if (message == NULL)
		return -ENOMEM;

	set = g_try_malloc0(sizeof(struct property_setting));
	if (set == NULL)
		goto error;

	set->data = technology;
	set->index = property;

	property_name = PROPERTY(property);

	dbus_message_iter_init_append(message, &arg);

	cui_dbus_append_basic(&arg, property_name, dbus_type, data);

	if (dbus_connection_send_with_reply(tech_if->dbus_cnx, message,
					&technology->call_modify[property],
					DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(technology->call_modify[property],
					set_property_cb, set, g_free) == FALSE)
		goto error;

	return 0;

error:
	dbus_message_unref(message);

	if (set == NULL)
		return -ENOMEM;

	g_free(set);

	return -EINVAL;
}

int connman_technology_init(void)
{
	int ret;

	if (connman == NULL)
		return -EINVAL;

	if (tech_if != NULL)
		return 0;

	tech_if = g_try_malloc0(sizeof(struct connman_technology_interface));
	if (tech_if == NULL)
		return -ENOMEM;

	tech_if->techs = g_hash_table_new_full(g_str_hash, g_str_equal,
							NULL, technology_free);
	if (tech_if->techs == NULL) {
		g_free(tech_if);
		return -ENOMEM;
	}

	ret = __connman_manager_get_technologies(get_technologies_cb);
	if (ret < 0)
		return ret;

	tech_if->dbus_cnx = dbus_connection_ref(connman->dbus_cnx);

	return 0;
}

void connman_technology_finalize(void)
{
	if (tech_if == NULL)
		return;

	__connman_manager_register_technology_signals(NULL, NULL);

	dbus_connection_unref(tech_if->dbus_cnx);

	g_hash_table_destroy(tech_if->techs);

	g_free(tech_if);

	tech_if = NULL;
}

void connman_technology_set_global_property_callback(
			connman_property_changed_cb_f property_changed_cb,
			void *user_data)
{
	if (tech_if == NULL)
		return;

	tech_if->property_changed_cb = property_changed_cb;
	tech_if->property_user_data = user_data;
}

void connman_technology_set_property_changed_callback(const char *path,
			connman_property_changed_cb_f property_changed_cb,
			void *user_data)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return;

	technology->property_changed_cb = property_changed_cb;
	technology->property_changed_user_data = user_data;
}

void connman_technology_set_property_error_callback(const char *path,
				connman_property_set_cb_f property_set_cb,
				void *user_data)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return;

	technology->property_set_error_cb = property_set_cb;
	technology->property_set_error_user_data = user_data;
}

void connman_technology_set_removed_callback(
					connman_path_changed_cb_f removed_cb)
{
	if (tech_if != NULL)
		tech_if->removed_cb = removed_cb;
}

void connman_technology_set_added_callback(connman_path_changed_cb_f added_cb)
{
	if (tech_if != NULL)
		tech_if->added_cb = added_cb;
}

GList *connman_technology_get_technologies(void)
{
	if (tech_if == NULL || tech_if->techs == NULL)
		return NULL;

	return g_hash_table_get_keys(tech_if->techs);
}

int connman_technology_scan(const char *path,
				connman_scan_cb_f callback, void *user_data)
{
	struct connman_technology *technology;
	DBusMessage *message;

	technology = get_technology(path);
	if (technology == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						technology->path,
						CONNMAN_TECHNOLOGY_INTERFACE,
						"Scan");
	if (message == NULL)
		return -ENOMEM;

	technology->scan_cb = callback;

	if (dbus_connection_send_with_reply(tech_if->dbus_cnx, message,
				&technology->scan_call,
				DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(technology->scan_call,
				scan_callback, technology, NULL) == FALSE)
		goto error;

	technology->scan_user_data = user_data;

	dbus_message_unref(message);

	return 0;

error:
	dbus_message_unref(message);

	technology->scan_cb = NULL;

	return -EINVAL;
}

int connman_technology_enable(const char *path, gboolean enable)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return -EINVAL;

	if (technology->powered == enable)
		return 0;

	return set_technology_property(technology,
					TECHNOLOGY_POWERED,
					DBUS_TYPE_BOOLEAN, &enable);
}

int connman_technology_tether(const char *path, gboolean tethering)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return -EINVAL;

	if (technology->tethering == tethering)
		return 0;

	return set_technology_property(technology,
					TECHNOLOGY_TETHERING,
					DBUS_TYPE_BOOLEAN, &tethering);
}

int connman_technology_set_tethering_identifier(const char *path,
						const char *identifier)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return -EINVAL;

	return set_technology_property(technology,
					TECHNOLOGY_TETHERING_IDENTIFIER,
					DBUS_TYPE_STRING, &identifier);
}

int connman_technology_set_tethering_passphrase(const char *path,
						const char *passphrase)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return -EINVAL;

	return set_technology_property(technology,
					TECHNOLOGY_TETHERING_PASSPHRASE,
					DBUS_TYPE_STRING, &passphrase);
}

const char *connman_technology_get_name(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return NULL;

	return technology->name;
}

const char *connman_technology_get_type(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return NULL;

	return technology->type;
}

gboolean connman_technology_is_enabled(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return FALSE;

	return technology->powered;
}

gboolean connman_technology_is_connected(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return FALSE;

	return technology->connected;
}

gboolean connman_technology_is_tethering(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return FALSE;

	return technology->tethering;
}

const char *connman_technology_get_tethering_identifier(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return NULL;

	return technology->tethering_identifier;
}

const char *connman_technology_get_tethering_passphrase(const char *path)
{
	struct connman_technology *technology;

	technology = get_technology(path);
	if (technology == NULL)
		return NULL;

	return technology->tethering_passphrase;
}

