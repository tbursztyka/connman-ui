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

#ifndef __CONNMAN_INTERFACE_H__
#define __CONNMAN_INTERFACE_H__

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <glib.h>

// TEMPORARY
#include <stdio.h>
//

enum connman_state {
	CONNMAN_STATE_UNKNOWN = 0,
	CONNMAN_STATE_OFFLINE = 1,
	CONNMAN_STATE_IDLE    = 2,
	CONNMAN_STATE_READY   = 3,
	CONNMAN_STATE_ONLINE  = 4,
	CONNMAN_STATE_FAILURE = 5,
};

struct connman_ipv4 {
	char *method;
	char *address;
	char *netmask;
	char *gateway;
};

struct connman_ipv6 {
	char *method;
	char *address;
	uint8_t prefix;
	char *gateway;
	char *privacy;
};

struct connman_proxy {
	char *method;
	char *url;
	char *servers;
	char *excludes;
};

struct connman_provider {
	char *host;
	char *domain;
	char *name;
	char *type;
};

struct connman_ethernet {
	char *method;
	char *interface;
	char *address;
	uint16_t mtu;
	uint16_t speed;
	char *duplex;
};

typedef void (*connman_interface_cb_f)(void *user_data);
typedef void (*connman_scan_cb_f)(void *user_data);
typedef void (*connman_refresh_cb_f)(void *user_data);
typedef void (*connman_property_changed_cb_f)(const char *path,
						const char *property,
						void *user_data);
typedef void (*connman_property_set_cb_f)(const char *path,
						const char *property,
						int error,
						void *user_data);
typedef void (*connman_path_changed_cb_f)(const char *path);

typedef void (*agent_error_cb_f)(const char *path, const char *error);
typedef void (*agent_browser_cb_f)(const char *path, const char *url);
typedef void (*agent_input_cb_f)(const char *path, gboolean hidden,
		gboolean identity, gboolean passphrase,
		const char *previous_passphrase, gboolean wpspin,
		const char *previous_wpspin, gboolean login);
typedef void (*agent_cancel_cb_f)(void);

/***********\
* Main part *
\***********/

int connman_interface_init(connman_interface_cb_f interface_connected_cb,
			connman_interface_cb_f interface_disconnected_cb,
			void *user_data);

void connman_interface_finalize(void);


/**************\
* Manager part *
\**************/

int connman_manager_init(connman_property_changed_cb_f property_changed_cb,
							void *user_data);
void connman_manager_finalize(void);

int connman_manager_set_offlinemode(gboolean offlinemode);

enum connman_state connman_manager_get_state(void);
gboolean connman_manager_get_offlinemode(void);

void connman_manager_register_agent(const char *path);
void connman_manager_unregister_agent(const char *path);

/*****************\
* Technology part *
\*****************/

int connman_technology_init(void);
void connman_technology_finalize(void);

void connman_technology_set_global_property_callback(
			connman_property_changed_cb_f property_changed_cb,
			void *user_data);
void connman_technology_set_property_changed_callback(const char *path,
			connman_property_changed_cb_f property_changed_cb,
			void *user_data);
void connman_technology_set_property_error_callback(const char *path,
				connman_property_set_cb_f property_set_cb,
				void *user_data);
void connman_technology_set_removed_callback(
					connman_path_changed_cb_f removed_cb);
void connman_technology_set_added_callback(connman_path_changed_cb_f added_cb);

GList *connman_technology_get_technologies(void);
int connman_technology_scan(const char *path,
				connman_scan_cb_f callback, void *user_data);
int connman_technology_enable(const char *path, gboolean enable);
int connman_technology_tether(const char *path, gboolean tethering);

int connman_technology_set_tethering_identifier(const char *path,
						const char *identifier);
int connman_technology_set_tethering_passphrase(const char *path,
						const char *passphrase);

const char *connman_technology_get_name(const char *path);
const char *connman_technology_get_type(const char *path);
gboolean connman_technology_is_enabled(const char *path);
gboolean connman_technology_is_tethering(const char *path);
const char *connman_technology_get_tethering_identifier(const char *path);
const char *connman_technology_get_tethering_passphrase(const char *path);


/**************\
* Service part *
\**************/

int connman_service_init(void);
void connman_service_finalize(void);

int connman_service_refresh_services_list(connman_refresh_cb_f refresh_cb,
				connman_scan_cb_f scan_cb, void *user_data);
GSList *connman_service_get_services(void);
void connman_service_free_services_list(void);
void connman_service_set_property_changed_callback(const char *path,
			connman_property_changed_cb_f property_changed_cb,
			void *user_data);
void connman_service_set_property_error_callback(const char *path,
				connman_property_set_cb_f property_set_cb,
				void *user_data);
void connman_service_set_removed_callback(
				connman_path_changed_cb_f removed_cb);

const char *connman_service_get_name(const char *path);
const char *connman_service_get_type(const char *path);
enum connman_state connman_service_get_state(const char *path);
const char *connman_service_get_error(const char *path);
const char *connman_service_get_security(const char *path);
uint8_t connman_service_get_strength(const char *path);
gboolean connman_service_is_favorite(const char *path);
gboolean connman_service_is_immutable(const char *path);
gboolean connman_service_is_autoconnect(const char *path);
gboolean connman_service_is_roaming(const char *path);
const char *connman_service_get_nameservers(const char *path);
const char *connman_service_get_nameservers_config(const char *path);
const char *connman_service_get_domains(const char *path);
const char *connman_service_get_domains_config(const char *path);
const char *connman_service_get_timeservers(const char *path);
const char *connman_service_get_timeservers_config(const char *path);
const struct connman_ipv4 *connman_service_get_ipv4(const char *path);
const struct connman_ipv4 *connman_service_get_ipv4_config(const char *path);
const struct connman_ipv6 *connman_service_get_ipv6(const char *path);
const struct connman_ipv6 *connman_service_get_ipv6_config(const char *path);
const struct connman_proxy *connman_service_get_proxy(const char *path);
const struct connman_proxy *connman_service_get_proxy_config(const char *path);
const struct connman_provider *connman_service_get_provider(const char *path);
const struct connman_ethernet *connman_service_get_ethernet(const char *path);

gboolean connman_service_is_connected(const char *path);

int connman_service_select(const char *path);
void connman_service_deselect(void);

int connman_service_connect(const char *path);
int connman_service_disconnect(const char *path);
int connman_service_remove(const char *path);
int connman_service_set_autoconnectable(const char *path, gboolean enable);
int connman_service_set_ipv4_config(const char *path,
				const struct connman_ipv4 *ipv4_config);
int connman_service_set_ipv6_config(const char *path,
				const struct connman_ipv6 *ipv6_config);
int connman_service_set_proxy_config(const char *path,
				const struct connman_proxy *proxy_config);
int connman_service_set_nameservers_config(const char *path,
					const char *nameservers_config);
int connman_service_set_domains_config(const char *path,
					const char *domains_config);
int connman_service_set_timeservers_config(const char *path,
					const char *timeservers_config);

/************\
* Agent part *
\************/

int connman_agent_init(void);
void connman_agent_finalize(void);

int connman_agent_set_error_cb(agent_error_cb_f error_cb);
int connman_agent_set_browser_cb(agent_browser_cb_f browser_cb);
int connman_agent_set_input_cb(agent_input_cb_f input_cb);
int connman_agent_set_cancel_cb(agent_cancel_cb_f cancel_cb);

void connman_agent_reply_retry(void);
void connman_agent_reply_canceled(void);
void connman_agent_reply_launch_browser(void);
int connman_agent_reply_identity(const char *identity, const char *passphrase);
int connman_agent_reply_passphrase(const char *hidden_name,
		const char *passphrase, gboolean wps, const char *wpspin);
int connman_agent_reply_login(const char *username, const char *password);

#endif /* __CONNMAN_INTERFACE_H__ */
