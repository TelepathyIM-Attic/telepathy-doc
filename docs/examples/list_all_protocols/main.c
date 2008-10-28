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

/* A utility function. */
static void
list_connection_manager_protocols (TpConnectionManager * const cm, TpCMInfoSource source)
{
  /* Get the connection manager name: */
  gchar *cm_name = NULL;
  g_object_get (G_OBJECT(cm),
    "connection-manager", &cm_name,
    NULL);

  /* List the protocols implemented by this connection manager: */
  /* TODO: See http://bugs.freedesktop.org/show_bug.cgi?id=17112
   * about the lack of real API for this: */
  if(cm->protocols)
    {
      const TpConnectionManagerProtocol * const *protocols_iter = cm->protocols;
      for (; *protocols_iter != NULL; ++protocols_iter)
         {
           const TpConnectionManagerProtocol *protocol = *protocols_iter;
           if (protocol)
             {
               const gchar *source_name = "unknown";
               if (source == TP_CM_INFO_SOURCE_LIVE)
                 source_name = "introspected";
               else if (source == TP_CM_INFO_SOURCE_FILE)
                 source_name = "cached";

               if(protocol->name)
                 g_printf ("      Connection Manager: %s: Protocol name: %s (%s)\n", 
                   cm_name, protocol->name, source_name);
             }
         }
    }

  g_free (cm_name);
  cm_name = NULL;
}

/* A signal handler: */
/* See https://bugs.freedesktop.org/show_bug.cgi?id=18055 about the incorrectly-registered signal.
 */
static void
on_connection_manager_got_info (TpConnectionManager *cm,
                                TpCMInfoSource source,
                                void *user_data)
{
  /* Get the connection manager name: */
  gchar *cm_name = NULL;
  g_object_get (G_OBJECT(cm),
    "connection-manager", &cm_name,
    NULL);

  g_printf ("    Connection Manager: got-info: %s:\n", cm_name);
  /* TODO: Sometimes we only get the FILE (cached) result, but mostly we 
     get that plus a second call with the LIVE (introspected) result. Why the difference? */
  if (source !=  TP_CM_INFO_SOURCE_NONE)
   list_connection_manager_protocols (cm, source);
}

/* A callback handler. */
static void
on_list_connection_managers(TpConnectionManager * const *connection_managers,
                            gsize n_cms, /* TODO: Why do we have this if it is NULL-terminated? */
                            const GError *error,
                            gpointer user_data,
                            GObject *weak_object) /* TODO: What is this good for? */
{
  if (error)
    {
      g_warning ("%s", error->message);
      
      /* Stop the mainloop so the program finishes: */
      g_main_loop_quit (mainloop);
      return;
    }

  g_printf ("Found %" G_GSIZE_FORMAT " connection managers:\n", n_cms);

  if(!connection_managers)
    return;

  /* TODO: See http://bugs.freedesktop.org/show_bug.cgi?id=17115
   * about the awkwardness of these pointers to pointers:
   */
  TpConnectionManager * const *cm_iter = connection_managers;
  for (; *cm_iter != NULL; ++cm_iter)
    {
      TpConnectionManager *cm = *cm_iter;
      if (!cm)
        continue;

      /* TODO: See http://bugs.freedesktop.org/show_bug.cgi?id=18056
       * about the lack of get_name() being tedious.
       */
      gchar *cm_name = NULL;
      g_object_get (G_OBJECT(cm),
        "connection-manager", &cm_name,
        NULL);

      g_printf ("  Connection Manager name: %s\n", cm_name);
      g_free (cm_name);
      cm_name = NULL;

      //TODO: Usually there is no protocols information available, but sometimes 
      //there is cached information available. Why? TODO: Is that still the case with latest telepath-glib?
      //There is an always-introspect property, but setting that would only cause 
      //introspection to happen at idle time. How can we request it and wait for it.
      //
      //TODO: See mailing list discussion about this: There is a got-info signal, but how can we be sure that it hasn't been 
      //emitted before we have even had a chance to connect a signal handler here.
      //Should we create a new ConnectionManager (though we already have one) with 
      //the same name, just to be able to connect that signal early enough?

      if (cm->info_source == TP_CM_INFO_SOURCE_NONE)
        {
          g_printf("    No protocols information is available. Attempting introspection.\n");
          
          /* Request introspection:
           * We activate() the CM, triggering introspection, which will  
           * then be followed by a got-info signal.
           */

          /* TODO: See mailing list discussion about the awkwardness of the asynchronous API here:
          /* TODO: How do we know when the last signal has been emitted, so
           *  we can unreference the mainloop.
           */
          g_signal_connect (cm, "got-info",
            G_CALLBACK (on_connection_manager_got_info), mainloop);

          /* Activating a connection manager causes it to be running. */
          tp_connection_manager_activate (cm);
        }
      else
        {
           if (cm->info_source == TP_CM_INFO_SOURCE_LIVE)
             g_printf("    Introspected protocols information is available.\n");
           else if (cm->info_source == TP_CM_INFO_SOURCE_FILE)
             g_printf("    Cached protocols information is available.\n");
     
           list_connection_manager_protocols (cm, cm->info_source);
        }
    
    }

  /* Unref the mainloop so the program can finish when all references have been released: */
  /* TODO: Commented-out because we don't yet have a way to keep a reference while got-info 
   *  is being emitted.
   */
  /* g_main_loop_unref (mainloop); */
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
