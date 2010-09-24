#include "connections-monitor.h"


static void
_got_connection (ConnectionsMonitor *monitor,
    TpAccount *account,
    TpConnection *conn)
{
  g_debug ("GOT CONNECTION:\n\t%s\n\t%s",
      tp_proxy_get_object_path (account),
      tp_proxy_get_object_path (conn));
}


int
main (int argc,
    const char **argv)
{
  ConnectionsMonitor *monitor;
  GMainLoop *loop;

  g_type_init ();

  monitor = connections_monitor_new ();
  g_signal_connect (monitor, "connection",
      G_CALLBACK (_got_connection), NULL);

  loop = g_main_loop_new (NULL, FALSE);

  /* run the program */
  g_main_loop_run (loop);

  g_main_loop_unref (loop);
  g_object_unref (monitor);
}
