#include <unistd.h>

#include <glib.h>

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

/* begin ex.basics.tpproperties.setup */
typedef struct _TpProperty TpProperty;
struct _TpProperty
{
	guint id;
	char *name;
	guint flags;
};

static GHashTable *
tp_property_get_map (TpProxy *proxy)
{
	g_return_val_if_fail (TP_IS_PROXY (proxy), NULL);

	GHashTable *map = g_object_get_data (G_OBJECT (proxy),
			"tpproperties-map");

	return map;
}

static guint
tp_property_get_id (TpProxy *proxy, const char *name)
{
	GHashTable *map = tp_property_get_map (proxy);

	g_return_val_if_fail (map != NULL, 0);

	return GPOINTER_TO_UINT (g_hash_table_lookup (map, name));
}

static GArray *
tp_property_get_array (TpProxy *proxy)
{
	g_return_val_if_fail (TP_IS_PROXY (proxy), NULL);

	GArray *array = g_object_get_data (G_OBJECT (proxy),
			"tpproperties-array");

	return array;
}

static TpProperty *
tp_property_from_id (TpProxy *proxy, guint id)
{
	GArray *array = tp_property_get_array (proxy);

	g_return_val_if_fail (array != NULL, NULL);

	return &g_array_index (array, TpProperty, id);
}

static void
tp_property_insert (TpProxy *proxy, TpProperty *property)
{
	GArray *array = tp_property_get_array (proxy);
	GHashTable *map = tp_property_get_map (proxy);

	g_array_append_val (array, *property);
	g_hash_table_insert (map, property->name, GUINT_TO_POINTER (property->id));
}

static void
tp_property_init (TpProxy *proxy)
{
	g_return_if_fail (TP_IS_PROXY (proxy));

	GArray *array = g_array_new (FALSE, FALSE,
			sizeof (TpProperty));
	GHashTable *map = g_hash_table_new (g_str_hash, g_str_equal);

	/* FIXME: use g_object_set_data_full() to cleanup on object finalize */
	g_object_set_data (G_OBJECT (proxy), "tpproperties-array", array);
	g_object_set_data (G_OBJECT (proxy), "tpproperties-map", map);
}
/* end ex.basics.tpproperties.setup */

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

/* begin ex.basics.tpproperties.changecb */
static void
tp_properties_changed_cb (TpProxy	  *channel,
			  const GPtrArray *properties,
			  gpointer	   user_data,
			  GObject	  *weak_obj)
{
	g_print (" > tp_properties_changed_cb\n");

	int i;
	for (i = 0; i < properties->len; i++)
	{
		GValueArray *property = g_ptr_array_index (properties, i);
		/* the id is a GValue<UINT>
		 * the variant is a GValue<GValue<??> */
		guint id;
		GValue *value;

		tp_value_array_unpack (property, 2,
				&id,
				&value);

		TpProperty *tpproperty = tp_property_from_id (TP_PROXY (channel), id);

		/* get a string representation of value */
		char *str = g_strdup_value_contents (value);
		g_print ("Property %s (%i): %s\n", tpproperty->name, id, str);
		g_free (str);
	}
}
/* end ex.basics.tpproperties.changecb */

/* begin ex.basics.tpproperties.flagchangecb */
static void
tp_property_flags_changed_cb (TpProxy		*channel,
			      const GPtrArray	*properties,
			      gpointer		 user_data,
			      GObject		*weak_obj)
{
	g_print (" > tp_property_flags_changed_cb\n");

	int i;
	for (i = 0; i < properties->len; i++)
	{
		GValueArray *property = g_ptr_array_index (properties, i);
		guint id, flags;

		tp_value_array_unpack (property, 2,
				&id,
				&flags);

		TpProperty *tpproperty = tp_property_from_id (TP_PROXY (channel), id);

		g_print ("Property %s (%i): %x\n",
			tpproperty->name, id, flags);
	}
}
/* end ex.basics.tpproperties.flagchangecb */

/* begin ex.basics.tpproperties.getcb */
static void
tp_properties_get_cb (TpProxy		*channel,
		      const GPtrArray	*properties,
		      const GError	*in_error,
		      gpointer		 user_data,
		      GObject		*weak_obj)
{
	handle_error (in_error);

	g_print (" > tp_properties_get_cb\n");

	int i;
	for (i = 0; i < properties->len; i++)
	{
		GValueArray *property = g_ptr_array_index (properties, i);
		/* the id is a GValue<UINT>
		 * the variant is a GValue<GValue<??> */
		guint id;
		GValue *value;

		tp_value_array_unpack (property, 2,
				&id,
				&value);

		TpProperty *tpproperty = tp_property_from_id (TP_PROXY (channel), id);

		/* get a string representation of value */
		char *str = g_strdup_value_contents (value);
		g_print ("Property %s (%i): %s\n", tpproperty->name, id, str);
		g_free (str);
	}
}
/* end ex.basics.tpproperties.getcb */

/* begin ex.basics.tpproperties.list */
static void
list_properties_cb (TpProxy		*channel,
		    const GPtrArray	*available_properties,
		    const GError	*in_error,
		    gpointer		 user_data,
		    GObject		*weak_obj)
{
	handle_error (in_error);

	g_print (" > list_properties_cb\n");

	/* @available_properties is a GPtrArray of GValueArray structs
	 * of signature (ussu) */
	int i;
	for (i = 0; i < available_properties->len; i++)
	{
		GValueArray *prop = g_ptr_array_index (available_properties, i);

		guint id, flags;
		const char *name, *sig;

		tp_value_array_unpack (prop, 4,
				&id,
				&name,
				&sig,
				&flags);

		g_print ("%u %s (%s) %x\n", id, name, sig, flags);

		TpProperty property = { 0, };
		property.id = id;
		property.name = g_strdup (name);
		property.flags = flags;
		tp_property_insert (TP_PROXY (channel), &property);
	}

	/* call the chained callback if set */
	if (user_data)
	{
		void (* func) (TpProxy *);

		func = user_data;

		func (channel);
	}
}
/* end ex.basics.tpproperties.list */

static void
tpproperties_ready (TpChannel	*channel)
{
	{ /* pack the readable properties into a GArray */
		/* begin ex.basics.tpproperties.get */
		GArray *array = g_array_new (FALSE, FALSE, sizeof (guint));
		int i;

		g_print ("Read property: ");
		for (i = 0; i < tp_property_get_array (TP_PROXY (channel))->len; i++)
		{
			TpProperty *property = tp_property_from_id (TP_PROXY (channel), i);

			if (!(property->flags & TP_PROPERTY_FLAG_READ)) continue;

			g_print ("%i ", i);
			g_array_append_val (array, i);
		}
		g_print ("\n");

		tp_cli_properties_interface_call_get_properties (channel, -1,
				array, tp_properties_get_cb,
				NULL, NULL, NULL);

		g_array_free (array, TRUE);
		/* end ex.basics.tpproperties.get */
	}

	{ /* set some properties */
		/* begin ex.basics.tpproperties.set */
		GPtrArray *array = g_ptr_array_new_with_free_func (
				(GDestroyNotify) g_value_array_free);
		GValue value = { 0, };

		/* FIXME we're assuming this property exists, we should check */
		guint id = tp_property_get_id (TP_PROXY (channel), "subject");

		g_value_init (&value, G_TYPE_STRING);
		g_value_set_static_string (&value, "Test Subject");

		g_ptr_array_add (array, tp_value_array_build (2,
				G_TYPE_UINT, id,
				G_TYPE_VALUE, &value,
				G_TYPE_INVALID));

		g_value_unset (&value);

		g_print ("Setting properties...\n");
		tp_cli_properties_interface_call_set_properties (channel, -1,
				array, NULL, NULL, NULL, NULL);

		g_ptr_array_free (array, TRUE);
		/* end ex.basics.tpproperties.set */
	}
}

static void
muc_channel_ready (TpChannel	*channel,
		   const GError	*in_error,
		   gpointer	 user_data)
{
	g_print ("MUC channel (%s) ready\n",
			tp_channel_get_identifier (channel));

	/* exciting things about MUC channels are stored as Telepathy
	 * Properties (not D-Bus properties). This interface is a little
	 * awkward.
	 * First we need to get a list of available properties */
	tp_property_init (TP_PROXY (channel));
	tp_cli_properties_interface_call_list_properties (channel, -1,
			list_properties_cb, tpproperties_ready, NULL, NULL);

	GError *error = NULL;

	tp_cli_properties_interface_connect_to_properties_changed (
			channel, tp_properties_changed_cb,
			NULL, NULL, NULL, &error);
	handle_error (error);
	tp_cli_properties_interface_connect_to_property_flags_changed (
			channel, tp_property_flags_changed_cb,
			NULL, NULL, NULL, &error);
	handle_error (error);
}

static void
create_muc_cb (TpConnection	*conn,
	       gboolean		 yours,
	       const char	*object_path,
	       GHashTable	*props,
	       const GError	*in_error,
	       gpointer		 user_data,
	       GObject		*weak_obj)
{
	handle_error (in_error);
	GError *error = NULL;

	g_print (" > create_muc_cb (%s)\n", object_path);

	TpChannel *channel = tp_channel_new_from_properties (conn,
			object_path, props, &error);
	handle_error (error);

	tp_channel_call_when_ready (channel, muc_channel_ready, NULL);
}

static void
muc_request_channel_cb (TpConnection	*conn,
			const char	*object_path,
			const GError	*in_error,
			gpointer	 user_data,
			GObject		*weak_obj)
{
	handle_error (in_error);
	GError *error = NULL;

	g_print (" > muc_request_channel_cb (%s)\n", object_path);

	TpHandle handle = GPOINTER_TO_INT (user_data);
	TpChannel *channel = tp_channel_new (conn,
			object_path,
			TP_IFACE_CHANNEL_TYPE_TEXT,
			TP_HANDLE_TYPE_ROOM,
			handle,
			&error);
	handle_error (error);

	tp_channel_call_when_ready (channel, muc_channel_ready, NULL);
}

static void
request_handles_cb (TpConnection	*conn,
		    TpHandleType	 handle_type,
		    guint		 n_handles,
		    const TpHandle	*handles,
		    const char * const	*ids,
		    const GError	*in_error,
		    gpointer		 user_data,
		    GObject		*weak_object)
{
	g_print (" > request_handles_cb\n");

	handle_error (in_error);
	g_return_if_fail (n_handles == 1);

	/* since there is no error, and only 1 handle, let us assume that it
	 * is the handle for the room #test */

	tp_cli_connection_call_request_channel (conn, -1,
			TP_IFACE_CHANNEL_TYPE_TEXT,
			TP_HANDLE_TYPE_ROOM, handles[0],
			TRUE,
			muc_request_channel_cb,
			GINT_TO_POINTER (handles[0]), NULL, NULL);
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
		/* make a connection to a MUC channel */
		GHashTable *map = tp_asv_new (
			TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING, TP_IFACE_CHANNEL_TYPE_TEXT,
			TP_IFACE_CHANNEL ".TargetHandleType", G_TYPE_UINT, TP_HANDLE_TYPE_ROOM,
			TP_IFACE_CHANNEL ".TargetID", G_TYPE_STRING, "#test",
			NULL);

		tp_cli_connection_interface_requests_call_ensure_channel (
				conn, -1, map,
				create_muc_cb,
				NULL, NULL, NULL);

		g_hash_table_destroy (map);
	}
	else
	{
		g_print ("Requests interface is not available\n");

		/* we need a handle for the channel we are going to join */
		const char *handles[] = { "#test", NULL };
		tp_connection_request_handles (conn, -1, TP_HANDLE_TYPE_ROOM,
				handles, request_handles_cb,
				NULL, NULL, NULL);
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

	/* request a new connection */
	GHashTable *parameters = tp_asv_new (
			"account", G_TYPE_STRING, argv[1],
			"server", G_TYPE_STRING, argv[2],
			NULL);

	tp_cli_connection_manager_call_request_connection (cm, -1,
			"irc",
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
		g_error ("Must provide username and server!");
	}

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
			"idle", NULL, &error);
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
