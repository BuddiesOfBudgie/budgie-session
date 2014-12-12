/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GSM_DBUS_CLIENT_H__
#define __GSM_DBUS_CLIENT_H__

#include "gsm-client.h"

G_BEGIN_DECLS

#define GSM_TYPE_DBUS_CLIENT            (gsm_dbus_client_get_type ())
#define GSM_DBUS_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GSM_TYPE_DBUS_CLIENT, GsmDBusClient))
#define GSM_DBUS_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GSM_TYPE_DBUS_CLIENT, GsmDBusClientClass))
#define GSM_IS_DBUS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GSM_TYPE_DBUS_CLIENT))
#define GSM_IS_DBUS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GSM_TYPE_DBUS_CLIENT))
#define GSM_DBUS_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GSM_TYPE_DBUS_CLIENT, GsmDBusClientClass))

typedef struct _GsmDBusClient        GsmDBusClient;
typedef struct _GsmDBusClientClass   GsmDBusClientClass;

typedef struct GsmDBusClientPrivate  GsmDBusClientPrivate;

struct _GsmDBusClient
{
        GsmClient             parent;
        GsmDBusClientPrivate *priv;
};

struct _GsmDBusClientClass
{
        GsmClientClass parent_class;
};

GType          gsm_dbus_client_get_type           (void) G_GNUC_CONST;

GsmClient *    gsm_dbus_client_new                (const char     *startup_id,
                                                   const char     *bus_name);
const char *   gsm_dbus_client_get_bus_name       (GsmDBusClient  *client);

G_END_DECLS

#endif /* __GSM_DBUS_CLIENT_H__ */
