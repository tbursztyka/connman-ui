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

#include <stdlib.h>

#define CUI_SETTINGS_UI_PATH CUI_UI_PATH "/settings.ui"

static GtkBuilder *builder = NULL;
static GtkDialog *service_settings_dbox = NULL;
static char *path = NULL;

static gboolean ipv4_changed = FALSE;
static const char *ipv4_method = NULL;
static gboolean ipv6_changed = FALSE;
static const char *ipv6_method = NULL;
static const char *ipv6_privacy = NULL;
static gboolean proxy_changed = FALSE;
static const char *proxy_method = NULL;
static gboolean nameservers_changed = FALSE;
static gboolean domains_changed = FALSE;
static gboolean timeservers_changed = FALSE;

static inline
void toggle_button(const char *name)
{
	GtkToggleButton *button;

	button = (GtkToggleButton *) gtk_builder_get_object(builder, name);
	if (button == NULL)
		return;

	gtk_toggle_button_set_active(button, TRUE);
}

static void favorite_button_toggled(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	if (gtk_toggle_button_get_active(togglebutton) == FALSE)
		connman_service_remove(path);
}

static void autoconnect_button_toggled(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	connman_service_set_autoconnectable(path,
				gtk_toggle_button_get_active(togglebutton));
}

static void update_header(void)
{
	GdkPixbuf *image = NULL;
	const char *type, *info;
	GtkWidget *widget;
	gboolean favorite;

	type = connman_service_get_type(path);
	if (g_strcmp0(type, "wifi") == 0) {
		uint8_t strength;

		strength = connman_service_get_strength(path);

		cui_theme_get_signal_icone_and_info(strength, &image, &info);
	} else
		cui_theme_get_type_icone_and_info(type, &image, &info);

	set_image(builder, "service_type", image, info);
	set_label(builder, "service_name",
			connman_service_get_name(path), "- Hidden -");

	if (connman_service_is_connected(path) == TRUE)
		cui_theme_get_state_icone_and_info(
				connman_service_get_state(path), &image, &info);
	else
		image = NULL;

	set_image(builder, "service_state", image, info);
	set_label(builder, "service_error",
				connman_service_get_error(path), "");

	favorite = connman_service_is_favorite(path);

	set_button_toggle(builder, "service_autoconnect",
				connman_service_is_autoconnect(path));
	widget = set_widget_sensitive(builder,
				"service_autoconnect", favorite);
	if (favorite == TRUE) {
		g_signal_connect(widget, "toggled",
				G_CALLBACK(autoconnect_button_toggled), NULL);
	}

	set_button_toggle(builder, "service_favorite", favorite);
	widget = set_widget_sensitive(builder, "service_favorite", favorite);

	if (favorite == TRUE) {
		g_signal_connect(widget, "toggled",
				G_CALLBACK(favorite_button_toggled), NULL);
	}
}

static void enable_ipv4_config(gboolean enable)
{
	set_widget_sensitive(builder, "ipv4_conf_address", enable);
	set_widget_sensitive(builder, "ipv4_conf_netmask", enable);
	set_widget_sensitive(builder, "ipv4_conf_gateway", enable);
}

static void set_ipv4_method(const char *method)
{
	if (g_strcmp0(method, "dhcp") == 0) {
		toggle_button("ipv4_dhcp");
		enable_ipv4_config(FALSE);
	} else if (g_strcmp0(method, "manual") == 0) {
		toggle_button("ipv4_manual");
		enable_ipv4_config(TRUE);
	} else {
		toggle_button("ipv4_off");
		enable_ipv4_config(FALSE);
	}
}

static void update_ipv4(void)
{
	const struct connman_ipv4 *ipv4, *ipv4_conf;
	gboolean method_set = FALSE;

	ipv4 = connman_service_get_ipv4(path);
	ipv4_conf = connman_service_get_ipv4_config(path);

	if (ipv4 == NULL) {
		set_entry(builder, "ipv4_address", "", "");
		set_entry(builder, "ipv4_netmask", "", "");
		set_entry(builder, "ipv4_gateway", "", "");

		goto config;
	}

	set_ipv4_method(ipv4->method);
	method_set = TRUE;

	set_entry(builder, "ipv4_address", ipv4->address, "");
	set_entry(builder, "ipv4_netmask", ipv4->netmask, "");
	set_entry(builder, "ipv4_gateway", ipv4->gateway, "");

config:
	if (ipv4_conf == NULL) {
		set_entry(builder, "ipv4_conf_address", "", "");
		set_entry(builder, "ipv4_conf_netmask", "", "");
		set_entry(builder, "ipv4_conf_gateway", "", "");

		if (ipv4 == NULL)
			set_widget_sensitive(builder, "ipv4_settings", FALSE);

		return;
	}

	if (method_set == FALSE)
		set_ipv4_method(ipv4_conf->method);

	set_entry(builder, "ipv4_conf_address", ipv4_conf->address, "");
	set_entry(builder, "ipv4_conf_netmask", ipv4_conf->netmask, "");
	set_entry(builder, "ipv4_conf_gateway", ipv4_conf->gateway, "");

	set_widget_sensitive(builder, "ipv4_settings", TRUE);
}

static void enable_ipv6_config(gboolean enable)
{
	set_widget_sensitive(builder, "ipv6_conf_address", enable);
	set_widget_sensitive(builder, "ipv6_conf_prefix_length", enable);
	set_widget_sensitive(builder, "ipv6_conf_gateway", enable);
}

static void set_ipv6_method(const char *method)
{
	gboolean privacy_enabled = FALSE;

	if (g_strcmp0(method, "auto") == 0) {
		toggle_button("ipv6_auto");
		enable_ipv6_config(FALSE);

		privacy_enabled = TRUE;
	} else if (g_strcmp0(method, "manual") == 0) {
		toggle_button("ipv6_manual");
		enable_ipv6_config(TRUE);
	} else {
		toggle_button("ipv6_off");
		enable_ipv6_config(FALSE);
	}

	set_widget_sensitive(builder, "ipv6_priv_box", privacy_enabled);
}

static void set_ipv6_privacy(const char *privacy)
{
	if (g_strcmp0(privacy, "enabled") == 0)
		toggle_button("ipv6_priv_enabled");
	else if (g_strcmp0(privacy, "prefered") == 0)
		toggle_button("ipv6_priv_prefered");
	else
		toggle_button("ipv6_priv_disabled");
}

static void update_ipv6(void)
{
	const struct connman_ipv6 *ipv6, *ipv6_conf;
	gboolean method_set = FALSE;
	gboolean privacy_set = FALSE;
	char value[6];

	ipv6 = connman_service_get_ipv6(path);
	ipv6_conf = connman_service_get_ipv6_config(path);

	if (ipv6 == NULL) {
		set_entry(builder, "ipv6_address", "", "");
		set_entry(builder, "ipv6_prefix_length", "", "");
		set_entry(builder, "ipv6_gateway", "", "");

		goto config;
	}

	memset(value, 0, 6);
	snprintf(value, 6, "%u", ipv6->prefix);

	set_ipv6_method(ipv6->method);
	method_set = TRUE;

	set_label(builder, "ipv6_address", ipv6->address, "");
	set_label(builder, "ipv6_prefix_length", value, "");
	set_label(builder, "ipv6_gateway", ipv6->gateway, "");

	set_ipv6_privacy(ipv6->privacy);
	privacy_set = TRUE;

config:
	if (ipv6_conf == NULL) {
		set_entry(builder, "ipv6_conf_address", "", "");
		set_entry(builder, "ipv6_conf_prefix_length", "", "");
		set_entry(builder, "ipv6_conf_gateway", "", "");

		if (ipv6 == NULL)
			set_widget_sensitive(builder, "ipv6_settings", FALSE);

		return;
	}

	memset(value, 0, 6);
	snprintf(value, 6, "%u", ipv6_conf->prefix);

	if (method_set == FALSE)
		set_ipv6_method(ipv6_conf->method);

	set_entry(builder, "ipv6_conf_address", ipv6_conf->address, "");
	set_entry(builder, "ipv6_conf_prefix_length", value, "");
	set_entry(builder, "ipv6_conf_gateway", ipv6_conf->gateway, "");

	if (privacy_set == FALSE)
		set_ipv6_privacy(ipv6_conf->privacy);
}

static void update_dns(void)
{
	const char *dns, *dns_config, *domains, *domains_config;

	dns = connman_service_get_nameservers(path);
	dns_config = connman_service_get_nameservers_config(path);
	domains = connman_service_get_domains(path);
	domains_config = connman_service_get_domains_config(path);

	set_entry(builder, "nameservers", dns, "");
	set_entry(builder, "nameservers_conf", dns_config, "");
	set_entry(builder, "domains", domains, "");
	set_entry(builder, "domains_conf", domains_config, "");
}

static void update_timeservers(void)
{
	set_entry(builder, "timerservers", connman_service_get_timeservers(path), "");
	set_entry(builder, "timerservers_conf",
			connman_service_get_timeservers_config(path), "");
}

static void enable_proxy_config(gboolean enable)
{
	set_widget_sensitive(builder, "proxy_conf_url", enable);
	set_widget_sensitive(builder, "proxy_conf_servers", enable);
	set_widget_sensitive(builder, "proxy_conf_excludes", enable);
}

static void set_proxy_method(const char *method)
{
	if (g_strcmp0(method, "auto") == 0) {
		toggle_button("proxy_auto");
		enable_proxy_config(FALSE);
	} else if (g_strcmp0(method, "direct") == 0) {
		toggle_button("proxy_direct");
		enable_proxy_config(FALSE);
	} else {
		toggle_button("proxy_manual");
		enable_proxy_config(TRUE);
	}
}

static void update_proxy(void)
{
	const struct connman_proxy *proxy, *proxy_conf;
	gboolean method_set = FALSE;

	proxy = connman_service_get_proxy(path);
	proxy_conf = connman_service_get_proxy_config(path);

	if (proxy == NULL) {
		set_label(builder, "proxy_method", "", "");
		set_entry(builder, "proxy_url", "", "");
		set_entry(builder, "proxy_servers", "", "");
		set_entry(builder, "proxy_excludes", "", "");

		goto config;
	}

	set_proxy_method(proxy->method);
	method_set = TRUE;

	set_entry(builder, "proxy_url", proxy->url, "");
	set_entry(builder, "proxy_servers", proxy->servers, "");
	set_entry(builder, "proxy_excludes", proxy->excludes, "");

config:
	if (proxy_conf == NULL) {
		set_entry(builder, "proxy_conf_url", "", "");
		set_entry(builder, "proxy_conf_servers", "", "");
		set_entry(builder, "proxy_conf_excludes", "", "");

		if (proxy == NULL)
			set_widget_sensitive(builder, "proxy_settings", FALSE);

		return;
	}

	if (method_set == FALSE)
		set_proxy_method(proxy_conf->method);

	set_entry(builder, "proxy_conf_url", proxy_conf->url, "");
	set_entry(builder, "proxy_conf_servers", proxy_conf->servers, "");
	set_entry(builder, "proxy_conf_excludes", proxy_conf->excludes, "");

	set_widget_sensitive(builder, "proxy_settings", TRUE);
}

static void update_provider(void)
{
	const struct connman_provider *provider;

	provider = connman_service_get_provider(path);

	if (provider == NULL) {
		set_label(builder, "provider_host", "", "");
		set_label(builder, "provider_domain", "", "");
		set_label(builder, "provider_name", "", "");
		set_label(builder, "provider_type", "", "");

		set_widget_sensitive(builder, "provider_settings", FALSE);

		return;
	}

	set_label(builder, "provider_host", provider->host, "");
	set_label(builder, "provider_domain", provider->domain, "");
	set_label(builder, "provider_name", provider->name, "");
	set_label(builder, "provider_type", provider->type, "");

	set_widget_sensitive(builder, "provider_settings", TRUE);
}

static void update_ethernet(void)
{
	const struct connman_ethernet *ethernet;
	char value[6];

	ethernet = connman_service_get_ethernet(path);

	if (ethernet == NULL) {
		set_label(builder, "ethernet_method", "", "");
		set_label(builder, "ethernet_interface", "", "");
		set_label(builder, "ethernet_address", "", "");
		set_label(builder, "ethernet_mtu", "0", "");
		set_label(builder, "ethernet_speed", "0", "");
		set_label(builder, "ethernet_duplex", "", "");

		set_widget_sensitive(builder, "ethernet_settings", FALSE);

		return;
	}

	set_label(builder, "ethernet_method", ethernet->method, "");
	set_label(builder, "ethernet_interface", ethernet->interface, "");
	set_label(builder, "ethernet_address", ethernet->address, "");

	memset(value, 0, 6);
	snprintf(value, 6, "%u", ethernet->mtu);
	set_label(builder, "ethernet_mtu", value, "");

	memset(value, 0, 6);
	snprintf(value, 6, "%u", ethernet->speed);
	set_label(builder, "ethernet_speed", value, "");
	set_label(builder, "ethernet_duplex", ethernet->duplex, "");

	set_widget_sensitive(builder, "ethernet_settings", TRUE);
}

static void service_property_changed_cb(const char *path,
					const char *property, void *user_data)
{
	update_header();
}

static void service_property_set_cb(const char *path,
					const char *property, void *user_data)
{
	update_header();
	update_ipv4();
	update_ipv6();
	update_dns();
	update_timeservers();
	update_proxy();
	update_provider();
	update_ethernet();

	ipv4_changed = FALSE;
	ipv6_changed = FALSE;
	proxy_changed = FALSE;
	nameservers_changed = FALSE;
	domains_changed = FALSE;
	timeservers_changed = FALSE;

	connman_service_set_property_changed_callback(path,
					service_property_changed_cb, NULL);
}

static void service_property_error_cb(const char *path,
			const char *property, int error, void *user_data)
{
	printf("Error setting %s (%d)\n", property, error);

	service_property_set_cb(NULL, NULL, NULL);
}

static void toggled_ipv4_method_cb(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	const char *name = user_data;
	gboolean enable_config = FALSE;

	if (gtk_toggle_button_get_active(togglebutton) == FALSE)
		return;

	if (g_strcmp0(name, "ipv4_manual") == 0) {
		ipv4_method = "manual";
		enable_config = TRUE;
	} else if (g_strcmp0(name, "ipv4_dhcp") == 0)
		ipv4_method = "dhcp";
	else
		ipv4_method = "off";

	enable_ipv4_config(enable_config);

	ipv4_changed = TRUE;
}

static void toggled_ipv6_method_cb(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	const char *name = user_data;
	gboolean enable_config = FALSE;
	gboolean enable_privacy = FALSE;

	if (gtk_toggle_button_get_active(togglebutton) == FALSE)
		return;

	if (g_strcmp0(name, "ipv6_manual") == 0) {
		ipv6_method = "manual";
		enable_config = TRUE;
	} else if (g_strcmp0(name, "ipv6_auto") == 0) {
		ipv6_method = "auto";
		enable_privacy = TRUE;
	} else
		ipv6_method = "off";

	enable_ipv6_config(enable_config);
	set_widget_sensitive(builder, "ipv6_priv_box", enable_privacy);

	ipv6_changed = TRUE;
}

static void toggled_ipv6_privacy_cb(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	const char *name = user_data;

	if (g_strcmp0(name, "ipv6_priv_enabled") == 0)
		ipv6_privacy = "enabled";
	else if (g_strcmp0(name, "ipv6_priv_disabled") == 0)
		ipv6_privacy = "disabled";
	else
		ipv6_privacy = "prefered";

	ipv6_changed = TRUE;
}

static void toggled_proxy_method_cb(GtkToggleButton *togglebutton,
							gpointer user_data)
{
	const char *name = user_data;
	gboolean enable_config = FALSE;

	if (gtk_toggle_button_get_active(togglebutton) == FALSE)
		return;

	if (g_strcmp0(name, "proxy_manual") == 0) {
		proxy_method = "manual";
		enable_config = TRUE;
	} else if (g_strcmp0(name, "proxy_direct") == 0)
		proxy_method = "direct";
	else
		proxy_method = "auto";

	enable_proxy_config(enable_config);

	proxy_changed = TRUE;
}

static gboolean key_released_entry_cb(GtkWidget *widget,
					GdkEvent *event, gpointer user_data)
{
	gboolean *value = user_data;

	*value = TRUE;

	return TRUE;
}

static void get_ipv4_configuration(struct connman_ipv4 *ipv4)
{
	ipv4->method = (char *) ipv4_method;
	ipv4->address = (char *) get_entry_text(builder, "ipv4_conf_address");
	ipv4->netmask = (char *) get_entry_text(builder, "ipv4_conf_netmask");
	ipv4->gateway = (char *) get_entry_text(builder, "ipv4_conf_gateway");
}

static void get_ipv6_configuration(struct connman_ipv6 *ipv6)
{
	const char *prefix;

	prefix = get_entry_text(builder, "ipv6_conf_prefix_length");

	ipv6->method = (char *) ipv6_method;
	ipv6->address = (char *) get_entry_text(builder, "ipv6_conf_address");
	ipv6->prefix = atoi(prefix);
	ipv6->gateway = (char *) get_entry_text(builder, "ipv6_conf_gateway");
	ipv6->privacy = (char *) ipv6_privacy;
}

static void get_proxy_configuration(struct connman_proxy *proxy)
{
	proxy->method = (char *) proxy_method;
	proxy->url = (char *) get_entry_text(builder, "proxy_conf_url");
	proxy->servers = (char *) get_entry_text(builder, "proxy_conf_servers");
	proxy->excludes = (char *) get_entry_text(builder, "proxy_conf_excludes");
}

static void settings_cancel_callback(GtkButton *button, gpointer user_data)
{
	gtk_widget_destroy(GTK_WIDGET(service_settings_dbox));
	service_settings_dbox = NULL;

	connman_service_set_property_changed_callback(path, NULL, NULL);
	connman_service_set_property_error_callback(path, NULL, NULL);

	connman_service_deselect();

	g_free(path);
	path = NULL;

	cui_tray_enable();
}

static void settings_ok_callback(GtkButton *button, gpointer user_data)
{
	const char *value;
	int ret = 0;

	connman_service_set_property_changed_callback(path,
					service_property_set_cb, NULL);

	if (ipv4_changed == TRUE) {
		struct connman_ipv4 ipv4;

		get_ipv4_configuration(&ipv4);
		ret = connman_service_set_ipv4_config(path, &ipv4);
	} else if (ipv6_changed == TRUE) {
		struct connman_ipv6 ipv6;

		get_ipv6_configuration(&ipv6);
		ret = connman_service_set_ipv6_config(path, &ipv6);
	} else if (proxy_changed == TRUE) {
		struct connman_proxy proxy;

		get_proxy_configuration(&proxy);
		ret = connman_service_set_proxy_config(path, &proxy);
	} else if (nameservers_changed == TRUE) {
		value = get_entry_text(builder, "nameservers_conf");
		ret = connman_service_set_nameservers_config(path, value);
	} else if (domains_changed == TRUE) {
		value = get_entry_text(builder, "domains_conf");
		ret = connman_service_set_domains_config(path, value);
	} else if (timeservers_changed == TRUE) {
		value = get_entry_text(builder, "timerservers_conf");
		ret = connman_service_set_timeservers_config(path, value);
	}

	if (ret != 0)
		printf("Unable to set property, code %d\n", ret);
}

static void settings_close_callback(GtkDialog *dialog_box,
					gint response_id, gpointer user_data)
{
	if (response_id == GTK_RESPONSE_DELETE_EVENT ||
				response_id == GTK_RESPONSE_CLOSE)
		settings_cancel_callback(NULL, NULL);
}

static void settings_connect_signals(void)
{
	set_signal_callback(builder, "ipv4_dhcp", "toggled",
			G_CALLBACK(toggled_ipv4_method_cb), "ipv4_dhcp");
	set_signal_callback(builder, "ipv4_manual", "toggled",
			G_CALLBACK(toggled_ipv4_method_cb), "ipv4_manual");
	set_signal_callback(builder, "ipv4_off", "toggled",
			G_CALLBACK(toggled_ipv4_method_cb), "ipv4_off");

	set_signal_callback(builder, "ipv6_auto", "toggled",
			G_CALLBACK(toggled_ipv6_method_cb), "ipv6_auto");
	set_signal_callback(builder, "ipv6_manual", "toggled",
			G_CALLBACK(toggled_ipv6_method_cb), "ipv6_manual");
	set_signal_callback(builder, "ipv6_off", "toggled",
			G_CALLBACK(toggled_ipv6_method_cb), "ipv6_off");

	set_signal_callback(builder, "ipv4_conf_address", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv4_changed);
	set_signal_callback(builder, "ipv4_conf_netmask", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv4_changed);
	set_signal_callback(builder, "ipv4_conf_gateway", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv4_changed);

	set_signal_callback(builder, "ipv6_conf_address", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv6_changed);
	set_signal_callback(builder, "ipv6_conf_prefix_length",
			"key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv6_changed);
	set_signal_callback(builder, "ipv6_conf_gateway", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &ipv6_changed);

	set_signal_callback(builder, "ipv6_priv_prefered",
			"toggled", G_CALLBACK(toggled_ipv6_privacy_cb),
			"ipv6_priv_prefered");
	set_signal_callback(builder, "ipv6_priv_enabled",
			"toggled", G_CALLBACK(toggled_ipv6_privacy_cb),
			"ipv6_priv_enabled");
	set_signal_callback(builder, "ipv6_priv_disabled",
			"toggled", G_CALLBACK(toggled_ipv6_privacy_cb),
			"ipv6_priv_disabled");

	set_signal_callback(builder, "nameservers_conf", "key-release-event",
		G_CALLBACK(key_released_entry_cb), &nameservers_changed);
	set_signal_callback(builder, "domains_conf", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &domains_changed);
	set_signal_callback(builder, "timerservers_conf", "key-release-event",
		G_CALLBACK(key_released_entry_cb), &timeservers_changed);

	set_signal_callback(builder, "proxy_auto", "toggled",
			G_CALLBACK(toggled_proxy_method_cb), "proxy_auto");
	set_signal_callback(builder, "proxy_direct", "toggled",
			G_CALLBACK(toggled_proxy_method_cb), "proxy_direct");
	set_signal_callback(builder, "proxy_manual", "toggled",
			G_CALLBACK(toggled_proxy_method_cb), "proxy_manual");

	set_signal_callback(builder, "proxy_url", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &proxy_changed);
	set_signal_callback(builder, "proxy_servers", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &proxy_changed);
	set_signal_callback(builder, "proxy_excludes", "key-release-event",
			G_CALLBACK(key_released_entry_cb), &proxy_changed);

	set_signal_callback(builder, "settings_ok", "clicked",
				G_CALLBACK(settings_ok_callback), NULL);
	set_signal_callback(builder, "settings_close", "clicked",
				G_CALLBACK(settings_cancel_callback), NULL);

	set_signal_callback(builder, "service_settings_dbox", "response",
				G_CALLBACK(settings_close_callback), NULL);
}

gint cui_settings_popup(const char *selected_path)
{
	GError *error = NULL;

	if (builder == NULL)
		builder = gtk_builder_new();

	if (builder == NULL)
		return -ENOMEM;

	gtk_builder_add_from_file(builder, CUI_SETTINGS_UI_PATH, &error);
	if (error != NULL) {
		printf("Error: %s\n", error->message);
		g_error_free(error);

		return -EINVAL;
	}

	connman_service_select(selected_path);

	g_free(path);
	path = g_strdup(selected_path);

	service_settings_dbox = (GtkDialog *) gtk_builder_get_object(builder,
						"service_settings_dbox");

	gtk_widget_show(GTK_WIDGET(service_settings_dbox));

	connman_service_set_property_changed_callback(path,
					service_property_changed_cb, NULL);

	connman_service_set_property_error_callback(path,
					service_property_error_cb, NULL);

	settings_connect_signals();

	/* Force UI update */
	service_property_set_cb(NULL, NULL, NULL);

	cui_tray_disable();

	return 0;
}
