#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/util.h>
#include <telepathy-glib/gnio-util.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/debug.h>

static GMainLoop *loop = NULL;
static TpDBusDaemon *bus_daemon = NULL;
static TpConnection *conn = NULL;

static GSocketAddress *sockaddr = NULL;

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
tube_state_changed_cb (TpChannel *channel,
		       guint      state,
		       gpointer   user_data,
		       GObject   *weak_obj)
{
	g_print ("Tube state changed %i\n", state);

	if (state == TP_TUBE_CHANNEL_STATE_OPEN)
	{
		g_print ("Tube open\n");
	}
}

static void
tube_accept_cb (TpChannel	*channel,
	        const GValue	*address,
	        const GError	*in_error,
	        gpointer	 user_data,
	        GObject		*weak_obj)
{
	GError *error = NULL;

	handle_error (in_error);

	g_print ("variant type = %s\n", G_VALUE_TYPE_NAME (address));
	sockaddr = tp_g_socket_address_from_variant (
			TP_SOCKET_ADDRESS_TYPE_IPV4,
			address);

	/* FIXME: I _think_ the spec says you need to wait for state Open and 
	 * this callback -- seeking spec clarification */
	GSocketClient *client = g_socket_client_new ();
	GSocketConnection *conn = g_socket_client_connect (client,
			G_SOCKET_CONNECTABLE (sockaddr),
			NULL, &error);
	handle_error (error);
}

static void
channel_ready (TpChannel	*channel,
	       const GError	*in_error,
	       gpointer		 user_data)
{
	GError *error = NULL;
	g_print (" > channel_ready (%s)\n",
			tp_channel_get_identifier (channel));

	tp_cli_channel_interface_tube_connect_to_tube_channel_state_changed (
			channel, tube_state_changed_cb,
			NULL, NULL, NULL, &error);
	handle_error (error);

	/* accept the channel */
	/* begin ex.tubes.stream.accept.noop */
	GValue noop = { 0, };
	g_value_init (&noop, G_TYPE_INT);
	tp_cli_channel_type_stream_tube_call_accept (channel, -1,
			TP_SOCKET_ADDRESS_TYPE_IPV4,
			TP_SOCKET_ACCESS_CONTROL_LOCALHOST, &noop,
			tube_accept_cb, NULL, NULL, NULL);
	g_value_unset (&noop);
	/* end ex.tubes.stream.accept.noop */
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
		guint handle_type = tp_asv_get_uint32 (map,
				TP_IFACE_CHANNEL ".TargetHandleType", NULL);

		/* if this channel is a contact list, we want to know
		 * about it */
		if (!strcmp (type, TP_IFACE_CHANNEL_TYPE_STREAM_TUBE))
		{
			g_print ("New Stream Tube channel created\n");
			tp_asv_dump (map);

			TpChannel *channel = tp_channel_new_from_properties (
					conn, object_path, map,
					&error);
			handle_error (error);

			tp_channel_call_when_ready (channel,
					channel_ready, NULL);
		}
		else if (!strcmp (type, TP_IFACE_CHANNEL_TYPE_TEXT) &&
			 handle_type == TP_HANDLE_TYPE_ROOM)
		{
			g_print ("Got MUC channel\n");
		}
	}
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
		tp_cli_connection_interface_requests_connect_to_new_channels (
				conn, new_channels_cb,
				NULL, NULL, NULL, &error);
		handle_error (error);

		char *targetid = argv[2];

		GHashTable *request = tp_asv_new (
			TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
			TP_IFACE_CHANNEL ".TargetHandleType", TP_TYPE_HANDLE, TP_HANDLE_TYPE_ROOM,
			TP_IFACE_CHANNEL ".TargetID", G_TYPE_STRING, targetid,
			NULL);

		tp_cli_connection_interface_requests_call_ensure_channel (
				conn, -1, request, NULL, NULL, NULL, NULL);
		g_hash_table_destroy (request);
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

	g_print (" > request_connection_cb (%s, %s)\n", bus_name, object_path);

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

	const TpConnectionManagerProtocol *prot = tp_connection_manager_get_protocol (cm, "jabber");
	if (!prot) g_error ("Protocol is not supported");

	char *username = argv[1];
	char *password = getpass ("Password: ");

	/* request a new connection */
	GHashTable *parameters = tp_asv_new (
			"account", G_TYPE_STRING, username,
			"password", G_TYPE_STRING, password,
			NULL);

	tp_cli_connection_manager_call_request_connection (cm, -1,
			"jabber",
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

	if (argc != 3)
	{
		g_error ("Must provide username and target!");
	}

	/* create a main loop */
	loop = g_main_loop_new (NULL, FALSE);

	/* acquire a connection to the D-Bus daemon */
	bus_daemon = tp_dbus_daemon_dup (&error);
	if (bus_daemon == NULL)
	{
		g_error ("%s", error->message);
	}

	/* begin ex.basics.language-bindings.telepathy-glib.ready */
	/* we want to request the gabble CM */
	TpConnectionManager *cm = tp_connection_manager_new (bus_daemon,
			"gabble", NULL, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_manager_call_when_ready (cm, cm_ready,
			argv, NULL, NULL);
	/* end ex.basics.language-bindings.telepathy-glib.ready */

	/* set up a signal handler */
	struct sigaction sa = { 0 };
	sa.sa_handler = interrupt_cb;
	sigaction (SIGINT, &sa, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
