/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>

#include <libintl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <glib/gi18n.h>
#include <glib/goption.h>
#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gdm-signal-handler.h"
#include "gdm-log.h"

#include "gsm-gconf.h"
#include "gsm-util.h"
#include "gsm-manager.h"
#include "gsm-xsmp-server.h"
#include "gsm-store.h"

#define GSM_GCONF_DEFAULT_SESSION_KEY           "/desktop/gnome/session/default-session"
#define GSM_GCONF_REQUIRED_COMPONENTS_DIRECTORY "/desktop/gnome/session/required-components"
#define GSM_GCONF_REQUIRED_COMPONENTS_KEY       "/desktop/gnome/session/required-components"

#define GSM_DBUS_NAME "org.gnome.SessionManager"

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

static gboolean failsafe = FALSE;
static gboolean show_version = FALSE;
static char **override_autostart_dirs = NULL;
/* FIXME: turn this off closer to release */
static gboolean debug = TRUE;

static GOptionEntry entries[] = {
        { "autostart", 'a', 0, G_OPTION_ARG_STRING_ARRAY, &override_autostart_dirs, N_("Override standard autostart directories"), NULL },
        { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },
        { "failsafe", 'f', 0, G_OPTION_ARG_NONE, &failsafe, N_("Do not load user-specified applications"), NULL },
        { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
        { NULL, 0, 0, 0, NULL, NULL, NULL }
};

static void
on_bus_name_lost (DBusGProxy *bus_proxy,
                  const char *name,
                  gpointer    data)
{
        g_warning ("Lost name on bus: %s, exiting", name);
        exit (1);
}

static gboolean
acquire_name_on_proxy (DBusGProxy *bus_proxy,
                       const char *name)
{
        GError     *error;
        guint       result;
        gboolean    res;
        gboolean    ret;

        ret = FALSE;

        if (bus_proxy == NULL) {
                goto out;
        }

        error = NULL;
        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, name,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &result,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        /* register for name lost */
        dbus_g_proxy_add_signal (bus_proxy,
                                 "NameLost",
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (bus_proxy,
                                     "NameLost",
                                     G_CALLBACK (on_bus_name_lost),
                                     NULL,
                                     NULL);


        ret = TRUE;

 out:
        return ret;
}

static gboolean
acquire_name (void)
{
        DBusGProxy      *bus_proxy;
        GError          *error;
        DBusGConnection *connection;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                gsm_util_init_error (TRUE,
                                     "Could not connect to session bus: %s",
                                     error->message);
                /* not reached */
        }

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);

        if (! acquire_name_on_proxy (bus_proxy, GSM_DBUS_NAME) ) {
                gsm_util_init_error (TRUE,
                                     "Could not acquire name on session bus: %s",
                                     error->message);
                /* not reached */
        }

        g_object_unref (bus_proxy);

        return TRUE;
}

static char *
find_desktop_file_for_app_name (const char *name,
                                char      **autostart_dirs)
{
        char     *app_path;
        char    **app_dirs;
        GKeyFile *key_file;
        char     *desktop_file;

        app_path = NULL;

        app_dirs = gsm_util_get_app_dirs ();

        key_file = g_key_file_new ();

        desktop_file = g_strdup_printf ("%s.desktop", name);
        g_key_file_load_from_dirs (key_file,
                                   desktop_file,
                                   (const char **) app_dirs,
                                   &app_path,
                                   G_KEY_FILE_NONE,
                                   NULL);

        if (app_path == NULL && autostart_dirs != NULL) {
                g_key_file_load_from_dirs (key_file,
                                           desktop_file,
                                           (const char **) autostart_dirs,
                                           &app_path,
                                           G_KEY_FILE_NONE,
                                           NULL);
        }

        /* look for gnome vender prefix */
        if (app_path == NULL) {
                g_free (desktop_file);
                desktop_file = g_strdup_printf ("gnome-%s.desktop", name);

                g_key_file_load_from_dirs (key_file,
                                           desktop_file,
                                           (const char **) app_dirs,
                                           &app_path,
                                           G_KEY_FILE_NONE,
                                           NULL);
        }

        if (app_path == NULL && autostart_dirs != NULL) {
                g_key_file_load_from_dirs (key_file,
                                           desktop_file,
                                           (const char **) autostart_dirs,
                                           &app_path,
                                           G_KEY_FILE_NONE,
                                           NULL);
        }

        g_free (desktop_file);
        g_key_file_free (key_file);

        g_strfreev (app_dirs);

        return app_path;
}

static void
append_default_apps (GsmManager *manager,
                     char      **autostart_dirs)
{
        GSList      *default_apps;
        GSList      *a;
        GConfClient *client;

        g_debug ("main: *** Adding default apps");

        client = gconf_client_get_default ();
        default_apps = gconf_client_get_list (client,
                                              GSM_GCONF_DEFAULT_SESSION_KEY,
                                              GCONF_VALUE_STRING,
                                              NULL);
        g_object_unref (client);

        for (a = default_apps; a; a = a->next) {
                char *app_path;

                if (IS_STRING_EMPTY ((char *)a->data)) {
                        continue;
                }

                app_path = find_desktop_file_for_app_name (a->data, autostart_dirs);
                if (app_path != NULL) {
                        gsm_manager_add_autostart_app (manager, app_path, NULL);
                        g_free (app_path);
                }
        }

        g_slist_foreach (default_apps, (GFunc) g_free, NULL);
        g_slist_free (default_apps);
}

static void
append_saved_session_apps (GsmManager *manager)
{
        char *session_filename;

        /* try resuming from the old gnome-session's files */
        session_filename = g_build_filename (g_get_home_dir (),
                                             ".gnome2",
                                             "session",
                                             NULL);
        if (g_file_test (session_filename, G_FILE_TEST_EXISTS)) {
                gsm_manager_add_legacy_session_apps (manager, session_filename);
                g_free (session_filename);
                return;
        }

        g_free (session_filename);
}

static void
append_required_apps (GsmManager *manager)
{
        GSList      *required_components;
        GSList      *r;
        GConfClient *client;

        client = gconf_client_get_default ();
        required_components = gconf_client_get_list (client,
                                                     GSM_GCONF_REQUIRED_COMPONENTS_KEY,
                                                     GCONF_VALUE_STRING,
                                                     NULL);

        for (r = required_components; r != NULL; r = r->next) {
                char       *path;
                char       *default_provider;
                const char *component;

                if (IS_STRING_EMPTY ((char *)r->data)) {
                        continue;
                }

                component = r->data;

                path = g_strdup_printf ("%s/%s",
                                        GSM_GCONF_REQUIRED_COMPONENTS_DIRECTORY,
                                        component);

                default_provider = gconf_client_get_string (client, path, NULL);
                if (default_provider != NULL) {
                        char *app_path;

                        app_path = find_desktop_file_for_app_name (default_provider, NULL);
                        if (app_path != NULL) {
                                gsm_manager_add_autostart_app (manager, app_path, component);
                        } else {
                                g_warning ("Unable to find provider '%s' of required component '%s'",
                                           default_provider,
                                           component);
                        }
                        g_free (app_path);
                }

                g_free (default_provider);
                g_free (path);
        }

        g_slist_foreach (required_components, (GFunc)g_free, NULL);
        g_slist_free (required_components);

        g_object_unref (client);
}

static void
load_standard_apps (GsmManager *manager)
{
        char **autostart_dirs;
        int    i;

        autostart_dirs = gsm_util_get_autostart_dirs ();

        append_default_apps (manager, autostart_dirs);

        if (failsafe) {
                goto out;
        }

        for (i = 0; autostart_dirs[i]; i++) {
                gsm_manager_add_autostart_apps_from_dir (manager, autostart_dirs[i]);
        }

        append_saved_session_apps (manager);

        /* We don't do this in the failsafe case, because the default
         * session should include all requirements anyway. */
        append_required_apps (manager);

 out:
        g_strfreev (autostart_dirs);
}

static void
load_override_apps (GsmManager *manager)
{
        int i;
        for (i = 0; override_autostart_dirs[i]; i++) {
                gsm_manager_add_autostart_apps_from_dir (manager, override_autostart_dirs[i]);
        }
}

static gboolean
signal_cb (int      signo,
           gpointer data)
{
        int ret;

        g_debug ("Got callback for signal %d", signo);

        ret = TRUE;

        switch (signo) {
        case SIGFPE:
        case SIGPIPE:
                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down abnormally.", signo);
                ret = FALSE;
                break;
        case SIGINT:
        case SIGTERM:
                /* let the fatal signals interrupt us */
                g_debug ("Caught signal %d, shutting down normally.", signo);
                ret = FALSE;
                break;
        case SIGHUP:
                g_debug ("Got HUP signal");
                ret = TRUE;
                break;
        case SIGUSR1:
                g_debug ("Got USR1 signal");
                ret = TRUE;
                gdm_log_toggle_debug ();
                break;
        default:
                g_debug ("Caught unhandled signal %d", signo);
                ret = TRUE;

                break;
        }

        return ret;
}

int
main (int argc, char **argv)
{
        struct sigaction  sa;
        GError           *error;
        char             *display_str;
        GsmManager       *manager;
        GsmStore         *client_store;
        GsmXsmpServer    *xsmp_server;
        GdmSignalHandler *signal_handler;

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        sa.sa_handler = SIG_IGN;
        sa.sa_flags = 0;
        sigemptyset (&sa.sa_mask);
        sigaction (SIGPIPE, &sa, 0);

        error = NULL;
        gtk_init_with_args (&argc, &argv,
                            (char *) _(" - the GNOME session manager"),
                            entries, GETTEXT_PACKAGE,
                            &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                exit (1);
        }

        if (show_version) {
                g_print ("%s %s\n", argv [0], VERSION);
                exit (1);
        }

        gdm_log_init ();
        gdm_log_set_debug (debug);

        signal_handler = gdm_signal_handler_new ();
        gdm_signal_handler_set_fatal_func (signal_handler, (GDestroyNotify)gtk_main_quit, NULL);
        gdm_signal_handler_add_fatal (signal_handler);
        gdm_signal_handler_add (signal_handler, SIGTERM, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGINT, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGFPE, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGHUP, signal_cb, NULL);
        gdm_signal_handler_add (signal_handler, SIGUSR1, signal_cb, NULL);

        /* Set DISPLAY explicitly for all our children, in case --display
         * was specified on the command line.
         */
        display_str = gdk_get_display ();
        gsm_util_setenv ("DISPLAY", display_str);
        g_free (display_str);

        client_store = gsm_store_new ();


        /* Start up gconfd if not already running. */
        gsm_gconf_init ();

        xsmp_server = gsm_xsmp_server_new (client_store);

        /* Now make sure they succeeded. (They'll call
         * gsm_util_init_error() if they failed.)
         */
        gsm_gconf_check ();
        acquire_name ();

        manager = gsm_manager_new (client_store, failsafe);
        if (override_autostart_dirs != NULL) {
                load_override_apps (manager);
        } else {
                load_standard_apps (manager);
        }

        gsm_xsmp_server_start (xsmp_server);
        gsm_manager_start (manager);

        gtk_main ();

        if (xsmp_server != NULL) {
                g_object_unref (xsmp_server);
        }

        if (manager != NULL) {
                g_debug ("Unreffing manager");
                g_object_unref (manager);
        }

        if (client_store != NULL) {
                g_object_unref (client_store);
        }

        gsm_gconf_shutdown ();

        gdm_log_shutdown ();

        return 0;
}
