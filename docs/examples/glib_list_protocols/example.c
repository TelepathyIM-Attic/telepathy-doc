#include <glib.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/debug.h>

static GMainLoop *loop = NULL;

static void
got_connection_managers (TpConnectionManager	* const * cms,
			 gsize			 ncms,
			 const GError		*error,
			 gpointer		 user_data,
			 GObject		*weak_object)
{
	g_print (" > got_connection_managers\n");

	/* From the documentation:
	 *  tp_list_connection_managers() will wait for each
	 *  TpConnectionManager to become ready, so all connection managers
	 *  passed to callback will be ready */

	int i;
	for (i = 0; i < ncms; i++)
	{
		TpConnectionManager *cm = cms[i];

		if (!tp_connection_manager_is_ready (cm))
		{
			/* this should never happen, unless there is an
			 * error */
			g_print ("CM not ready!\n");
			continue;
		}

		g_print (" - %s\n", cm->name);

		/* get the protocols */
		const TpConnectionManagerProtocol * const *iter;
		for (iter = cm->protocols; iter && *iter; iter++)
		{
			const TpConnectionManagerProtocol *prot = *iter;
			g_print ("   . %s\n", prot->name);
		}
	}
}

int main (int argc, char **argv)
{
	GError *error = NULL;

	g_type_init ();

	/* create a main loop */
	loop = g_main_loop_new (NULL, FALSE);

	/* acquire a connection to the D-Bus daemon */
	TpDBusDaemon *bus_daemon = tp_dbus_daemon_dup (&error);
	if (bus_daemon == NULL)
	{
		g_error ("%s", error->message);
	}

	/* let's get a list of the connection managers */
	tp_list_connection_managers (bus_daemon, got_connection_managers,
			NULL, NULL, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
