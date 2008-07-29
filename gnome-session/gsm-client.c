/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Novell, Inc.
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

#include "config.h"

#include <dbus/dbus-glib.h>

#include "gsm-marshal.h"
#include "gsm-client.h"
#include "gsm-client-glue.h"

static guint32 client_serial = 1;

#define GSM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GSM_TYPE_CLIENT, GsmClientPrivate))

struct GsmClientPrivate
{
        char            *id;
        char            *startup_id;
        char            *app_id;
        int              status;
        DBusGConnection *connection;
};

enum {
        PROP_0,
        PROP_ID,
        PROP_STARTUP_ID,
        PROP_APP_ID,
        PROP_STATUS,
};

enum {
        DISCONNECTED,
        END_SESSION_RESPONSE,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_ABSTRACT_TYPE (GsmClient, gsm_client, G_TYPE_OBJECT)

static guint32
get_next_client_serial (void)
{
        guint32 serial;

        serial = client_serial++;

        if ((gint32)client_serial < 0) {
                client_serial = 1;
        }

        return serial;
}

static gboolean
register_client (GsmClient *client)
{
        GError *error;

        error = NULL;
        client->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (client->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        dbus_g_connection_register_g_object (client->priv->connection, client->priv->id, G_OBJECT (client));

        return TRUE;
}

static GObject *
gsm_client_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_properties)
{
        GsmClient *client;
        gboolean   res;

        client = GSM_CLIENT (G_OBJECT_CLASS (gsm_client_parent_class)->constructor (type,
                                                                                    n_construct_properties,
                                                                                    construct_properties));

        g_free (client->priv->id);
        client->priv->id = g_strdup_printf ("/org/gnome/SessionManager/Client%u", get_next_client_serial ());

        res = register_client (client);
        if (! res) {
                g_warning ("Unable to register client with session bus");
        }

        return G_OBJECT (client);
}

static void
gsm_client_init (GsmClient *client)
{
        client->priv = GSM_CLIENT_GET_PRIVATE (client);
}

static void
gsm_client_finalize (GObject *object)
{
        GsmClient *client;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GSM_IS_CLIENT (object));

        client = GSM_CLIENT (object);

        g_return_if_fail (client->priv != NULL);

        g_free (client->priv->id);
        g_free (client->priv->startup_id);
        g_free (client->priv->app_id);
}

void
gsm_client_set_status (GsmClient *client,
                       int        status)
{
        g_return_if_fail (GSM_IS_CLIENT (client));
        if (client->priv->status != status) {
                client->priv->status = status;
                g_object_notify (G_OBJECT (client), "status");
        }
}

static void
gsm_client_set_startup_id (GsmClient  *client,
                           const char *startup_id)
{
        g_return_if_fail (GSM_IS_CLIENT (client));

        g_free (client->priv->startup_id);

        client->priv->startup_id = g_strdup (startup_id);
        g_object_notify (G_OBJECT (client), "startup-id");
}

void
gsm_client_set_app_id (GsmClient  *client,
                       const char *app_id)
{
        g_return_if_fail (GSM_IS_CLIENT (client));

        g_free (client->priv->app_id);

        client->priv->app_id = g_strdup (app_id);
        g_object_notify (G_OBJECT (client), "app-id");
}

static void
gsm_client_set_property (GObject       *object,
                         guint          prop_id,
                         const GValue  *value,
                         GParamSpec    *pspec)
{
        GsmClient *self;

        self = GSM_CLIENT (object);

        switch (prop_id) {
        case PROP_STARTUP_ID:
                gsm_client_set_startup_id (self, g_value_get_string (value));
                break;
        case PROP_APP_ID:
                gsm_client_set_app_id (self, g_value_get_string (value));
                break;
        case PROP_STATUS:
                gsm_client_set_status (self, g_value_get_int (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gsm_client_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
        GsmClient *self;

        self = GSM_CLIENT (object);

        switch (prop_id) {
        case PROP_STARTUP_ID:
                g_value_set_string (value, self->priv->startup_id);
                break;
        case PROP_APP_ID:
                g_value_set_string (value, self->priv->app_id);
                break;
        case PROP_STATUS:
                g_value_set_int (value, self->priv->status);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static gboolean
default_stop (GsmClient *client,
              GError   **error)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), FALSE);

        g_warning ("Stop not implemented");

        return TRUE;
}

static void
gsm_client_class_init (GsmClientClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gsm_client_get_property;
        object_class->set_property = gsm_client_set_property;
        object_class->constructor = gsm_client_constructor;
        object_class->finalize = gsm_client_finalize;

        klass->impl_stop = default_stop;

        signals[DISCONNECTED] =
                g_signal_new ("disconnected",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmClientClass, disconnected),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE,
                              0);
        signals[END_SESSION_RESPONSE] =
                g_signal_new ("end-session-response",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GsmClientClass, end_session_response),
                              NULL, NULL,
                              gsm_marshal_VOID__BOOLEAN_STRING,
                              G_TYPE_NONE,
                              2, G_TYPE_BOOLEAN, G_TYPE_STRING);

        g_object_class_install_property (object_class,
                                         PROP_STARTUP_ID,
                                         g_param_spec_string ("startup-id",
                                                              "startup-id",
                                                              "startup-id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_APP_ID,
                                         g_param_spec_string ("app-id",
                                                              "app-id",
                                                              "app-id",
                                                              NULL,
                                                              G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_STATUS,
                                         g_param_spec_int ("status",
                                                           "status",
                                                           "status",
                                                           -1,
                                                           G_MAXINT,
                                                           GSM_CLIENT_UNREGISTERED,
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GsmClientPrivate));

        dbus_g_object_type_install_info (GSM_TYPE_CLIENT, &dbus_glib_gsm_client_object_info);
}

const char *
gsm_client_peek_id (GsmClient *client)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), NULL);

        return client->priv->id;
}

const char *
gsm_client_peek_app_id (GsmClient *client)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), NULL);

        return client->priv->app_id;
}

const char *
gsm_client_peek_startup_id (GsmClient *client)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), NULL);

        return client->priv->startup_id;
}

guint
gsm_client_peek_restart_style_hint (GsmClient *client)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), GSM_CLIENT_RESTART_NEVER);

        return GSM_CLIENT_GET_CLASS (client)->impl_get_restart_style_hint (client);
}


gboolean
gsm_client_get_startup_id (GsmClient *client,
                           char     **id,
                           GError   **error)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), FALSE);

        *id = g_strdup (client->priv->startup_id);

        return TRUE;
}

gboolean
gsm_client_get_app_id (GsmClient *client,
                       char     **id,
                       GError   **error)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), FALSE);

        *id = g_strdup (client->priv->app_id);

        return TRUE;
}

gboolean
gsm_client_get_restart_style_hint (GsmClient *client,
                                   guint     *hint,
                                   GError   **error)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), GSM_CLIENT_RESTART_NEVER);

        *hint = GSM_CLIENT_GET_CLASS (client)->impl_get_restart_style_hint (client);

        return TRUE;
}

char *
gsm_client_get_app_name (GsmClient *client)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), NULL);

        return GSM_CLIENT_GET_CLASS (client)->impl_get_app_name (client);
}

void
gsm_client_cancel_end_session (GsmClient *client)
{
        g_return_if_fail (GSM_IS_CLIENT (client));

        GSM_CLIENT_GET_CLASS (client)->impl_cancel_end_session (client);
}


void
gsm_client_query_end_session (GsmClient *client,
                              guint      flags)
{
        g_return_if_fail (GSM_IS_CLIENT (client));

        GSM_CLIENT_GET_CLASS (client)->impl_query_end_session (client, flags);
}

void
gsm_client_end_session (GsmClient *client,
                        guint      flags)
{
        g_return_if_fail (GSM_IS_CLIENT (client));

        GSM_CLIENT_GET_CLASS (client)->impl_end_session (client, flags);
}

gboolean
gsm_client_stop (GsmClient *client,
                 GError   **error)
{
        g_return_val_if_fail (GSM_IS_CLIENT (client), FALSE);

        return GSM_CLIENT_GET_CLASS (client)->impl_stop (client, error);
}

void
gsm_client_disconnected (GsmClient *client)
{
        g_signal_emit (client, signals[DISCONNECTED], 0);
}

void
gdm_client_end_session_response (GsmClient  *client,
                                 gboolean    is_ok,
                                 const char *reason)
{
        g_signal_emit (client, signals[END_SESSION_RESPONSE], 0, is_ok, reason);
}
