#include <glib.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/debug.h>

static GMainLoop *loop = NULL;
static TpDBusDaemon *bus_daemon = NULL;

static void
handle_error (const GError *error, TpConnection *conn)
{
	if (error)
	{
		g_print ("ERROR: %s\n", error->message);
		tp_cli_connection_call_disconnect (conn, -1, NULL,
				NULL, NULL, NULL);
	}
}

static void
conn_ready (TpConnection	*conn,
            const GError	*in_error,
	    gpointer		 user_data)
{
	g_print (" > conn_ready\n");

	handle_error (in_error, conn);

	/* disconnect */
	tp_cli_connection_call_disconnect (conn, -1, NULL, NULL, NULL, NULL);
}

static void
status_changed_cb (TpConnection	*conn,
                   guint	 status,
		   guint	 reason,
		   gpointer	 user_data,
		   GObject	*weak_object)
{
	if (status == TP_CONNECTION_STATUS_DISCONNECTED)
	{
		g_print ("Disconnected\n");
		g_main_loop_quit (loop);
	}
	else if (status == TP_CONNECTION_STATUS_CONNECTED)
	{
		g_print ("Connected\n");
	}
}

static void
new_channels_cb (TpConnection		*conn,
                 const GPtrArray	*channels,
		 gpointer		 user_data,
		 GObject		*weak_obj)
{
	g_print (" > new channels\n");
}

static void
request_connection_cb (TpConnectionManager	*cm,
                       const char		*bus_name,
		       const char		*object_path,
		       const GError		*in_error,
		       gpointer			 user_data,
		       GObject			*weak_object)
{
	GError *error = NULL;

	g_print (" > request_connection_cb (%s, %s)\n", bus_name, object_path);

	if (in_error) g_error ("%s", in_error->message);

	TpConnection *conn = tp_connection_new (bus_daemon,
			bus_name, object_path, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_call_when_ready (conn, conn_ready, NULL);

	tp_cli_connection_connect_to_status_changed (conn, status_changed_cb,
			NULL, NULL, NULL, &error);
	handle_error (error, conn);

	/*
	tp_cli_connection_interface_requests_connect_to_new_channels (conn,
			new_channels_cb, NULL, NULL, NULL, &error);
	handle_error (error, conn);
	 */

	/* initiate the connection */
	tp_cli_connection_call_connect (conn, -1, NULL, NULL, NULL, NULL);
}

static void
cm_ready (TpConnectionManager	*cm,
	  const GError		*in_error,
	  gpointer		 user_data,
	  GObject		*weak_obj)
{
	g_print (" > cm_ready\n");

	if (in_error) g_error ("%s", in_error->message);

	const TpConnectionManagerProtocol *prot = tp_connection_manager_get_protocol (cm, "jabber");
	if (!prot) g_error ("Protocol is not supported");

	/* request a new connection */
#if 0
	GHashTable *parameters = g_hash_table_new_full (
			g_str_hash, g_str_equal,
			NULL, (GDestroyNotify) tp_g_value_slice_free);
	g_hash_table_insert (parameters, "account",
			tp_g_value_slice_new_string ("davyd"));
	g_hash_table_insert (parameters, "password",
			tp_g_value_slice_new_string ("sup"));
#endif
	GHashTable *parameters = tp_asv_new (
			"account", G_TYPE_STRING, "",
			"password", G_TYPE_STRING, "",
			NULL);

	tp_cli_connection_manager_call_request_connection (cm, -1,
			"jabber",
			parameters,
			request_connection_cb,
			NULL, NULL, NULL);

	g_hash_table_destroy (parameters);
}

int
main (int argc, char **argv)
{
	GError *error = NULL;

	g_type_init ();

	/* create a main loop */
	loop = g_main_loop_new (NULL, FALSE);

	/* acquire a connection to the D-Bus daemon */
	bus_daemon = tp_dbus_daemon_dup (&error);
	if (bus_daemon == NULL)
	{
		g_error ("%s", error->message);
	}

	/* we want to request the gabble CM */
	TpConnectionManager *cm = tp_connection_manager_new (bus_daemon,
			"gabble", NULL, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_manager_call_when_ready (cm, cm_ready, NULL, NULL, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
