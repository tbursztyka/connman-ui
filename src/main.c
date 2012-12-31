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
#include <connman-interface.h>

#include <config.h>

GtkBuilder *cui_builder;

static void connman_manager_changed(const char *unused,
				const char *property, void *user_data)
{
	if (g_strcmp0(property, "State") == 0)
		cui_trayicon_update_icon();
}

static void connman_up(void *user_data)
{
	connman_manager_init(connman_manager_changed, NULL);
	connman_service_init();
	connman_technology_init();
	connman_agent_init();
	cui_agent_init_callbacks();

	cui_trayicon_update_icon();

	cui_tray_enable();
	cui_right_menu_enable_all();
}

static void connman_down(void *user_data)
{
	connman_agent_finalize();
	connman_service_finalize();
	connman_technology_finalize();
	connman_manager_finalize();

	cui_trayicon_update_icon();

	cui_tray_left_menu_disable();
	cui_right_menu_enable_only_quit();
}

int main(int argc, char *argv[])
{
	int ret;

	setlocale(LC_ALL, "");

	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	textdomain(GETTEXT_PACKAGE);

	printf("%s\n", GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);

	cui_builder = gtk_builder_new();
	if (cui_builder == NULL)
		return -ENOMEM;

	cui_load_theme();

	if (cui_load_trayicon(cui_builder) != 0)
		return -EINVAL;

	if (cui_load_agent_dialogs() != 0)
		return -EINVAL;

	cui_tray_enable();

	ret = connman_interface_init(connman_up, connman_down, NULL);
	if (ret < 0)
		return ret;

	gtk_main();

	connman_agent_finalize();
	connman_service_finalize();
	connman_technology_finalize();
	connman_manager_finalize();
	connman_interface_finalize();

	return 0;
}

