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

#include <string.h>

#define CONNMAN_AGENT_INTERFACE CONNMAN_DBUS_NAME ".Agent"
#define CONNMAN_AGENT_PATH "/net/connman/agent/connmanui"

struct connman_agent {
	DBusConnection *dbus_cnx;

	DBusMessage *pending_reply;

	agent_error_cb_f error_cb;
	agent_browser_cb_f browser_cb;
	agent_input_cb_f input_cb;
	agent_cancel_cb_f cancel_cb;
};

static struct connman_agent *agent_if;

static DBusMessage *agent_release_method(DBusConnection *dbus_cnx,
						DBusMessage *msg, void *data)
{
	if (agent_if == NULL)
		goto reply;

	g_dbus_unregister_interface(agent_if->dbus_cnx,
				CONNMAN_AGENT_PATH, CONNMAN_AGENT_INTERFACE);

	if (agent_if->pending_reply != NULL)
		dbus_message_unref(agent_if->pending_reply);

	dbus_connection_unref(agent_if->dbus_cnx);

	g_free(agent_if);
	agent_if = NULL;

reply:
	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *agent_report_error_method(DBusConnection *dbus_cnx,
						DBusMessage *msg, void *data)
{
	DBusMessageIter arg;
	const char *error;
	const char *path;

	if (agent_if == NULL || agent_if->error_cb == NULL)
		goto error;

	if (dbus_message_iter_init(msg, &arg) == FALSE)
		goto error;

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_OBJECT_PATH, &path) != 0)
		goto error;

	dbus_message_iter_next(&arg);

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_STRING, &error) != 0)
		goto error;

	agent_if->pending_reply = dbus_message_ref(msg);
	agent_if->error_cb(path, error);

	return NULL;

error:
	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static DBusMessage *agent_request_browser_method(DBusConnection *dbus_cnx,
						DBusMessage *msg, void *data)
{
	DBusMessageIter arg;
	const char *path;
	const char *url;

	if (agent_if == NULL || agent_if->browser_cb == NULL)
		goto error;

	if (dbus_message_iter_init(msg, &arg) == FALSE)
		goto error;

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_OBJECT_PATH, &path) != 0)
		goto error;

	dbus_message_iter_next(&arg);

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_STRING, &url) != 0)
		goto error;

	agent_if->pending_reply = dbus_message_ref(msg);
	agent_if->browser_cb(path, url);

	return NULL;

error:
	return g_dbus_create_error(msg, CONNMAN_ERROR ".Canceled", NULL);
}

struct agent_input_data {
	gboolean hidden;
	gboolean identity;
	gboolean passphrase;
	gboolean wpspin;
	gboolean login;
	const char *previous_passphrase;
	const char *previous_wpspin;
};

static bool parse_input_request(DBusMessageIter *arg, void *user_data)
{
	struct agent_input_data *data = user_data;
	const char *name;

	printf("parse_input_request\n");

	if (cui_dbus_get_basic(arg, DBUS_TYPE_STRING, &name) != 0)
		return FALSE;

	dbus_message_iter_next(arg);

	if (g_strcmp0(name, "Passphrase") == 0)
		data->passphrase = TRUE;
	else if (g_strcmp0(name, "WPS") == 0)
		data->wpspin = TRUE;
	else if (g_strcmp0(name, "Name") == 0)
		data->hidden = TRUE;
	else if (g_strcmp0(name, "Identity") == 0)
		data->identity = TRUE;
	else if (g_strcmp0(name, "Username") == 0)
		data->login = TRUE;
	else if (g_strcmp0(name, "PreviousPassphrase") == 0) {
		DBusMessageIter dict;
		const char **value;
		const char *type;

		dbus_message_iter_recurse(arg, &dict);

		cui_dbus_get_dict_entry_basic(&dict,
					"Type", DBUS_TYPE_STRING, &type);

		if (g_strcmp0(type, "psk") == 0)
			value = &data->previous_passphrase;
		else if (g_strcmp0(type, "wpspin") == 0)
			value = &data->previous_wpspin;

		cui_dbus_get_dict_entry_basic(&dict,
					"Value", DBUS_TYPE_STRING, &value);
	}

	return FALSE;
}

static DBusMessage *agent_request_input_method(DBusConnection *dbus_cnx,
						DBusMessage *msg, void *data)
{
	struct agent_input_data arg_data;
	DBusMessageIter arg;
	const char *path;

	printf("agent_request_input_method\n");

	if (agent_if == NULL || agent_if->input_cb == NULL)
		goto error;

	memset(&arg_data, 0, sizeof(struct agent_input_data));

	if (dbus_message_iter_init(msg, &arg) == FALSE)
		goto error;

	if (cui_dbus_get_basic(&arg, DBUS_TYPE_OBJECT_PATH, &path) != 0)
		goto error;

	dbus_message_iter_next(&arg);

	cui_dbus_foreach_dict_entry(&arg, parse_input_request, &arg_data);

	if (arg_data.hidden == FALSE &&
				arg_data.identity == FALSE &&
				arg_data.passphrase == FALSE &&
				arg_data.login == FALSE)
		goto error;

	agent_if->pending_reply = dbus_message_ref(msg);
	agent_if->input_cb(path, arg_data.hidden, arg_data.identity,
			arg_data.passphrase, arg_data.previous_passphrase,
			arg_data.wpspin, arg_data.previous_wpspin,
			arg_data.login);

	return NULL;

error:
	return g_dbus_create_error(msg, CONNMAN_ERROR ".Canceled", NULL);
}

static DBusMessage *agent_cancel_method(DBusConnection *dbus_cnx,
						DBusMessage *msg, void *data)
{
	if (agent_if == NULL)
		goto reply;

	if (agent_if->pending_reply != NULL) {
		dbus_message_unref(agent_if->pending_reply);
		agent_if->pending_reply = NULL;
	}

	if (agent_if->cancel_cb != NULL)
		agent_if->cancel_cb();

reply:
	return g_dbus_create_reply(msg, DBUS_TYPE_INVALID);
}

static GDBusMethodTable agent_methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, agent_release_method) },
	{ GDBUS_ASYNC_METHOD("ReportError",
			GDBUS_ARGS({"service", "o"}, {"error", "s"}),
			NULL, agent_report_error_method) },
	{ GDBUS_ASYNC_METHOD("RequestBrowser",
			GDBUS_ARGS({"service", "o"}, {"url", "s"}),
			NULL, agent_request_browser_method) },
	{ GDBUS_ASYNC_METHOD("RequestInput",
			GDBUS_ARGS({"service", "o"}, {"fields", "a{sv}"}),
			GDBUS_ARGS({"inputs", "a{sv}"}),
			agent_request_input_method) },
	{ GDBUS_METHOD("Cancel", NULL, NULL, agent_cancel_method) },
	{ NULL },
};

static inline void send_agent_reply(DBusMessage *reply)
{
	if (reply == NULL)
		goto error;

	g_dbus_send_message(agent_if->dbus_cnx, reply);

error:
	dbus_message_unref(agent_if->pending_reply);
	agent_if->pending_reply = NULL;
}

static void agent_reply_error(const char *error, const char *msg)
{
	DBusMessage *reply;

	if (agent_if == NULL || agent_if->pending_reply == NULL)
		return;

	reply = g_dbus_create_error(agent_if->pending_reply, error, "%s", msg);

	send_agent_reply(reply);
}

int connman_agent_init(void)
{
	if (connman == NULL)
		return -EINVAL;

	if (agent_if != NULL)
		return 0;

	agent_if = g_try_malloc0(sizeof(struct connman_agent));
	if (agent_if == NULL)
		return -ENOMEM;

	agent_if->dbus_cnx = dbus_connection_ref(connman->dbus_cnx);

	g_dbus_register_interface(agent_if->dbus_cnx,
				CONNMAN_AGENT_PATH, CONNMAN_AGENT_INTERFACE,
				agent_methods, NULL, NULL, NULL, NULL);

	connman_manager_register_agent(CONNMAN_AGENT_PATH);

	return 0;
}

void connman_agent_finalize(void)
{
	if (agent_if == NULL)
		return;

	connman_manager_unregister_agent(CONNMAN_AGENT_PATH);

	g_dbus_unregister_interface(agent_if->dbus_cnx,
				CONNMAN_AGENT_PATH, CONNMAN_AGENT_INTERFACE);

	if (agent_if->pending_reply != NULL)
		dbus_message_unref(agent_if->pending_reply);

	dbus_connection_unref(agent_if->dbus_cnx);

	g_free(agent_if);
	agent_if = NULL;
}

int connman_agent_set_error_cb(agent_error_cb_f error_cb)
{
	if (agent_if == NULL)
		return -EINVAL;

	agent_if->error_cb = error_cb;

	return 0;
}

int connman_agent_set_browser_cb(agent_browser_cb_f browser_cb)
{
	if (agent_if == NULL)
		return -EINVAL;

	agent_if->browser_cb = browser_cb;

	return 0;
}

int connman_agent_set_input_cb(agent_input_cb_f input_cb)
{
	if (agent_if == NULL)
		return -EINVAL;

	agent_if->input_cb = input_cb;

	return 0;
}

int connman_agent_set_cancel_cb(agent_cancel_cb_f cancel_cb)
{
	if (agent_if == NULL)
		return -EINVAL;

	agent_if->cancel_cb = cancel_cb;

	return 0;
}

void connman_agent_reply_retry(void)
{
	agent_reply_error(CONNMAN_AGENT_INTERFACE ".Error.Retry", "Retry");
}

void connman_agent_reply_canceled(void)
{
	agent_reply_error(CONNMAN_AGENT_INTERFACE ".Error.Canceled",
								"Canceled");
}

void connman_agent_reply_launch_browser(void)
{
	agent_reply_error(CONNMAN_AGENT_INTERFACE ".Error.LaunchBrowser",
							"Launch the Browser");
}

int connman_agent_reply_identity(const char *identity, const char *passphrase)
{
	DBusMessageIter arg, dict;
	DBusMessage *reply;

	if (agent_if == NULL || agent_if->pending_reply == NULL)
		return 0;

	if (identity == NULL || passphrase == NULL)
		return -EINVAL;

	reply = dbus_message_new_method_return(agent_if->pending_reply);
	if (reply == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(reply, &arg);

	if (cui_dbus_open_dict(&arg, &dict) == FALSE)
		goto error;

	cui_dbus_append_dict_entry_basic(&dict, "Identity",
						DBUS_TYPE_STRING, &identity);
	cui_dbus_append_dict_entry_basic(&dict, "Passphrase",
						DBUS_TYPE_STRING, &passphrase);

	dbus_message_iter_close_container(&dict, &arg);

	send_agent_reply(reply);

	return 0;

error:
	dbus_message_unref(reply);

	dbus_message_unref(agent_if->pending_reply);
	agent_if->pending_reply = NULL;

	return -ENOMEM;
}

int connman_agent_reply_passphrase(const char *hidden_name,
		const char *passphrase, gboolean wps, const char *wpspin)
{
	DBusMessageIter arg, dict;
	DBusMessage *reply;

	if (agent_if == NULL || agent_if->pending_reply == NULL)
		return 0;

	if (passphrase == NULL && hidden_name == NULL && wps == FALSE)
		return -EINVAL;

	reply = dbus_message_new_method_return(agent_if->pending_reply);
	if (reply == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(reply, &arg);

	if (cui_dbus_open_dict(&arg, &dict) == FALSE)
		goto error;

	if (hidden_name != NULL)
		cui_dbus_append_dict_entry_basic(&dict, "Name",
					DBUS_TYPE_STRING, &hidden_name);

	if (passphrase != NULL)
		cui_dbus_append_dict_entry_basic(&dict, "Passphrase",
					DBUS_TYPE_STRING, &passphrase);
	else if (wps == TRUE) {
		if (wpspin == NULL)
			wpspin = "";

		cui_dbus_append_dict_entry_basic(&dict, "WPS",
					DBUS_TYPE_STRING, &wpspin);
	}

	dbus_message_iter_close_container(&arg, &dict);

	send_agent_reply(reply);

	return 0;

error:
	dbus_message_unref(reply);

	dbus_message_unref(agent_if->pending_reply);
	agent_if->pending_reply = NULL;

	return -ENOMEM;
}

int connman_agent_reply_login(const char *username, const char *password)
{
	DBusMessageIter arg, dict;
	DBusMessage *reply;

	if (agent_if == NULL || agent_if->pending_reply == NULL)
		return 0;

	if (username == NULL && password == NULL)
		return -EINVAL;

	reply = dbus_message_new_method_return(agent_if->pending_reply);
	if (reply == NULL)
		return -ENOMEM;

	dbus_message_iter_init_append(reply, &arg);

	if (cui_dbus_open_dict(&arg, &dict) == FALSE)
		goto error;

	cui_dbus_append_dict_entry_basic(&dict, "Username",
						DBUS_TYPE_STRING, &username);
	cui_dbus_append_dict_entry_basic(&dict, "Password",
						DBUS_TYPE_STRING, &password);

	dbus_message_iter_close_container(&dict, &arg);

	send_agent_reply(reply);

	return 0;

error:
	dbus_message_unref(reply);

	dbus_message_unref(agent_if->pending_reply);
	agent_if->pending_reply = NULL;

	return -ENOMEM;
}

