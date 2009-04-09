#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/debug.h>

static GMainLoop *loop = NULL;
static TpDBusDaemon *bus_daemon = NULL;
static TpConnection *conn = NULL;

static void
handle_error (const GError *error)
{
	if (error)
	{
		g_print ("ERROR: %s\n", error->message);
		tp_cli_connection_call_disconnect (conn, -1, NULL,
				NULL, NULL, NULL);
	}
}

/* begin ex.channel.contactlist.user-defined.glib */
static void
new_channels_cb (TpConnection		*conn,
                 const GPtrArray	*channels,
		 gpointer		 user_data,
		 GObject		*weak_obj)
{
	GError *error = NULL;

	/* channels has the D-Bus type a(oa{sv}), which decomposes to:
	 *  - a GPtrArray containing a GValueArray for each channel
	 *  - each GValueArray contains
	 *     - an object path
	 *     - an a{sv} map
	 */

	int i;
	for (i = 0; i < channels->len; i++)
	{
		GValueArray *channel = g_ptr_array_index (channels, i);
		char *object_path = g_value_get_boxed (
				g_value_array_get_nth (channel, 0));
		GHashTable *map = g_value_get_boxed (
				g_value_array_get_nth (channel, 1));

		const char *type = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".ChannelType");
		const char *id = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".TargetID");

		g_print ("New channel %s: %s\n", type, id);
	}
}
/* end ex.channel.contactlist.user-defined.glib */

static void
get_channels_cb (TpProxy	*proxy,
		 const GValue	*value,
		 const GError	*in_error,
		 gpointer	 user_data,
		 GObject	*weak_obj)
{
	handle_error (in_error);

	g_return_if_fail (G_VALUE_HOLDS (value,
				TP_ARRAY_TYPE_CHANNEL_DETAILS_LIST));

	GPtrArray *channels = g_value_get_boxed (value);

	new_channels_cb (conn, channels, user_data, weak_obj);
}

static void
create_roomlist_cb (TpConnection	*conn,
		    const char		*object_path,
		    GHashTable		*props,
		    const GError	*in_error,
		    gpointer		 user_data,
		    GObject		*weak_obj)
{
	handle_error (in_error);

	g_print (" > create_roomlist_cb (%s)\n", object_path);
}

static void
conn_ready (TpConnection	*conn,
            const GError	*in_error,
	    gpointer		 user_data)
{
	GError *error = NULL;

	g_print (" > conn_ready\n");

	handle_error (in_error);

	/* check if the Requests interface is available */
	if (tp_proxy_has_interface_by_id (conn,
		TP_IFACE_QUARK_CONNECTION_INTERFACE_REQUESTS))
	{
		/* request the current channels */
		tp_cli_dbus_properties_call_get (conn, -1,
				TP_IFACE_CONNECTION_INTERFACE_REQUESTS,
				"Channels",
				get_channels_cb,
				NULL, NULL, NULL);

		/* notify of all new channels */
		tp_cli_connection_interface_requests_connect_to_new_channels (
				conn, new_channels_cb,
				NULL, NULL, NULL, &error);
		handle_error (error);

		/* begin example.channel.roomlist.requestglib */
		/* request a RoomList channel */
		GHashTable *map = tp_asv_new (
			TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_ROOM_LIST,
			TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT, TP_HANDLE_TYPE_NONE,
			/* we omit TargetHandle because it's anonymous */
			NULL);

		tp_cli_connection_interface_requests_call_create_channel (
				conn, -1, map,
				create_roomlist_cb,
				NULL, NULL, NULL);

		g_hash_table_destroy (map);
		/* end example.channel.roomlist.requestglib */
	}
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
request_connection_cb (TpConnectionManager	*cm,
                       const char		*bus_name,
		       const char		*object_path,
		       const GError		*in_error,
		       gpointer			 user_data,
		       GObject			*weak_object)
{
	GError *error = NULL;

	if (in_error) g_error ("%s", in_error->message);

	conn = tp_connection_new (bus_daemon, bus_name, object_path, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_call_when_ready (conn, conn_ready, NULL);

	tp_cli_connection_connect_to_status_changed (conn, status_changed_cb,
			NULL, NULL, NULL, &error);
	handle_error (error);

	/* initiate the connection */
	tp_cli_connection_call_connect (conn, -1, NULL, NULL, NULL, NULL);
}

static void
cm_ready (TpConnectionManager	*cm,
	  const GError		*in_error,
	  gpointer		 user_data,
	  GObject		*weak_obj)
{
	char *username = (char *) user_data;

	g_print (" > cm_ready\n");

	if (in_error) g_error ("%s", in_error->message);

	const TpConnectionManagerProtocol *prot = tp_connection_manager_get_protocol (cm, "jabber");
	if (!prot) g_error ("Protocol is not supported");

	/* list the parameters */
	TpConnectionManagerParam *ptr;
	for (ptr = prot->params; ptr->name; ptr++)
	{
		g_print (" - %s (%s)\n", ptr->name, ptr->dbus_signature);
	}

	char *password = getpass ("Password: ");

	/* request a new connection */
	GHashTable *parameters = tp_asv_new (
			"account", G_TYPE_STRING, username,
			"password", G_TYPE_STRING, password,
			"require-encryption", G_TYPE_BOOLEAN, TRUE,
			"ignore-ssl-errors", G_TYPE_BOOLEAN, TRUE,
			NULL);

	tp_cli_connection_manager_call_request_connection (cm, -1,
			"jabber",
			parameters,
			request_connection_cb,
			NULL, NULL, NULL);

	g_hash_table_destroy (parameters);
}

static void
interrupt_cb (int signal)
{
	g_print ("Interrupt\n");
	/* disconnect */
	tp_cli_connection_call_disconnect (conn, -1, NULL, NULL, NULL, NULL);
}

int
main (int argc, char **argv)
{
	GError *error = NULL;

	g_type_init ();

	if (argc != 2)
	{
		g_error ("Must provide username!");
	}
	char *username = argv[1];

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

	tp_connection_manager_call_when_ready (cm, cm_ready,
			username, NULL, NULL);

	/* set up a signal handler */
	struct sigaction sa = { 0 };
	sa.sa_handler = interrupt_cb;
	sigaction (SIGINT, &sa, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
