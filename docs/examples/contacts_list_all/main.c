/* Copyright 2008 Collabora Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <telepathy-glib/connection-manager.h>
#include <glib/gprintf.h>

GMainLoop *mainloop = NULL;

static void
on_list_connection_managers(TpConnectionManager * const *connection_manager,
                            gsize n_cms,
                            const GError *error,
                            gpointer user_data,
                            GObject *weak_object)
{
  if (error != NULL)
    {
      g_warning ("%s", error->message);
      
      /* Stop the mainloop so the program finishes: */
      g_main_loop_quit (mainloop);
      return;
    }

  g_printf ("Found %" G_GSIZE_FORMAT " connection managers:\n", n_cms);

  //TODO: See http://bugs.freedesktop.org/show_bug.cgi?id=17115
  //about the awkwardness of these pointers to pointers:
  TpConnectionManager * const *cm_iter = connection_manager;
  for (; *cm_iter != NULL; ++cm_iter)
    {
      TpConnectionManager * cm = *cm_iter;
      //TODO: The protocols really shouldn't be const.
      //const shouldn't be used for complex types in C because C doesn't have full const support.
      //For instance, g_object_get() takes a non-const, so this causes a warning:
      gchar *cm_name = NULL;

      g_object_get (G_OBJECT(cm_iter),
        "connection-manager", &cm_name,
        NULL);

      g_printf ("  Connection Manager name: %s\n", cm_name);

      g_free (cm_name);

      //TODO: See http://bugs.freedesktop.org/show_bug.cgi?id=17112
      //about the lack of real API for this:
      //Note that it's an array of pointers, not a pointer to an array
      //(unlike the connection_manager array above.)
      TpConnectionManagerProtocol * const *protocols;

      for (protocols = (TpConnectionManagerProtocol * const *)cm->protocols;
          protocols != NULL && *protocols != NULL; ++protocols)
        {
          TpConnectionManagerProtocol *protocol = *protocols;
          if (protocol->name)
            g_printf ("    Protocol name: %s\n", protocol->name);
        }

    }

  /* Stop the mainloop so the program finishes: */
  g_main_loop_quit (mainloop);
}

int
main (int argc, char **argv)
{
  g_type_init ();

  /* Create the main loop: */
  mainloop = g_main_loop_new (NULL, FALSE);

  TpDBusDaemon *bus_daemon = tp_dbus_daemon_new (tp_get_bus ());

  tp_list_connection_managers (bus_daemon, &on_list_connection_managers, 
    NULL /* user_data */, NULL /* destroy callback */, NULL);


  /* tp_list_connection_names (bus_daemon, got_connections, &data, NULL, NULL); */

  /* Start the main loop, and clean up when it finishes. */
  g_main_loop_run (mainloop);
  g_main_loop_unref (mainloop);
  g_object_unref (bus_daemon);

  return 0;
}
