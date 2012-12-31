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

#define CONNMAN_SERVICE_INTERFACE CONNMAN_DBUS_NAME ".Service"

#define PROPERTY(n) Service_updatable_properties[n]

enum connman_service_property {
	SERVICE_STATE                     = 0,
	SERVICE_ERROR                     = 1,
	SERVICE_STRENGTH                  = 2,
	SERVICE_FAVORITE                  = 3,
	SERVICE_AUTOCONNECT               = 4,
	SERVICE_ROAMING                   = 5,
	SERVICE_NAMESERVERS               = 6,
	SERVICE_NAMESERVERS_CONFIGURATION = 7,
	SERVICE_TIMESERVERS               = 8,
	SERVICE_TIMESERVERS_CONFIGURATION = 9,
	SERVICE_DOMAINS                   = 10,
	SERVICE_DOMAINS_CONFIGURATION     = 11,
	SERVICE_IPv4                      = 12,
	SERVICE_IPv4_CONFIGURATION        = 13,
	SERVICE_IPv6                      = 14,
	SERVICE_IPv6_CONFIGURATION        = 15,
	SERVICE_PROXY                     = 16,
	SERVICE_PROXY_CONFIGURATION       = 17,
	SERVICE_PROVIDER                  = 18,
	SERVICE_ETHERNET                  = 19,
	SERVICE_MAX                       = 20,
};

static const char *Service_updatable_properties[] = {
	"State",
	"Error",
	"Strength",
	"Favorite",
	"AutoConnect",
	"Roaming",
	"Nameservers",
	"Nameservers.Configuration",
	"Timeservers",
	"Timeservers.Configuration",
	"Domains",
	"Domains.Configuration",
	"IPv4",
	"IPv4.Configuration",
	"IPv6",
	"IPv6.Configuration",
	"Proxy",
	"Proxy.Configuration",
	"Provider",
	"Ethernet",
};

struct connman_service {
	char *path;

	enum connman_state state;
	char *error;
	char *name;
	char *type;
	char *security;

	uint8_t strength;

	gboolean favorite;
	gboolean immutable;
	gboolean autoconnect;
	gboolean roaming;

	char *nameservers;
	char *nameservers_conf;

	char *timeservers;
	char *timeservers_conf;

	char *domains;
	char *domains_conf;

	struct connman_ipv4 *ipv4;
	struct connman_ipv4 *ipv4_conf;

	struct connman_ipv6 *ipv6;
	struct connman_ipv6 *ipv6_conf;

	struct connman_proxy *proxy;
	struct connman_proxy *proxy_conf;

	struct connman_provider *provider;
	struct connman_ethernet *ethernet;

	int update_index;

	guint property_changed_wid;
	guint to_update[SERVICE_MAX];
	connman_property_changed_cb_f property_changed_cb;
	void *property_changed_user_data;

	DBusPendingCall *call_modify[SERVICE_MAX];
	guint to_error[SERVICE_MAX];
	connman_property_set_cb_f property_set_error_cb;
	void *property_set_error_user_data;
};

struct connman_service_interface {
	DBusConnection *dbus_cnx;

	gboolean refreshed;

	GHashTable *services;
	GSList *ordered_services;

	connman_path_changed_cb_f removed_cb;

	connman_refresh_cb_f refresh_services_cb;
	connman_scan_cb_f scan_services_cb;
	guint to_refresh;
	void *refresh_user_data;

	struct connman_service *selected_service;
};

static struct connman_service_interface *service_if = NULL;

static void ipv4_free(gpointer data)
{
	struct connman_ipv4 *ipv4 = data;

	if (ipv4 == NULL)
		return;

	g_free(ipv4->method);
	g_free(ipv4->address);
	g_free(ipv4->netmask);
	g_free(ipv4->gateway);

	g_free(ipv4);
}

static void ipv6_free(gpointer data)
{
	struct connman_ipv6 *ipv6 = data;

	if (ipv6 == NULL)
		return;

	g_free(ipv6->method);
	g_free(ipv6->address);
	g_free(ipv6->gateway);
	g_free(ipv6->privacy);

	g_free(ipv6);
}

static void proxy_free(gpointer data)
{
	struct connman_proxy *proxy = data;

	if (proxy == NULL)
		return;

	g_free(proxy->method);
	g_free(proxy->url);
	g_free(proxy->servers);
	g_free(proxy->excludes);

	g_free(proxy);
}

static void provider_free(gpointer data)
{
	struct connman_provider *provider = data;

	if (provider == NULL)
		return;

	g_free(provider->host);
	g_free(provider->domain);
	g_free(provider->name);
	g_free(provider->type);

	g_free(provider);
}

static void ethernet_free(gpointer data)
{
	struct connman_ethernet *ethernet = data;

	if (ethernet == NULL)
		return;

	g_free(ethernet->method);
	g_free(ethernet->interface);
	g_free(ethernet->address);
	g_free(ethernet->duplex);

	g_free(ethernet);
}

static void service_free(gpointer data)
{
	struct connman_service *service = data;
	int i;

	if (service_if != NULL && service_if->selected_service == service)
		return;

	if (service->property_changed_wid != 0)
		g_dbus_remove_watch(service_if->dbus_cnx,
					service->property_changed_wid);

	for (i = 0; i < SERVICE_MAX; i++) {
		if (service->to_update[i] != 0)
			g_source_remove(service->to_update[i]);

		if (service->call_modify[i] != 0) {
			dbus_pending_call_cancel(service->call_modify[i]);
			dbus_pending_call_unref(service->call_modify[i]);
		}

		if (service->to_error[i] != 0)
			g_source_remove(service->to_error[i]);
	}

	g_free(service->path);
	g_free(service->error);
	g_free(service->name);
	g_free(service->type);
	g_free(service->security);
	g_free(service->nameservers);
	g_free(service->nameservers_conf);
	g_free(service->timeservers);
	g_free(service->timeservers_conf);
	g_free(service->domains);
	g_free(service->domains_conf);

	ipv4_free(service->ipv4);
	ipv4_free(service->ipv4_conf);

	ipv6_free(service->ipv6);
	ipv6_free(service->ipv6_conf);

	proxy_free(service->proxy);
	proxy_free(service->proxy_conf);

	provider_free(service->provider);

	ethernet_free(service->ethernet);

	g_free(service);
}

static struct connman_service *get_service(const char *path)
{
	struct connman_service *service;

	if (path == NULL || service_if == NULL)
		return NULL;

	service = g_hash_table_lookup(service_if->services, path);
	if (service == NULL && service_if->selected_service != NULL) {
		if (g_strcmp0(path, service_if->selected_service->path) == 0)
			service = service_if->selected_service;
	}

	return service;
}

static void destroy_property(gpointer user_data)
{
	struct property_change *property = user_data;
	struct connman_service *service;

	if (property == NULL)
		return;

	service = get_service(property->path);
	if (service != NULL) {
		if (property->error != 0)
			service->to_error[property->index] = 0;
		else
			service->to_update[property->index] = 0;
	}

	g_free(property);
}

static gboolean property_changed_or_error(gpointer user_data)
{
	struct property_change *property = user_data;
	struct connman_service *service;

	service = get_service(property->path);
	if (service == NULL)
		return FALSE;

	if (property->error != 0) {
		if (service->property_set_error_cb == NULL)
			return FALSE;

		service->property_set_error_cb(property->path,
				property->name, property->error,
				service->property_set_error_user_data);
		return FALSE;
	}

	if (service->property_changed_cb != NULL) {
		service->property_changed_cb(property->path,
			property->name, service->property_changed_user_data);
	}

	return FALSE;
}

static void property_update(struct connman_service *service, int index)
{
	struct property_change *property;

	if (service->property_changed_cb == NULL)
		return;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		return;

	property->index = index;
	property->name = PROPERTY(index);
	property->path = service->path;

	if (service->to_update[index] != 0)
		g_source_remove(service->to_update[index]);

	service->to_update[index] = g_timeout_add_full(G_PRIORITY_DEFAULT,
						0, property_changed_or_error,
						property, destroy_property);
}

static struct connman_ipv4 *parse_ipv4(DBusMessageIter *arg,
						struct connman_ipv4 *ipv4)
{
	DBusMessageIter dict;
	gboolean set = FALSE;
	char *value;

	if (ipv4 == NULL) {
		ipv4 = g_try_malloc0(sizeof(struct connman_ipv4));
		if (ipv4 == NULL)
			return NULL;
	}

	dbus_message_iter_recurse(arg, &dict);

	if (cui_dbus_get_dict_entry_basic(&dict, "Method",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv4->method);
		ipv4->method = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Address",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv4->address);
		ipv4->address = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Netmask",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv4->netmask);
		ipv4->netmask = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Gateway",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv4->gateway);
		ipv4->gateway = g_strdup(value);
		set = TRUE;
	}

	if (set == TRUE)
		return ipv4;

	g_free(ipv4);
	return NULL;
}

static struct connman_ipv6 *parse_ipv6(DBusMessageIter *arg,
						struct connman_ipv6 *ipv6)
{
	uint16_t uint16_value;
	DBusMessageIter dict;
	gboolean set = FALSE;
	char *value;

	if (ipv6 == NULL) {
		ipv6 = g_try_malloc0(sizeof(struct connman_ipv6));
		if (ipv6 == NULL)
			return NULL;
	}

	dbus_message_iter_recurse(arg, &dict);

	if (cui_dbus_get_dict_entry_basic(&dict, "Method",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv6->method);
		ipv6->method = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Address",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv6->address);
		ipv6->address = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Prefix",
				DBUS_TYPE_UINT16, &uint16_value) == 0) {
		ipv6->prefix = uint16_value;
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Gateway",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv6->gateway);
		ipv6->gateway = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Privacy",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ipv6->privacy);
		ipv6->privacy = g_strdup(value);
		set = TRUE;
	}

	if (set == TRUE)
		return ipv6;

	g_free(ipv6);
	return NULL;
}

static struct connman_proxy *parse_proxy(DBusMessageIter *arg,
						struct connman_proxy *proxy)
{
	DBusMessageIter dict;
	gboolean set = FALSE;
	char *value, **array;
	int length;

	if (proxy == NULL) {
		proxy = g_try_malloc0(sizeof(struct connman_proxy));
		if (proxy == NULL)
			return NULL;
	}

	dbus_message_iter_recurse(arg, &dict);

	if (cui_dbus_get_dict_entry_basic(&dict, "Method",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(proxy->method);
		proxy->method = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Url",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(proxy->url);
		proxy->url = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_array(&dict, "Servers",
				DBUS_TYPE_STRING, &length, &array) == 0) {
		g_free(proxy->servers);
		if (array != NULL) {
			proxy->servers = g_strjoinv(";", array);
			g_free(array);
		} else
			proxy->servers = NULL;

		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_array(&dict, "Excludes",
				DBUS_TYPE_STRING, &length, &array) == 0) {
		g_free(proxy->excludes);
		if (array != NULL) {
			proxy->excludes = g_strjoinv(";", array);
			g_free(array);
		} else
			proxy->excludes = NULL;

		set = TRUE;
	}

	if (set == TRUE)
		return proxy;

	g_free(proxy);
	return NULL;
}

static struct connman_provider *parse_provider(DBusMessageIter *arg,
					struct connman_provider *provider)
{
	DBusMessageIter dict;
	gboolean set = FALSE;
	char *value;

	if (provider == NULL) {
		provider = g_try_malloc0(sizeof(struct connman_provider));
		if (provider == NULL)
			return NULL;
	}

	dbus_message_iter_recurse(arg, &dict);

	if (cui_dbus_get_dict_entry_basic(&dict, "Host",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(provider->host);
		provider->host = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Domain",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(provider->domain);
		provider->domain = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Name",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(provider->name);
		provider->name = g_strdup(value);
		set = TRUE;
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Type",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(provider->type);
		provider->type = g_strdup(value);
		set = TRUE;
	}

	if (set == TRUE)
		return provider;

	g_free(provider);
	return NULL;
}

static struct connman_ethernet *parse_ethernet(DBusMessageIter *arg,
					struct connman_ethernet *ethernet)
{
	uint16_t uint16_value;
	DBusMessageIter dict;
	char *value;

	if (ethernet == NULL) {
		ethernet = g_try_malloc0(sizeof(struct connman_ethernet));
		if (ethernet == NULL)
			return NULL;
	};

	dbus_message_iter_recurse(arg, &dict);

	if (cui_dbus_get_dict_entry_basic(&dict, "Method",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ethernet->method);
		ethernet->method = g_strdup(value);
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Interface",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ethernet->interface);
		ethernet->interface = g_strdup(value);
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "Address",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ethernet->address);
		ethernet->address = g_strdup(value);
	}

	if (cui_dbus_get_dict_entry_basic(&dict, "MTU",
				DBUS_TYPE_UINT16, &uint16_value) == 0)
		ethernet->mtu = uint16_value;


	if (cui_dbus_get_dict_entry_basic(&dict, "Speed",
				DBUS_TYPE_UINT16, &uint16_value) == 0)
		ethernet->speed = uint16_value;

	if (cui_dbus_get_dict_entry_basic(&dict, "Duplex",
					DBUS_TYPE_STRING, &value) == 0) {
		g_free(ethernet->duplex);
		ethernet->duplex = g_strdup(value);
	}

	return ethernet;
}

static bool update_service_property(DBusMessageIter *arg, void *user_data)
{
	struct connman_service *service = user_data;
	const char *name, *value;
	gboolean boolean_value;
	char **array;
	int length;

	if (cui_dbus_get_basic(arg, DBUS_TYPE_STRING, &name) != 0)
		return FALSE;

	dbus_message_iter_next(arg);

	if (g_strcmp0(name, "Name") == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);
		service->name = g_strdup(value);
	} else if (g_strcmp0(name, "Type") == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);
		service->type = g_strdup(value);
	} else if (g_strcmp0(name, "Security") == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->security);
		if (array != NULL) {
			service->security = g_strjoinv(";", array);
			g_free(array);
		} else
			service->security = NULL;
	} else if (g_strcmp0(name, "Immutable") == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &boolean_value);
		service->immutable = boolean_value;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_STATE)) == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);

		service->state = string2enum_state(value);

		service->update_index = SERVICE_STATE;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_ERROR)) == 0) {
		cui_dbus_get_basic_variant(arg, DBUS_TYPE_STRING, &value);

		g_free(service->error);
		service->error = g_strdup(value);

		service->update_index = SERVICE_ERROR;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_STRENGTH)) == 0) {
		uint8_t uint8_value;

		cui_dbus_get_basic_variant(arg, DBUS_TYPE_BYTE, &uint8_value);
		service->strength = uint8_value;

		service->update_index = SERVICE_STRENGTH;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_FAVORITE)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &boolean_value);
		service->favorite = boolean_value;

		service->update_index = SERVICE_FAVORITE;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_AUTOCONNECT)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &boolean_value);
		service->autoconnect = boolean_value;

		service->update_index = SERVICE_AUTOCONNECT;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_ROAMING)) == 0) {
		cui_dbus_get_basic_variant(arg,
				DBUS_TYPE_BOOLEAN, &boolean_value);
		service->roaming = boolean_value;

		service->update_index = SERVICE_ROAMING;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_NAMESERVERS)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->nameservers);
		if (array != NULL) {
			service->nameservers = g_strjoinv(";", array);
			g_free(array);
		} else
			service->nameservers = NULL;

		service->update_index = SERVICE_NAMESERVERS;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_NAMESERVERS_CONFIGURATION)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->nameservers_conf);
		if (array != NULL) {
			service->nameservers_conf = g_strjoinv(";", array);
			g_free(array);
		} else
			 service->nameservers_conf = NULL;

		service->update_index = SERVICE_NAMESERVERS_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_DOMAINS)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->domains);
		if (array != NULL) {
			service->domains = g_strjoinv(";", array);
			g_free(array);
		} else
			service->domains = NULL;

		service->update_index = SERVICE_DOMAINS;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_DOMAINS_CONFIGURATION)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->domains_conf);
		if (array != NULL) {
			service->domains_conf = g_strjoinv(";", array);
			g_free(array);
		} else
			service->domains_conf = NULL;

		service->update_index = SERVICE_DOMAINS_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_TIMESERVERS)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->timeservers);
		if (array != NULL) {
			service->timeservers = g_strjoinv(";", array);
			g_free(array);
		} else
			service->timeservers = NULL;

		service->update_index = SERVICE_TIMESERVERS;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_TIMESERVERS_CONFIGURATION)) == 0) {
		cui_dbus_get_array(arg, DBUS_TYPE_STRING, &length, &array);

		g_free(service->timeservers_conf);
		if (array != NULL) {
			service->timeservers_conf = g_strjoinv(";", array);
			g_free(array);
		} else
			service->timeservers_conf = NULL;

		service->update_index = SERVICE_TIMESERVERS_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_IPv4)) == 0) {
		service->ipv4 = parse_ipv4(arg, service->ipv4);
		service->update_index = SERVICE_IPv4;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_IPv4_CONFIGURATION)) == 0) {
		service->ipv4_conf = parse_ipv4(arg, service->ipv4_conf);
		service->update_index = SERVICE_IPv4_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_IPv6)) == 0) {
		service->ipv6 = parse_ipv6(arg, service->ipv6);
		service->update_index = SERVICE_IPv6;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_IPv6_CONFIGURATION)) == 0) {
		service->ipv6_conf = parse_ipv6(arg, service->ipv6_conf);
		service->update_index = SERVICE_IPv6_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_PROXY)) == 0) {
		service->proxy = parse_proxy(arg, service->proxy);
		service->update_index = SERVICE_PROXY;
	} else if (g_strcmp0(name,
			PROPERTY(SERVICE_PROXY_CONFIGURATION)) == 0) {
		service->proxy_conf = parse_proxy(arg, service->proxy_conf);
		service->update_index = SERVICE_PROXY_CONFIGURATION;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_PROVIDER)) == 0) {
		service->provider = parse_provider(arg, service->provider);
		service->update_index = SERVICE_PROVIDER;
	} else if (g_strcmp0(name, PROPERTY(SERVICE_ETHERNET)) == 0) {
		service->ethernet = parse_ethernet(arg, service->ethernet);
		service->update_index = SERVICE_ETHERNET;
	}

	return FALSE;
}

static gboolean property_changed_signal_cb(DBusConnection *dbus_cnx,
					DBusMessage *message, void *user_data)
{
	struct connman_service *service = user_data;
	DBusMessageIter arg;

	if (message == NULL)
		return TRUE;

	if (dbus_message_iter_init(message, &arg) == FALSE)
		return TRUE;

	service->update_index = SERVICE_MAX;

	update_service_property(&arg, service);

	if (service->update_index < SERVICE_MAX)
		property_update(service, service->update_index);

	return TRUE;
}

static void update_or_create_service(const char *obj_path,
					DBusMessageIter *dict)
{
	struct connman_service *service;

	if (service_if == NULL)
		return;

	service = get_service(obj_path);
	if (service == NULL) {
		service = g_try_malloc0(sizeof(struct connman_service));
		if (service == NULL)
			return;

		service->path = g_strdup(obj_path);

		g_hash_table_insert(service_if->services,
					service->path, service);

		service->property_changed_wid = g_dbus_add_signal_watch(
						service_if->dbus_cnx,
						CONNMAN_DBUS_NAME,
						service->path,
						CONNMAN_SERVICE_INTERFACE,
						"PropertyChanged",
						property_changed_signal_cb,
						service, NULL);
	}

	service_if->ordered_services = g_slist_append(
				service_if->ordered_services, service->path);

	cui_dbus_foreach_dict_entry(dict, update_service_property, service);
}

gboolean refresh_cb(gpointer data)
{
	if (service_if == NULL || service_if->refresh_services_cb == NULL)
		return FALSE;

	service_if->refresh_services_cb(service_if->refresh_user_data);

	return FALSE;
}

static void call_refresh_callback(void)
{
	if (service_if == NULL || service_if->refresh_services_cb == NULL)
		return;

	if (service_if->to_refresh != 0)
		g_source_remove(service_if->to_refresh);

	service_if->to_refresh = g_timeout_add_full(G_PRIORITY_DEFAULT,
						0, refresh_cb, NULL, NULL);
}

static void service_changed_signal_cb(DBusMessageIter *iter)
{
	DBusMessageIter array, strt;
	char *obj_path;
	int arg_type;

	if (service_if == NULL)
		return;

	g_slist_free(service_if->ordered_services);
	service_if->ordered_services = NULL;

	dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		dbus_message_iter_recurse(&array, &strt);

		cui_dbus_get_basic(&strt, DBUS_TYPE_OBJECT_PATH, &obj_path);

		dbus_message_iter_next(&strt);
		update_or_create_service(obj_path, &strt);

		dbus_message_iter_next(&array);
		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	dbus_message_iter_next(iter);
	dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		struct connman_service *service;

		cui_dbus_get_basic(&array, DBUS_TYPE_OBJECT_PATH, &obj_path);

		service = get_service(obj_path);
		if (service != NULL) {
			g_hash_table_remove(service_if->services, obj_path);
			if (service_if->removed_cb != NULL)
				service_if->removed_cb(obj_path);
		}

		dbus_message_iter_next(&array);
		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	if (service_if->refreshed == TRUE)
		call_refresh_callback();
}

static void get_services_cb(DBusMessageIter *iter)
{
	DBusMessageIter array;
	DBusMessageIter strt;
	char *obj_path;
	int arg_type;

	if (iter == NULL || service_if == NULL)
		return;

	g_slist_free(service_if->ordered_services);
	service_if->ordered_services = NULL;

	if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_ARRAY)
		return;

	dbus_message_iter_recurse(iter, &array);

	arg_type = dbus_message_iter_get_arg_type(&array);
	while (arg_type != DBUS_TYPE_INVALID) {
		dbus_message_iter_recurse(&array, &strt);

		cui_dbus_get_basic(&strt, DBUS_TYPE_OBJECT_PATH, &obj_path);

		dbus_message_iter_next(&strt);
		update_or_create_service(obj_path, &strt);

		dbus_message_iter_next(&array);
		arg_type = dbus_message_iter_get_arg_type(&array);
	}

	if (service_if->refreshed == FALSE) {
		service_if->refreshed = TRUE;
		__connman_manager_get_services(get_services_cb);
	} else
		call_refresh_callback();
}

static void scan_services_cb(void *user_data)
{
	if (service_if == NULL)
		return;

	if (service_if->scan_services_cb != NULL)
		service_if->scan_services_cb(service_if->refresh_user_data);
}

static int call_empty_method(const char *path, const char *method)
{
	struct connman_service *service;
	DBusMessage *message;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						service->path,
						CONNMAN_SERVICE_INTERFACE,
						method);
	if (message == NULL)
		return -ENOMEM;

	if (g_dbus_send_message(service_if->dbus_cnx, message) == FALSE) {
		dbus_message_unref(message);
		return -EINVAL;
	}

	return 0;
}

static void set_property_cb(DBusPendingCall *pending, void *user_data)
{
	struct property_setting *set = user_data;
	struct connman_service *service;
	struct property_change *property;
	DBusMessage *reply;
	DBusError error;

	if (dbus_pending_call_get_completed(pending) == FALSE)
		return;

	if (set == NULL)
		return;

	service = set->data;

	service->call_modify[set->index] = NULL;

	if (service->property_set_error_cb == NULL)
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

	if (service->property_set_error_cb == NULL)
		goto done;

	property = g_try_malloc0(sizeof(struct property_change));
	if (property == NULL)
		goto done;

	property->path = service->path;
	property->name = PROPERTY(set->index);
	property->error = -1;

	if (service->to_error[set->index] != 0)
		g_source_remove(service->to_error[set->index]);

	service->to_error[set->index] = g_timeout_add_full(
						G_PRIORITY_DEFAULT,
						0, property_changed_or_error,
						property, destroy_property);

done:
	dbus_message_unref(reply);
}

static void append_ipv4_config(DBusMessageIter *dict, void *data)
{
	const struct connman_ipv4 *ipv4 = data;

	cui_dbus_append_dict_entry_basic(dict, "Method",
				DBUS_TYPE_STRING, (void *) &ipv4->method);
	cui_dbus_append_dict_entry_basic(dict, "Address",
				DBUS_TYPE_STRING, (void *) &ipv4->address);
	cui_dbus_append_dict_entry_basic(dict, "Netmask",
				DBUS_TYPE_STRING, (void *) &ipv4->netmask);
	cui_dbus_append_dict_entry_basic(dict, "Gateway",
				DBUS_TYPE_STRING, (void *) &ipv4->gateway);
}

static void append_ipv6_config(DBusMessageIter *dict, void *data)
{
	const struct connman_ipv6 *ipv6 = data;

	cui_dbus_append_dict_entry_basic(dict, "Method",
				DBUS_TYPE_STRING, (void *) &ipv6->method);
	cui_dbus_append_dict_entry_basic(dict, "Address",
				DBUS_TYPE_STRING, (void *) &ipv6->address);
	cui_dbus_append_dict_entry_basic(dict, "PrefixLength",
				DBUS_TYPE_BYTE, (void *) &ipv6->prefix);
	cui_dbus_append_dict_entry_basic(dict, "Gateway",
				DBUS_TYPE_STRING, (void *) &ipv6->gateway);
	cui_dbus_append_dict_entry_basic(dict, "Privacy",
				DBUS_TYPE_STRING, (void *) &ipv6->privacy);
}

static void append_string_list(DBusMessageIter *iter, void *data)
{
	const char **string_list = data;

	for (; *string_list != NULL; string_list++)
		cui_dbus_append_basic(iter, NULL,
					DBUS_TYPE_STRING, string_list);
}

static void append_proxy_config(DBusMessageIter *dict, void *data)
{
	const struct connman_proxy *proxy = data;
	char **config_list;

	cui_dbus_append_dict_entry_basic(dict, "Method",
				DBUS_TYPE_STRING, (void *) &proxy->method);
	cui_dbus_append_dict_entry_basic(dict, "URL",
				DBUS_TYPE_STRING, (void *) &proxy->url);

	config_list = g_strsplit(proxy->servers, ";", 0);
	cui_dbus_append_dict_entry_array(dict, "Servers",
				DBUS_TYPE_STRING, append_string_list,
				(void *)config_list);
	g_strfreev(config_list);

	config_list = g_strsplit(proxy->excludes, ";", 0);
	cui_dbus_append_dict_entry_array(dict, "Excludes",
				DBUS_TYPE_STRING, append_string_list,
				(void *)config_list);
	g_strfreev(config_list);
}

static int set_service_property(struct connman_service *service,
				enum connman_service_property property,
				int dbus_type, void *data)
{
	struct property_setting *set = NULL;
	const char *property_name;
	DBusMessage *message;
	DBusMessageIter arg;

	if (connman == NULL)
		return -EINVAL;

	if (service->call_modify[property] != NULL)
		return -EINVAL;

	message = dbus_message_new_method_call(CONNMAN_DBUS_NAME,
						service->path,
						CONNMAN_SERVICE_INTERFACE,
						"SetProperty");
	if (message == NULL)
		return -ENOMEM;

	set = g_try_malloc0(sizeof(struct property_setting));
	if (set == NULL)
		goto error;

	set->data = service;
	set->index = property;

	property_name = PROPERTY(property);

	dbus_message_iter_init_append(message, &arg);

	switch (property) {
	case SERVICE_IPv4_CONFIGURATION:
		cui_dbus_append_dict(&arg, property_name,
						append_ipv4_config, data);
		break;
	case SERVICE_IPv6_CONFIGURATION:
		cui_dbus_append_dict(&arg, property_name,
						append_ipv6_config, data);
		break;
	case SERVICE_PROXY_CONFIGURATION:
		cui_dbus_append_dict(&arg, property_name,
						append_proxy_config, data);
		break;
	case SERVICE_NAMESERVERS_CONFIGURATION:
	case SERVICE_DOMAINS_CONFIGURATION:
	case SERVICE_TIMESERVERS_CONFIGURATION:
		cui_dbus_append_array(&arg, property_name,
				DBUS_TYPE_STRING, append_string_list, data);
		break;
	default:
		cui_dbus_append_basic(&arg, property_name, dbus_type, data);
		break;
	}

	if (dbus_connection_send_with_reply(service_if->dbus_cnx, message,
					&service->call_modify[property],
					DBUS_TIMEOUT_USE_DEFAULT) == FALSE)
		goto error;

	if (dbus_pending_call_set_notify(service->call_modify[property],
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

int connman_service_init(void)
{
	if (connman == NULL)
		return -EINVAL;

	if (service_if != NULL)
		return 0;

	service_if = g_try_malloc0(sizeof(struct connman_service_interface));
	if (service_if == NULL)
		return -ENOMEM;

	service_if->services = g_hash_table_new_full(g_str_hash, g_str_equal,
							NULL, service_free);
	if (service_if->services == NULL) {
		g_free(service_if);

		return -ENOMEM;
	};

	service_if->dbus_cnx = dbus_connection_ref(connman->dbus_cnx);

	return 0;
}

void connman_service_finalize(void)
{
	if (service_if == NULL)
		return;

	__connman_manager_register_service_signal(NULL);

	dbus_connection_unref(service_if->dbus_cnx);

	connman_service_deselect();

	g_slist_free(service_if->ordered_services);
	g_hash_table_destroy(service_if->services);

	g_free(service_if);

	service_if = NULL;
}

int connman_service_refresh_services_list(connman_refresh_cb_f refresh_cb,
				connman_scan_cb_f scan_cb, void *user_data)
{
	GList *techs;
	GList *list;
	char *path;

	if (service_if == NULL)
		return -EINVAL;

	service_if->refreshed = FALSE;

	techs = connman_technology_get_technologies();
	if (techs == NULL)
		return -EINVAL;

	for (list = techs; list != NULL; list = list->next) {
		const char *type;
		path = list->data;

		type = connman_technology_get_type(path);

		if (g_strcmp0(type, "wifi") == 0)
			break;

		path = NULL;
	};

	g_list_free(techs);

	service_if->refresh_services_cb = refresh_cb;
	service_if->scan_services_cb = scan_cb;
	service_if->refresh_user_data = user_data;

	if (path == NULL || connman_technology_is_enabled(path) == FALSE) {
		__connman_manager_get_services(get_services_cb);

		if (service_if->scan_services_cb != NULL)
			service_if->scan_services_cb(
						service_if->refresh_user_data);
	} else {
		__connman_manager_register_service_signal(
						service_changed_signal_cb);
		connman_technology_scan(path, scan_services_cb, NULL);

		__connman_manager_get_services(get_services_cb);
	}

	return 0;
}

GSList *connman_service_get_services(void)
{
	if (service_if == NULL || service_if->ordered_services == NULL)
		return NULL;

	return g_slist_copy(service_if->ordered_services);
}

void connman_service_free_services_list(void)
{
	if (service_if == NULL)
		return;

	__connman_manager_register_service_signal(NULL);

	g_slist_free(service_if->ordered_services);
	service_if->ordered_services = NULL;

	g_hash_table_remove_all(service_if->services);
}

void connman_service_set_property_changed_callback(const char *path,
			connman_property_changed_cb_f property_changed_cb,
			void *user_data)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return;

	service->property_changed_cb = property_changed_cb;
	service->property_changed_user_data = user_data;
}

void connman_service_set_property_error_callback(const char *path,
				connman_property_set_cb_f property_set_cb,
				void *user_data)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return;

	service->property_set_error_cb = property_set_cb;
	service->property_set_error_user_data = user_data;
}

void connman_service_set_removed_callback(
				connman_path_changed_cb_f removed_cb)
{
	if (service_if != NULL)
		service_if->removed_cb = removed_cb;
}

const char *connman_service_get_name(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->name;
}

const char *connman_service_get_type(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->type;
}

enum connman_state connman_service_get_state(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return CONNMAN_STATE_OFFLINE;

	return service->state;
}

const char *connman_service_get_error(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->error;
}

const char *connman_service_get_security(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->security;
}

uint8_t connman_service_get_strength(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return 0;

	return service->strength;
}

gboolean connman_service_is_favorite(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return FALSE;

	return service->favorite;
}

gboolean connman_service_is_immutable(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return FALSE;

	return service->immutable;
}

gboolean connman_service_is_autoconnect(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return FALSE;

	return service->autoconnect;
}

gboolean connman_service_is_roaming(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return FALSE;

	return service->roaming;
}

const char *connman_service_get_nameservers(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->nameservers;
}

const char *connman_service_get_nameservers_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->nameservers_conf;
}

const char *connman_service_get_domains(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->domains;
}

const char *connman_service_get_domains_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->domains_conf;
}

const char *connman_service_get_timeservers(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->timeservers;
}

const char *connman_service_get_timeservers_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->timeservers_conf;
}

const struct connman_ipv4 *connman_service_get_ipv4(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->ipv4;
}

const struct connman_ipv4 *connman_service_get_ipv4_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->ipv4_conf;
}

const struct connman_ipv6 *connman_service_get_ipv6(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->ipv6;
}

const struct connman_ipv6 *connman_service_get_ipv6_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->ipv6_conf;
}

const struct connman_proxy *connman_service_get_proxy(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->proxy;
}

const struct connman_proxy *connman_service_get_proxy_config(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->proxy_conf;
}

const struct connman_provider *connman_service_get_provider(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		 return NULL;

	return service->provider;
}

const struct connman_ethernet *connman_service_get_ethernet(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return NULL;

	return service->ethernet;
}

gboolean connman_service_is_connected(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return FALSE;

	if (service->state == CONNMAN_STATE_READY ||
				service->state == CONNMAN_STATE_ONLINE)
		return TRUE;

	return FALSE;
}

int connman_service_select(const char *path)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	service_if->selected_service = service;

	return 0;
}

void connman_service_deselect(void)
{
	struct connman_service *service, *service_in_ht;

	if (service_if == NULL || service_if->selected_service == NULL)
		return;

	service = service_if->selected_service;
	service_if->selected_service = NULL;

	service_in_ht = get_service(service->path);
	if (service_in_ht == NULL)
		service_free(service);
}

int connman_service_connect(const char *path)
{
	return call_empty_method(path, "Connect");
}

int connman_service_disconnect(const char *path)
{
	return call_empty_method(path, "Disconnect");
}

int connman_service_remove(const char *path)
{
	return call_empty_method(path, "Remove");
}

int connman_service_set_autoconnectable(const char *path, gboolean enable)
{
	struct connman_service *service;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	return set_service_property(service, SERVICE_AUTOCONNECT,
						DBUS_TYPE_BOOLEAN, &enable);
}

int connman_service_set_ipv4_config(const char *path,
				const struct connman_ipv4 *ipv4_config)
{
	struct connman_service *service;

	if (ipv4_config == NULL)
		return -EINVAL;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (service->ipv4_conf != NULL) {
		struct connman_ipv4 *orig = service->ipv4_conf;

		if (g_strcmp0(orig->method, ipv4_config->method) != 0 ||
				g_strcmp0(orig->address,
						ipv4_config->address) != 0 ||
				g_strcmp0(orig->netmask,
						ipv4_config->netmask) != 0 ||
				g_strcmp0(orig->gateway,
						ipv4_config->gateway) != 0)
			goto set;
		else
			return 0;
	}

set:
	return set_service_property(service,
			SERVICE_IPv4_CONFIGURATION, 0, (void *)ipv4_config);
}

int connman_service_set_ipv6_config(const char *path,
				const struct connman_ipv6 *ipv6_config)
{
	struct connman_service *service;

	if (ipv6_config == NULL)
		return -EINVAL;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (service->ipv6_conf != NULL) {
		struct connman_ipv6 *orig = service->ipv6_conf;

		if (g_strcmp0(orig->method, ipv6_config->method) != 0 ||
				g_strcmp0(orig->address,
						ipv6_config->address) != 0 ||
				g_strcmp0(orig->gateway,
						ipv6_config->gateway) != 0 ||
				g_strcmp0(orig->privacy,
						ipv6_config->privacy) != 0 ||
				orig->prefix != ipv6_config->prefix)
			goto set;
		else
			return 0;
	}

set:
	return set_service_property(service,
			SERVICE_IPv6_CONFIGURATION, 0, (void *)ipv6_config);
}

int connman_service_set_proxy_config(const char *path,
				const struct connman_proxy *proxy_config)
{
	struct connman_service *service;

	if (proxy_config == NULL)
		return -EINVAL;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (service->proxy_conf != NULL) {
		struct connman_proxy *orig = service->proxy_conf;

		if (g_strcmp0(orig->method, proxy_config->method) != 0 ||
				g_strcmp0(orig->url,
						proxy_config->url) != 0 ||
				g_strcmp0(orig->servers,
						proxy_config->servers) != 0 ||
				g_strcmp0(orig->excludes,
						proxy_config->excludes) != 0)
			goto set;
		else
			return 0;
	}

set:
	return set_service_property(service,
			SERVICE_PROXY_CONFIGURATION, 0, (void *)proxy_config);
}

int connman_service_set_nameservers_config(const char *path,
					const char *nameservers_config)
{
	struct connman_service *service;
	char **config_list;
	int ret;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (g_strcmp0(service->nameservers_conf, nameservers_config) == 0)
		return 0;

	config_list = g_strsplit(nameservers_config, ";", 0);
	if (config_list == NULL)
		return -ENOMEM;

	ret = set_service_property(service,
				SERVICE_NAMESERVERS_CONFIGURATION,
				DBUS_TYPE_STRING, (void *)config_list);
	g_strfreev(config_list);

	return ret;
}

int connman_service_set_domains_config(const char *path,
					const char *domains_config)
{
	struct connman_service *service;
	char **config_list;
	int ret;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (g_strcmp0(service->domains_conf, domains_config) == 0)
		return 0;

	config_list = g_strsplit(domains_config, ";", 0);
	if (config_list == NULL)
		return -ENOMEM;

	ret = set_service_property(service,
				SERVICE_DOMAINS_CONFIGURATION,
				DBUS_TYPE_STRING, (void *)config_list);
	g_strfreev(config_list);

	return ret;
}

int connman_service_set_timeservers_config(const char *path,
					const char *timeservers_config)
{
	struct connman_service *service;
	char **config_list;
	int ret;

	service = get_service(path);
	if (service == NULL)
		return -EINVAL;

	if (g_strcmp0(service->timeservers_conf, timeservers_config) == 0)
		return 0;

	config_list = g_strsplit(timeservers_config, ";", 0);
	if (config_list == NULL)
		return -ENOMEM;

	ret = set_service_property(service,
				SERVICE_TIMESERVERS_CONFIGURATION,
				DBUS_TYPE_STRING, (void *)config_list);
	g_strfreev(config_list);

	return ret;
}
