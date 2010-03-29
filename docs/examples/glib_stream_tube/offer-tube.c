#include <unistd.h>

#include <glib.h>
#include <gio/gio.h>

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

static GSocketAddress *server_sockaddr = NULL;

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
		       guint	  state,
		       gpointer	  user_data,
		       GObject	 *weak_obj)
{
	g_print ("Tube state changed %i\n", state);
}

static void
tube_offer_cb (TpChannel	*channel,
	       const GError	*in_error,
	       gpointer		 user_data,
	       GObject		*weak_obj)
{
	handle_error (in_error);

	g_print (" > tube_offer_cb\n");
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

	g_print ("Offering Tube...\n");
	GHashTable *parameters = tp_asv_new (
			"SomeKey", G_TYPE_STRING, "SomeValue",
			"IntKey", G_TYPE_INT, 42,
			NULL);

	GValue *value = tp_address_variant_from_g_socket_address (
			server_sockaddr, NULL, NULL);

	tp_cli_channel_type_stream_tube_call_offer (channel, -1,
			TP_SOCKET_ADDRESS_TYPE_IPV4, value,
			TP_SOCKET_ACCESS_CONTROL_LOCALHOST, parameters,
			tube_offer_cb, NULL, NULL, NULL);

	tp_g_value_slice_free (value);

	g_hash_table_destroy (parameters);
}

static void
create_channel_cb (TpConnection *conn,
		   const char   *address,
		   GHashTable   *properties,
		   const GError *in_error,
		   gpointer      user_data,
		   GObject      *weak_obj)
{
	handle_error (in_error);
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
		char *object_path;
		GHashTable *map;

		tp_value_array_unpack (channel, 2,
				&object_path,
				&map);

		const char *type = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".ChannelType");
		guint handle_type = tp_asv_get_uint32 (map,
				TP_IFACE_CHANNEL ".TargetHandleType", NULL);
		const char *targetid = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".TargetID");

		/* if this channel is a contact list, we want to know
		 * about it */
		if (!strcmp (type, TP_IFACE_CHANNEL_TYPE_STREAM_TUBE))
		{
			g_print ("New D-Bus Tube channel created\n");
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

			/* Dial up a D-Bus Tube */
			GHashTable *request = tp_asv_new (
				TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_STREAM_TUBE,
				TP_IFACE_CHANNEL ".TargetHandleType", TP_TYPE_HANDLE, handle_type,
				TP_IFACE_CHANNEL ".TargetID", G_TYPE_STRING, targetid,
				TP_IFACE_CHANNEL_TYPE_STREAM_TUBE ".Service", G_TYPE_STRING, "badger",
				NULL);

			tp_cli_connection_interface_requests_call_create_channel (
					conn, -1, request,
					create_channel_cb, NULL, NULL, NULL);

			g_hash_table_destroy (request);
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

#if 0
#endif
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

static gboolean
socket_incoming (GSocketService    *service,
		 GSocketConnection *connection,
		 GObject           *src_object,
		 gpointer           user_data)
{
	g_print (" > socket_incoming\n");

	return FALSE;
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

	/* begin ex.tubes.stream.setup.gnio */
	/* create the network service */
	GSocketService *socket_service = g_socket_service_new ();
	GInetAddress *inet_address = g_inet_address_new_loopback (
			G_SOCKET_FAMILY_IPV4);
	GSocketAddress *socket_address = g_inet_socket_address_new (
			inet_address, 0);
	g_object_unref (inet_address);

	g_socket_listener_add_address (G_SOCKET_LISTENER (socket_service),
			socket_address,
			G_SOCKET_TYPE_STREAM,
			G_SOCKET_PROTOCOL_DEFAULT,
			NULL,
			&server_sockaddr,
			&error);
	g_object_unref (socket_address);
	if (error) g_error ("%s", error->message);

	char *address_str = g_inet_address_to_string (
			g_inet_socket_address_get_address (
				G_INET_SOCKET_ADDRESS (server_sockaddr)));
	guint16 port = g_inet_socket_address_get_port (
			G_INET_SOCKET_ADDRESS (server_sockaddr));
	g_print ("address = %s\nport = %u\n",
			address_str, port);
	g_free (address_str);

	g_signal_connect (socket_service, "incoming",
			G_CALLBACK (socket_incoming), NULL);
	g_socket_service_start (socket_service);
	/* end ex.tubes.stream.setup.gnio */

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
