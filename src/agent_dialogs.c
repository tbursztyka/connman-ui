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

#define CUI_AGENT_DIALOG_UI_PATH CUI_UI_PATH "/agent.ui"

static GtkDialog *input_dbox = NULL;
static GtkDialog *login_dbox = NULL;
static GtkDialog *error_dbox = NULL;
static GtkDialog *tethering_dbox = NULL;

static struct cui_selected_service service;
static char *technology = NULL;
static gboolean tethering = FALSE;

static void agent_popup_error_dbox(const char *error)
{
	GtkLabel *label;

	label = (GtkLabel *)gtk_builder_get_object(cui_builder,
							"error_message");
	gtk_label_set_text(label, error);

	gtk_widget_show(GTK_WIDGET(error_dbox));

	cui_tray_disable();
}

static void agent_error_cb(const char *path, const char *error)
{
	agent_popup_error_dbox(error);
}

static void agent_browser_cb(const char *path, const char *url)
{
	return;
}

static void agent_popup_input_dbox(gboolean hidden, gboolean passphrase,
				const char *previous_passphrase,
				gboolean wpspin, const char *previous_wpspin)
{
	GtkToggleButton *button;
	gchar *previous_value;
	GtkLabel *label;
	GtkEntry *entry;

	set_label(cui_builder, "service_label", service.name, "");

	/* Hidden settings */
	button = (GtkToggleButton *) gtk_builder_get_object(cui_builder,
							"name_button");
	gtk_widget_set_visible(GTK_WIDGET(button), hidden);
	button = (GtkToggleButton *) gtk_builder_get_object(cui_builder,
							"ssid_button");
	gtk_widget_set_visible(GTK_WIDGET(button), hidden);
	entry = (GtkEntry *) gtk_builder_get_object(cui_builder,
							"hidden_entry");
	gtk_widget_set_visible((GtkWidget *)entry, hidden);
	gtk_entry_set_text(entry, "");

	/* Passphrase/WPS settings */
	button = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"passphrase_button");
	gtk_toggle_button_set_active(button, TRUE);
	entry = (GtkEntry *)gtk_builder_get_object(cui_builder,
							"secret_entry");
	gtk_entry_set_text(entry, "");
	gtk_entry_set_visibility(entry, FALSE);
	set_widget_sensitive(cui_builder, "input_ok", FALSE);

	/* Setting WPS part */
	button = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"wpspbc_button");
	gtk_widget_set_visible(GTK_WIDGET(button), wpspin);
	gtk_toggle_button_set_active(button, FALSE);

	button = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"wpspin_button");
	gtk_widget_set_visible(GTK_WIDGET(button), FALSE); /* Disabling PIN */
	gtk_toggle_button_set_active(button, FALSE);

	label = (GtkLabel *)gtk_builder_get_object(cui_builder,
							"previous_label");
	gtk_widget_set_visible(GTK_WIDGET(label), TRUE);

	if (previous_passphrase != NULL) {
		previous_value = g_strdup_printf(_("Previous Passphrase:\n%s"),
							previous_passphrase);
		gtk_label_set_text(label, previous_value);
	}/* else if (previous_wpspin != NULL) {
		previous_value = g_strdup_printf(_("Previous WPS PIN:\n%s"),
							previous_wpspin);
		gtk_label_set_text(label, previous_value);
	}*/ else {
		gtk_label_set_text(label, "");
		gtk_widget_set_visible(GTK_WIDGET(label), FALSE);
	}

	gtk_widget_show(GTK_WIDGET(input_dbox));

	cui_tray_disable();
}

static void agent_popup_login_dbox(void)
{
	GtkEntry *entry;

	set_entry(cui_builder, "login_username", "", "");

	entry = set_entry(cui_builder, "login_password", "", "");
	gtk_entry_set_visibility(entry, FALSE);

	gtk_widget_show(GTK_WIDGET(login_dbox));

	cui_tray_disable();
}

static void agent_input_cb(const char *path, gboolean hidden,
			gboolean identity, gboolean passphrase,
			const char *previous_passphrase, gboolean wpspin,
			const char *previous_wpspin, gboolean login)
{
	if (g_strcmp0(path, service.path) != 0) {
		connman_agent_reply_canceled();
		return;
	}

	if (passphrase == TRUE || hidden == TRUE)
		agent_popup_input_dbox(hidden, passphrase,
				previous_passphrase, wpspin, previous_wpspin);
	else if (login == TRUE)
		agent_popup_login_dbox();
	else
		connman_agent_reply_canceled();
}

static void agent_cancel_cb(void)
{
	gtk_widget_hide(GTK_WIDGET(input_dbox));
	cui_tray_enable();
}

static void agent_ok_input(GtkButton *button, gpointer user_data)
{
	const char *name, *passphrase, *wpspin;
	GtkToggleButton *toggle;
	gboolean wps = FALSE;
	GtkEntry *entry;

	name = passphrase = wpspin = NULL;

	entry = (GtkEntry *)gtk_builder_get_object(cui_builder,
							"hidden_entry");
	if (gtk_entry_get_text_length(entry) > 0)
		name = gtk_entry_get_text(entry);

	entry = (GtkEntry *)gtk_builder_get_object(cui_builder,
							"secret_entry");

	toggle = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"passphrase_button");
	if (gtk_toggle_button_get_active(toggle) == TRUE)
		passphrase = gtk_entry_get_text(entry);

	toggle = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"wpspbc_button");
	if (gtk_toggle_button_get_active(toggle) == TRUE) {
		wps = TRUE;
		wpspin = "";
	}

	toggle = (GtkToggleButton *)gtk_builder_get_object(cui_builder,
							"wpspin_button");
	if (gtk_toggle_button_get_active(toggle) == TRUE) {
		wps = TRUE;
		wpspin = gtk_entry_get_text(entry);
	}

	connman_agent_reply_passphrase(name, passphrase, wps, wpspin);
	gtk_widget_hide(GTK_WIDGET(input_dbox));

	cui_tray_enable();
}

static void agent_ok_login(GtkButton *button, gpointer user_data)
{
	GtkEntry *entry;
	const char *username, *password;

	username = password = NULL;

	entry = (GtkEntry *)gtk_builder_get_object(cui_builder,
							"login_username");
	if (gtk_entry_get_text_length(entry) > 0)
		username = gtk_entry_get_text(entry);

	entry = (GtkEntry *)gtk_builder_get_object(cui_builder,
							"login_password");
	if (gtk_entry_get_text_length(entry) > 0)
		password = gtk_entry_get_text(entry);

	connman_agent_reply_login(username, password);
	gtk_widget_hide(GTK_WIDGET(login_dbox));

	cui_tray_enable();
}

static void cancel(void)
{
	cui_tray_enable();

	connman_agent_reply_canceled();
}

static void agent_cancel(GtkButton *button, gpointer user_data)
{
	GtkDialog *dialog_box = user_data;

	gtk_widget_hide(GTK_WIDGET(dialog_box));

	cancel();
}

static void agent_close(GtkDialog *dialog_box,
			gint response_id, gpointer user_data)
{
	if (response_id == GTK_RESPONSE_DELETE_EVENT ||
				response_id == GTK_RESPONSE_CLOSE)
		cancel();
}

static void agent_retry(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(error_dbox));
	cui_tray_enable();

	connman_agent_reply_retry();
}

static void agent_passphrase_changed(GtkToggleButton *togglebutton,
						gpointer user_data)
{
	const gchar *label;
	GtkWidget *entry;

	entry = (GtkWidget *)gtk_builder_get_object(cui_builder,
							"secret_entry");

	label = gtk_button_get_label(GTK_BUTTON(togglebutton));
	if (g_strcmp0(label, "WPS Push-Button") == 0)
		gtk_widget_set_sensitive(entry, FALSE);
	else
		gtk_widget_set_sensitive(entry, TRUE);
}

void cui_agent_init_callbacks(void)
{
	connman_agent_set_error_cb(agent_error_cb);
	connman_agent_set_browser_cb(agent_browser_cb);
	connman_agent_set_input_cb(agent_input_cb);
	connman_agent_set_cancel_cb(agent_cancel_cb);
}

static gboolean change_invisible_char_on_entry(GtkWidget *widget,
					GdkEvent *event, gpointer user_data)
{
	GdkEventCrossing *ev_cross = (GdkEventCrossing *) event;
	GtkEntry *entry = user_data;

	if (ev_cross->type == GDK_ENTER_NOTIFY)
		gtk_entry_set_visibility(entry, TRUE);
	else
		gtk_entry_set_visibility(entry, FALSE);

	return TRUE;
}

void cui_agent_set_selected_service(const char *path, const char *name)
{
	g_free(service.path);
	service.path = g_strdup(path);

	g_free(service.name);
	if (name == NULL)
		service.name = g_strdup("Hidden");
	else
		service.name = g_strdup(name);
}

static void agent_ok_tethering(GtkButton *button, gpointer user_data)
{
	GtkEntry *entry;

	entry = (GtkEntry *) gtk_builder_get_object(cui_builder,
							"tethering_ssid");
	connman_technology_set_tethering_identifier(technology,
						gtk_entry_get_text(entry));
	entry = (GtkEntry *) gtk_builder_get_object(cui_builder,
						"tethering_passphrase");
	connman_technology_set_tethering_passphrase(technology,
						gtk_entry_get_text(entry));

	if (tethering == TRUE)
		connman_technology_tether(technology, TRUE);

	g_free(technology);
	technology = NULL;
	tethering = FALSE;

	gtk_widget_hide(GTK_WIDGET(tethering_dbox));

	cui_tray_enable();
}

static void agent_popup_tethering_dbox(void)
{
	const char *value;
	GtkEntry *entry;

	value = connman_technology_get_tethering_identifier(technology);

	set_entry(cui_builder, "tethering_ssid", value, "");

	value = connman_technology_get_tethering_passphrase(technology);
	entry = set_entry(cui_builder, "tethering_passphrase", value, "");
	gtk_entry_set_visibility(entry, FALSE);
	if (gtk_entry_get_text_length(entry) >= 8)
		set_widget_sensitive(cui_builder, "tethering_ok", TRUE);
	else
		set_widget_sensitive(cui_builder, "tethering_ok", FALSE);

	gtk_widget_show(GTK_WIDGET(tethering_dbox));

	cui_tray_disable();
}

static gboolean secret_entry_key_release_cb(GtkWidget *widget,
					GdkEvent  *event, gpointer user_data)
{
	GtkEntry *entry = GTK_ENTRY(widget);
	GtkWidget *button = user_data;
	gboolean enable = FALSE;

	if (gtk_entry_get_text_length(entry) >= 8)
		enable = TRUE;

	gtk_widget_set_sensitive(button, enable);

	return TRUE;
}

void cui_agent_set_wifi_tethering_settings(const char *path, gboolean tether)
{
	const char *ssid, *passphrase;
	g_free(technology);

	technology = g_strdup(path);
	if (technology == NULL)
		return;

	tethering = tether;

	ssid = connman_technology_get_tethering_identifier(path);
	passphrase = connman_technology_get_tethering_passphrase(path);

	if (tethering == TRUE && (ssid == NULL || passphrase == NULL))
		agent_popup_tethering_dbox();
	else if (tethering == TRUE)
		connman_technology_tether(path, TRUE);
	else
		agent_popup_tethering_dbox();
}

gint cui_load_agent_dialogs(void)
{
	GError *error = NULL;
	GtkWidget *button, *entry;

	memset(&service, 0, sizeof(struct cui_selected_service));

	gtk_builder_add_from_file(cui_builder,
				CUI_AGENT_DIALOG_UI_PATH, &error);
	if (error != NULL) {
		printf("Error: %s\n", error->message);
		g_error_free(error);

		return -EINVAL;
	}

	input_dbox = (GtkDialog *) gtk_builder_get_object(cui_builder,
								"input_dbox");
	login_dbox = (GtkDialog *) gtk_builder_get_object(cui_builder,
								"login_dbox");
	error_dbox = (GtkDialog *) gtk_builder_get_object(cui_builder,
								"error_dbox");
	tethering_dbox = (GtkDialog *) gtk_builder_get_object(cui_builder,
							"tethering_dbox");

	/* Passphrase dbox buttons */
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"input_ok");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_ok_input), NULL);

	entry = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"secret_entry");
	g_signal_connect(entry, "enter-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);
	g_signal_connect(entry, "leave-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);
	g_signal_connect(entry, "key-release-event",
			G_CALLBACK(secret_entry_key_release_cb), button);

	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"passphrase_button");
	g_signal_connect(button, "toggled",
				G_CALLBACK(agent_passphrase_changed), NULL);
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"wpspbc_button");
	g_signal_connect(button, "toggled",
				G_CALLBACK(agent_passphrase_changed), NULL);
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"wpspin_button");
	g_signal_connect(button, "toggled",
				G_CALLBACK(agent_passphrase_changed), NULL);

	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"input_cancel");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_cancel), input_dbox);
	g_signal_connect(input_dbox, "response",
				G_CALLBACK(agent_close), NULL);
	g_signal_connect(input_dbox, "delete-event",
				G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	/* Login dbox widgets */
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"login_ok");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_ok_login), NULL);
	entry = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"login_password");
	g_signal_connect(entry, "enter-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);
	g_signal_connect(entry, "leave-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);

	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"login_cancel");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_cancel), login_dbox);
	g_signal_connect(login_dbox, "response",
				G_CALLBACK(agent_close), NULL);
	g_signal_connect(login_dbox, "delete-event",
				G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	/* Tethering dbox widgets */
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"tethering_ok");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_ok_tethering), NULL);
	entry = (GtkWidget *) gtk_builder_get_object(cui_builder,
						"tethering_passphrase");
	g_signal_connect(entry, "enter-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);
	g_signal_connect(entry, "leave-notify-event",
			G_CALLBACK(change_invisible_char_on_entry), entry);
	g_signal_connect(entry, "key-release-event",
			G_CALLBACK(secret_entry_key_release_cb), button);
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"tethering_cancel");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_cancel), tethering_dbox);
	g_signal_connect(tethering_dbox, "response",
				G_CALLBACK(agent_close), NULL);
	g_signal_connect(tethering_dbox, "delete-event",
				G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	/* Error dbox buttons */
	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"error_retry");
	g_signal_connect(button, "clicked", G_CALLBACK(agent_retry), NULL);

	button = (GtkWidget *) gtk_builder_get_object(cui_builder,
							"error_cancel");
	g_signal_connect(button, "clicked",
				G_CALLBACK(agent_cancel), error_dbox);
	g_signal_connect(error_dbox, "response",
				G_CALLBACK(agent_close), NULL);
	g_signal_connect(error_dbox, "delete-event",
				G_CALLBACK(gtk_widget_hide_on_delete), NULL);

	return 0;
}
