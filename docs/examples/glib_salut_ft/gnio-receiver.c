#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gio/gunixoutputstream.h>
#include <gio/gnio.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/debug.h>

static GMainLoop *loop = NULL;
static TpDBusDaemon *bus_daemon = NULL;
static TpConnection *conn = NULL;

struct ft_state
{
	GSocketClient *client;
	GSocketConnection *connection;
	GSocketAddress *address;

	GOutputStream *output;
	guint64 offset;
};

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

static void
accept_file_cb (TpChannel	*channel,
                const GValue	*addressv,
		const GError	*in_error,
		gpointer	 user_data,
		GObject		*weak_obj)
{
	struct ft_state *ftstate = (struct ft_state *) user_data;
	GError *error = NULL;

	handle_error (in_error);


	if (G_IS_UNIX_CLIENT (ftstate->client))
	{
		/* old versions of telepathy-salut stored this address as a
		   string rather than an 'ay'. Those versions of Salut are
		   broken */
		GArray *address = g_value_get_boxed (addressv);
		char path[address->len + 1];

		strncpy (path, address->data, address->len);
		path[address->len] = '\0';

		g_print (" > file_transfer_cb (unix:%s)\n", path);

		ftstate->address = G_SOCKET_ADDRESS (
			g_unix_socket_address_new (path));
	}
	else if (G_IS_TCP_CLIENT (ftstate->client))
	{
		GValueArray *address = g_value_get_boxed (addressv);
		const char *host = g_value_get_string (
			g_value_array_get_nth (address, 0));
		guint port = g_value_get_uint (
			g_value_array_get_nth (address, 1));
		g_print (" > file_transfer_cb (tcp:%s:%i)\n", host, port);

		GInetAddress *addr = g_inet_address_new_from_string (host);
		ftstate->address = G_SOCKET_ADDRESS (
			g_inet_socket_address_new (addr, port));
		g_object_unref (addr);
	}
}

static void
splice_done_cb (GObject		*output,
		GAsyncResult	*res,
		gpointer	 user_data)
{
	struct ft_state *ftstate = (struct ft_state *) user_data;
	GError *error = NULL;

	g_output_stream_splice_finish (G_OUTPUT_STREAM (output), res, &error);
	handle_error (error);
}

static void
file_transfer_state_changed_cb (TpChannel	*channel,
                                guint		 state,
				guint		 reason,
				gpointer	 user_data,
				GObject		*weak_obj)
{
	struct ft_state *ftstate = (struct ft_state *) user_data;
	GError *error = NULL;

	g_print (" :: file_transfer_state_changed_cb (%i)\n", state);

	if (state == TP_FILE_TRANSFER_STATE_OPEN)
	{
		ftstate->connection = g_socket_client_connect (
				ftstate->client,
				G_SOCKET_CONNECTABLE (ftstate->address),
				NULL, &error);
		handle_error (error);

		/* we can now use the stream like any other GIO stream.
		 * Open an output stream for writing to */
		GInputStream *input = g_io_stream_get_input_stream (
				G_IO_STREAM (ftstate->connection));
		ftstate->output = g_unix_output_stream_new (STDOUT_FILENO,
				FALSE);
		/* splice the input stream into the output stream and GIO
		 * takes care of the rest */
		g_output_stream_splice_async (ftstate->output, input,
				G_OUTPUT_STREAM_SPLICE_NONE, 0,
				NULL, splice_done_cb, ftstate);
	}
	else if (state == TP_FILE_TRANSFER_STATE_COMPLETED ||
		 state == TP_FILE_TRANSFER_STATE_CANCELLED)
	{
		/* close the socket */
		g_socket_connection_close (ftstate->connection);

		/* free the resources */
		g_object_unref (ftstate->output);
		g_object_unref (ftstate->connection);
		g_object_unref (ftstate->address);
		g_object_unref (ftstate->client);
		g_slice_free (struct ft_state, ftstate);
		tp_cli_channel_call_close (channel, -1, NULL, NULL, NULL, NULL);
	}
}

static void
file_transfer_channel_ready (TpChannel		*channel,
                             const GError	*in_error,
			     gpointer		 user_data)
{
	GError *error = NULL;

	handle_error (in_error);

	GHashTable *map = tp_channel_borrow_immutable_properties (channel);

	const char *filename = tp_asv_get_string (map,
			TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename");
	guint64 size = tp_asv_get_uint64 (map,
			TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size", NULL);

	g_print ("New file transfer from %s -- `%s' (%llu bytes)\n",
			tp_channel_get_identifier (channel),
			filename, size);

	/* File transfers in Telepathy work by opening a socket to the
	 * Connection Manager and streaming the file over that socket.
	 * Let's find out what manner of sockets are supported by this CM */
	GHashTable *sockets = tp_asv_get_boxed (map,
		TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".AvailableSocketTypes",
		TP_HASH_TYPE_SUPPORTED_SOCKET_MAP);

	struct ft_state *ftstate = g_slice_new (struct ft_state);
	guint socket_type, access_control;

	/* let's try for IPv4 */
	if (g_hash_table_lookup (sockets,
				GINT_TO_POINTER (TP_SOCKET_ADDRESS_TYPE_IPV4)))
	{
		socket_type = TP_SOCKET_ADDRESS_TYPE_IPV4;
		access_control = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
		ftstate->client = G_SOCKET_CLIENT (g_tcp_client_new ());
	}
	else if (g_hash_table_lookup (sockets,
				GINT_TO_POINTER (TP_SOCKET_ADDRESS_TYPE_UNIX)))
	{
		socket_type = TP_SOCKET_ADDRESS_TYPE_UNIX;
		access_control = TP_SOCKET_ACCESS_CONTROL_LOCALHOST;
		ftstate->client = G_SOCKET_CLIENT (g_unix_client_new ());
	}

	tp_cli_channel_type_file_transfer_connect_to_file_transfer_state_changed (
			channel, file_transfer_state_changed_cb,
			ftstate, NULL, NULL, &error);
	handle_error (error);

	/* let us accept the file */
	GValue *value = tp_g_value_slice_new_static_string ("");
	tp_cli_channel_type_file_transfer_call_accept_file (channel,
			-1, socket_type, access_control,
			value, 0, accept_file_cb,
			ftstate, NULL, NULL);
	tp_g_value_slice_free (value);
}

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
		int handle_type = tp_asv_get_uint32 (map,
				TP_IFACE_CHANNEL ".TargetHandleType", NULL);
		const char *id = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".TargetID");

		g_print ("New channel: %s\n", type);

		if (!strcmp (type, TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER))
		{
			/* new incoming file transfer, set up the channel */
			TpChannel *ft = tp_channel_new_from_properties (conn,
					object_path, map, &error);
			handle_error (error);

			tp_channel_call_when_ready (ft,
					file_transfer_channel_ready,
					NULL);
		}
	}
}

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
	char **argv = (char **) user_data;

	g_print (" > cm_ready\n");

	if (in_error) g_error ("%s", in_error->message);

	const TpConnectionManagerProtocol *prot = tp_connection_manager_get_protocol (cm, "local-xmpp");
	if (!prot) g_error ("Protocol is not supported");

	/* request a new connection */
	GHashTable *parameters = tp_asv_new (
			"first-name", G_TYPE_STRING, argv[1],
			"last-name", G_TYPE_STRING, argv[2],
			NULL);

	tp_cli_connection_manager_call_request_connection (cm, -1,
			"local-xmpp",
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

	if (argc != 3)
	{
		g_error ("Must provide first name and last name!");
	}

	/* create a main loop */
	loop = g_main_loop_new (NULL, FALSE);

	/* acquire a connection to the D-Bus daemon */
	bus_daemon = tp_dbus_daemon_dup (&error);
	if (bus_daemon == NULL)
	{
		g_error ("%s", error->message);
	}

	/* we want to request the salut CM */
	TpConnectionManager *cm = tp_connection_manager_new (bus_daemon,
			"salut", NULL, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_manager_call_when_ready (cm, cm_ready,
			argv, NULL, NULL);

	/* set up a signal handler */
	struct sigaction sa = { 0 };
	sa.sa_handler = interrupt_cb;
	sigaction (SIGINT, &sa, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
