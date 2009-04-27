#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
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

	GFile *file;
	GInputStream *input;
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
provide_file_cb (TpChannel	*channel,
                 const GValue	*addressv,
		 const GError	*in_error,
		 gpointer	 user_data,
		 GObject	*weak_obj)
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

	/* close the socket */
	g_socket_connection_close (ftstate->connection);
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
		GOutputStream *output = g_io_stream_get_output_stream (
				G_IO_STREAM (ftstate->connection));
		ftstate->input = G_INPUT_STREAM (
				g_file_read (ftstate->file, NULL, &error));
		handle_error (error);

		g_seekable_seek (G_SEEKABLE (ftstate->input),
				ftstate->offset, G_SEEK_SET, NULL,
				&error);
		handle_error (error);

		/* splice the input stream into the output stream and GIO
		 * takes care of the rest */
		g_output_stream_splice_async (output, ftstate->input,
				G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE,
				0, NULL,
				splice_done_cb, ftstate);
	}
	else if (state == TP_FILE_TRANSFER_STATE_COMPLETED ||
		 state == TP_FILE_TRANSFER_STATE_CANCELLED)
	{
		/* free the resources */
		g_object_unref (ftstate->connection);
		g_object_unref (ftstate->address);
		g_object_unref (ftstate->client);
		g_object_unref (ftstate->input);
		g_object_unref (ftstate->file);
		g_slice_free (struct ft_state, ftstate);
		tp_cli_channel_call_close (channel, -1, NULL, NULL, NULL, NULL);
		g_print ("Done\n");
	}
}

static void
initial_offset_defined_cb (TpChannel	*channel,
			   guint64	 offset,
			   gpointer	 user_data,
			   GObject	*weak_obj)
{
	struct ft_state *ftstate = (struct ft_state *) user_data;

	g_print (" > initial_offset_defined_cb (%llu)\n", offset);
	ftstate->offset = offset;
}

static void
file_transfer_channel_ready (TpChannel		*channel,
                             const GError	*in_error,
			     gpointer		 user_data)
{
	GFile *file = G_FILE (user_data);
	GError *error = NULL;

	handle_error (in_error);

	GHashTable *map = tp_channel_borrow_immutable_properties (channel);

	const char *filename = tp_asv_get_string (map,
			TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename");
	guint64 size = tp_asv_get_uint64 (map,
			TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size", NULL);

	g_print ("New file transfer to %s -- `%s' (%llu bytes)\n",
			tp_channel_get_identifier (channel),
			filename, size);

	/* begin ex.filetransfer.sending.providing */
	/* File transfers in Telepathy work by opening a socket to the
	 * Connection Manager and streaming the file over that socket.
	 * Let's find out what manner of sockets are supported by this CM */
	GHashTable *sockets = tp_asv_get_boxed (map,
		TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".AvailableSocketTypes",
		TP_HASH_TYPE_SUPPORTED_SOCKET_MAP);
	
	struct ft_state *ftstate = g_slice_new (struct ft_state);
	ftstate->file = file;
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

	tp_cli_channel_type_file_transfer_connect_to_initial_offset_defined (
			channel, initial_offset_defined_cb,
			ftstate, NULL, NULL, &error);
	handle_error (error);

	tp_cli_channel_type_file_transfer_connect_to_file_transfer_state_changed (
			channel, file_transfer_state_changed_cb,
			ftstate, NULL, NULL, &error);
	handle_error (error);

	/* set up the socket for providing the file */
	GValue *value = tp_g_value_slice_new_static_string ("");
	tp_cli_channel_type_file_transfer_call_provide_file (channel,
			-1, socket_type, access_control,
			value, provide_file_cb,
			ftstate, NULL, NULL);
	tp_g_value_slice_free (value);
	/* end ex.filetransfer.sending.providing */
}

static void
create_ft_channel_cb (TpConnection	*conn,
                      const char	*object_path,
		      GHashTable	*properties,
		      const GError	*in_error,
		      gpointer		 user_data,
		      GObject		*weak_obj)
{
	GFile *file = G_FILE (user_data);
	GError *error = NULL;
	handle_error (in_error);

	TpChannel *channel = tp_channel_new_from_properties (conn, object_path,
			properties, &error);
	handle_error (error);

	tp_channel_call_when_ready (channel, file_transfer_channel_ready, file);
}

static void
iterate_contacts (TpChannel	 *channel,
		  GArray	 *handles,
		  char		**argv)
{
	GError *error = NULL;
	
	GFile *file = g_file_new_for_commandline_arg (argv[3]);
	GFileInfo *info = g_file_query_info (file,
			"standard::*",
			G_FILE_QUERY_INFO_NONE,
			NULL, &error);
	handle_error (error);

	GHashTable *props = tp_asv_new (
		TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER,
		TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT, TP_HANDLE_TYPE_CONTACT,
		TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Filename", G_TYPE_STRING, g_file_info_get_display_name (info),
		TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".ContentType", G_TYPE_STRING, g_file_info_get_content_type (info),
		TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER ".Size", G_TYPE_UINT64, g_file_info_get_size (info),
		NULL);

	int i;
	for (i = 0; i < handles->len; i++)
	{
		int handle = g_array_index (handles, int, i);
		/* FIXME: we should check that our client has the
		 * FT capability */

		tp_asv_set_uint32 (props, TP_IFACE_CHANNEL ".TargetHandle",
				handle);

		tp_cli_connection_interface_requests_call_create_channel (
				conn, -1, props,
				create_ft_channel_cb,
				g_object_ref (file), NULL, NULL);
	}
		
	g_hash_table_destroy (props);
	g_object_unref (info);
	g_object_unref (file);
}

static void
group_members_changed_cb (TpChannel	 *channel,
			  char		 *message,
			  GArray	 *added,
			  GArray	 *removed,
			  GArray	 *local_pending,
			  GArray	 *remote_pending,
			  guint		  actor,
			  guint		  reason,
			  char		**argv)
{
	g_print (" :: group_members_changed_cb\n");
	g_print ("   channel contains %i new members\n", added->len);

	iterate_contacts (channel, added, argv);
}

static void
contact_list_channel_ready (TpChannel		*channel,
                            const GError	*in_error,
                            gpointer		 user_data)
{
	char **argv = (char **) user_data;
	GError *error = NULL;

	handle_error (in_error);

	g_print (" > contact_list_channel_ready\n");

	g_signal_connect (channel, "group-members-changed",
			G_CALLBACK (group_members_changed_cb), argv);

	const TpIntSet *members = tp_channel_group_get_members (channel);
	GArray *handles = tp_intset_to_array (members);
	g_print ("   channel contains %i members\n", handles->len);

	iterate_contacts (channel, handles, argv);
	g_array_free (handles, TRUE);
}

static void
create_contact_list_channel_cb (TpConnection	*conn,
                                gboolean	 yours,
                                const char	*object_path,
				GHashTable	*properties,
				const GError	*in_error,
				gpointer	 user_data,
				GObject		*weak_obj)
{
	char **argv = (char **) user_data;
	GError *error = NULL;

	handle_error (in_error);

	TpChannel *channel = tp_channel_new_from_properties (conn, object_path,
			properties, &error);
	handle_error (error);

	tp_channel_call_when_ready (channel, contact_list_channel_ready, argv);
}

static void
conn_ready (TpConnection	*conn,
            const GError	*in_error,
	    gpointer		 user_data)
{
	char **argv = (char **) user_data;
	GError *error = NULL;

	g_print (" > conn_ready\n");

	handle_error (in_error);

	/* check if the Requests interface is available */
	if (tp_proxy_has_interface_by_id (conn,
		TP_IFACE_QUARK_CONNECTION_INTERFACE_REQUESTS))
	{
		/* we need to ensure a contact list */
		GHashTable *props = tp_asv_new (
			TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
			TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT, TP_HANDLE_TYPE_LIST,
			TP_IFACE_CHANNEL ".TargetID", G_TYPE_STRING, "subscribe",
			NULL);

		tp_cli_connection_interface_requests_call_ensure_channel (
				conn, -1, props,
				create_contact_list_channel_cb,
				argv, NULL, NULL);
		g_hash_table_destroy (props);
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
	char **argv = (char **) user_data;
	GError *error = NULL;

	if (in_error) g_error ("%s", in_error->message);

	conn = tp_connection_new (bus_daemon, bus_name, object_path, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_call_when_ready (conn, conn_ready, argv);

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
			argv, NULL, NULL);

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

	if (argc != 4)
	{
		g_error ("Must provide first name, last name and filename");
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
