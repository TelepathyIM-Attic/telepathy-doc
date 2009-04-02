#include <unistd.h>

#include <glib.h>

#include <telepathy-glib/connection-manager.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/contact.h>
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

static void
contacts_ready (TpConnection		*conn,
		guint			 n_contacts,
		TpContact * const	*contacts,
		guint			 n_failed,
		const TpHandle		*failed,
		const GError		*in_error,
		gpointer		 user_data,
		GObject			*weak_obj)
{
	TpChannel *channel = TP_CHANNEL (user_data);

	handle_error (in_error);

	g_print (" > contacts_ready for %s (%i contacts - %i failed)\n",
			tp_channel_get_identifier (channel),
			n_contacts, n_failed);

	int i;
	for (i = 0; i < n_contacts; i++)
	{
		TpContact *contact = contacts[i];

		g_print ("  - %s (%s)\t\t%s - %s\n",
				tp_contact_get_alias (contact),
				tp_contact_get_identifier (contact),
				tp_contact_get_presence_status (contact),
				tp_contact_get_presence_message (contact));
	}
}

static void
channel_ready (TpChannel	*channel,
	       const GError	*in_error,
	       gpointer		 user_data)
{
	g_print (" > channel_ready (%s)\n",
			tp_channel_get_identifier (channel));

	const TpIntSet *members = tp_channel_group_get_members (channel);
	GArray *handles = tp_intset_to_array (members);
	g_print ("   channel contains %i members\n", handles->len);

	/* we want to create a TpContact for each member of this channel */
	static const TpHandle features[] = { TP_CONTACT_FEATURE_ALIAS,
					     TP_CONTACT_FEATURE_PRESENCE };

	tp_connection_get_contacts_by_handle (conn,
			handles->len, (const TpHandle *) handles->data,
			G_N_ELEMENTS (features), features,
			contacts_ready,
			channel, NULL, NULL);

	g_array_free (handles, TRUE);
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

		/* begin ex.channel.requesting.glib.tpchannel */
		const char *type = tp_asv_get_string (map,
				TP_IFACE_CHANNEL ".ChannelType");

		/* if this channel is a contact list, we want to know
		 * about it */
		if (!strcmp (type, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST))
		{
			TpChannel *channel = tp_channel_new_from_properties (
					conn, object_path, map,
					&error);
			handle_error (error);

			tp_channel_call_when_ready (channel,
					channel_ready, NULL);
		}
		/* end ex.channel.requesting.glib.tpchannel */
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
presences_changed_cb (TpConnection	*conn,
                      GHashTable	*presence,
		      gpointer		 user_data,
		      GObject		*weak_obj)
{
	g_print (" :: presences_changed_cb\n");

	/* presence has a D-Bus type of a{u(uss)} which is represented by
	 * a GHashTable (uint -> GValueArray[uint, string, string]) */

	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, presence);
	while (g_hash_table_iter_next (&iter, &key, &value))
	{
		int contact = GPOINTER_TO_UINT (key);
		GValueArray *spresence = (GValueArray *) value;

		const char *status = g_value_get_string (
				g_value_array_get_nth (spresence, 1));
		const char *status_message = g_value_get_string (
				g_value_array_get_nth (spresence, 2));

		g_print ("Contact handle %i -> %s - %s\n",
				contact, status, status_message);
	}
}

static void
conn_ready (TpConnection	*conn,
            const GError	*in_error,
	    gpointer		 user_data)
{
	GError *error = NULL;

	g_print (" > conn_ready\n");

	handle_error (in_error);

	/* print out a list of available interfaces */
	char **interfaces, **ptr;
	g_object_get (conn, "interfaces", &interfaces, NULL);
	for (ptr = interfaces; ptr && *ptr; ptr++)
	{
		g_print ("- %s\n", *ptr);
	}
	g_strfreev (interfaces);

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

		tp_cli_connection_interface_requests_connect_to_new_channels (
				conn, new_channels_cb,
				NULL, NULL, NULL, &error);
		handle_error (error);

		/* begin ex.channel.requesting.glib.ensure */
		/* explicitly ask for the publish and subscribe contact lists
		 * these will be announced by NewChannels, so we don't need
		 * to handle their callbacks (this does mean we also can't
		 * handle their errors) */
		GHashTable *request = tp_asv_new (
			TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_CONTACT_LIST,
			TP_IFACE_CHANNEL ".TargetHandleType", TP_TYPE_HANDLE, TP_HANDLE_TYPE_LIST,
			NULL);

		/* the 'publish' list */
		tp_asv_set_string (request,
			TP_IFACE_CHANNEL ".TargetID", "publish");
		tp_cli_connection_interface_requests_call_ensure_channel (
				conn, -1, request, NULL, NULL, NULL, NULL);

		/* the 'subscribe' list */
		tp_asv_set_string (request,
			TP_IFACE_CHANNEL ".TargetID", "subscribe");
		tp_cli_connection_interface_requests_call_ensure_channel (
				conn, -1, request, NULL, NULL, NULL, NULL);

		g_hash_table_destroy (request);
		/* end ex.channel.requesting.glib.ensure */
	}

	/* check if the SimplePresence interface is available */
	if (tp_proxy_has_interface_by_id (conn,
		TP_IFACE_QUARK_CONNECTION_INTERFACE_SIMPLE_PRESENCE))
	{
		tp_cli_connection_interface_simple_presence_connect_to_presences_changed (
				conn, presences_changed_cb,
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

	g_print (" > request_connection_cb (%s, %s)\n", bus_name, object_path);

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

	/* begin ex.basics.language-bindings.telepathy-glib.ready */
	/* we want to request the gabble CM */
	TpConnectionManager *cm = tp_connection_manager_new (bus_daemon,
			"gabble", NULL, &error);
	if (error) g_error ("%s", error->message);

	tp_connection_manager_call_when_ready (cm, cm_ready,
			username, NULL, NULL);
	/* end ex.basics.language-bindings.telepathy-glib.ready */

	/* set up a signal handler */
	struct sigaction sa = { 0 };
	sa.sa_handler = interrupt_cb;
	sigaction (SIGINT, &sa, NULL);

	g_main_loop_run (loop);

	g_object_unref (bus_daemon);

	return 0;
}
