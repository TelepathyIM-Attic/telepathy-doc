/*
 * main.c - creates the Observer and registers it on the bus
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *   Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <glib.h>
#include <dbus/dbus-glib.h>

#include <telepathy-glib/dbus.h>
#include <telepathy-glib/defs.h>

#include "observer.h"

#define CLIENT_NAME "Blinkenlight"

static GMainLoop *loop = NULL;

int
main (int argc, char **argv)
{
  GError *error = NULL;

  g_type_init ();

  loop = g_main_loop_new (NULL, FALSE);

  TpDBusDaemon *tpdbus = tp_dbus_daemon_dup (NULL);
  DBusGConnection *dbus = tp_get_bus ();

  Observer *observer = observer_new ();

  /* register well-known name */
  g_assert (tp_dbus_daemon_request_name (tpdbus,
      TP_CLIENT_BUS_NAME_BASE CLIENT_NAME,
      TRUE, NULL));
  /* register observer on the bus */
  dbus_g_connection_register_g_object (dbus,
      TP_CLIENT_OBJECT_PATH_BASE CLIENT_NAME,
      G_OBJECT (observer));

  g_main_loop_run (loop);
}
