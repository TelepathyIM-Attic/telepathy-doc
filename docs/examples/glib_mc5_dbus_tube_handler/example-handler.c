#include <telepathy-glib/dbus.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/enums.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/connection.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-client.h>
#include <telepathy-glib/util.h>

#include "example-handler.h"

#define SERVICE_NAME "org.freedesktop.Telepathy.Examples.TubeClient"

static void client_iface_init (gpointer, gpointer);
static void observer_iface_init (gpointer, gpointer);

#define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_EXAMPLE_HANDLER, ExampleHandlerPrivate))

G_DEFINE_TYPE_WITH_CODE (ExampleHandler, example_handler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_HANDLER, observer_iface_init);
    );

static const char *client_interfaces[] = {
    TP_IFACE_CLIENT_HANDLER,
    NULL
};

enum
{
  PROP_0,
  PROP_INTERFACES,
  PROP_CHANNEL_FILTER,
  PROP_BYPASS_APPROVAL,
  PROP_HANDLED_CHANNELS
};

typedef struct _ExampleHandlerPrivate ExampleHandlerPrivate;
struct _ExampleHandlerPrivate
{
  GList *channels;

  TpTubeState state;
  char *address;
};

static void
open_tube (ExampleHandler *self)
{
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  /* we can't be sure what order we will get our address and the open status
   * so check here that we have both */
  if (priv->state != TP_TUBE_STATE_OPEN || priv->address == NULL)
    {
      return;
    }

  g_print ("Ready to connect to D-Bus address = %s\n", priv->address);

  /* make a connection to the named D-Bus bus here.
   * Each member of the MUC is assigned a well-known address by the Connection
   * Manager, track the DBusNamesChanged signal to find out when users appear
   * or disappear on the bus. You can publish objects under this unique
   * address or make method calls to someone elses objects. */
}

static void
tube_state_changed_callback (TpChannel *channel,
                             guint      state,
                             gpointer   user_data,
                             GObject   *weak_obj)
{
  ExampleHandler *self = EXAMPLE_HANDLER (weak_obj);
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  priv->state = state;

  open_tube (self);
}

static void
dbus_names_changed_callback (TpChannel    *channel,
                             GHashTable   *added,
                             const GArray *removed,
                             gpointer      user_data,
                             GObject      *weak_obj)
{
  ExampleHandler *self = EXAMPLE_HANDLER (weak_obj);
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  /* the mapping between Handles and the unique D-Bus addresses of other users
   * has been updated because people have joined or left the MUC */

  g_print ("DBus names changed\n");
}

static void
tube_accept_callback (TpChannel    *channel,
                      const char   *address,
                      const GError *in_error,
                      gpointer      user_data,
                      GObject      *weak_obj)
{
  ExampleHandler *self = EXAMPLE_HANDLER (weak_obj);
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  if (in_error != NULL)
    {
      g_error ("%s", in_error->message);
    }

  priv->address = g_strdup (address);

  open_tube (self);
}

static void
tube_channel_ready (TpChannel    *channel,
                    const GError *in_error,
                    gpointer      user_data)
{
  ExampleHandler *self = EXAMPLE_HANDLER (user_data);
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  if (in_error != NULL)
    {
      g_error ("%s", in_error->message);
    }

  GError *error = NULL;

  g_print ("Channel ready\n");

  /* add the channel to the list */
  priv->channels = g_list_prepend (priv->channels, channel);

  tp_cli_channel_interface_tube_connect_to_tube_channel_state_changed (
      channel, tube_state_changed_callback,
      NULL, NULL, G_OBJECT (self), &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }
  tp_cli_channel_type_dbus_tube_connect_to_dbus_names_changed (
      channel, dbus_names_changed_callback,
      NULL, NULL, G_OBJECT (self), &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  tp_cli_channel_type_dbus_tube_call_accept (channel, -1,
      TP_SOCKET_ACCESS_CONTROL_LOCALHOST, tube_accept_callback,
      NULL, NULL, G_OBJECT (self));
}

static void
example_handler_handle_channels (TpSvcClientHandler   *self,
                                 const char            *account,
                                 const char            *connection,
                                 const GPtrArray       *channels,
                                 const GPtrArray       *requests_satisfied,
                                 guint64                user_action_time,
                                 GHashTable            *handler_info,
                                 DBusGMethodInvocation *context)
{
  GError *error = NULL;

  TpDBusDaemon *bus = tp_dbus_daemon_dup (&error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  TpConnection *conn = tp_connection_new (bus, NULL, connection, &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
    }

  /* channels is of type a(oa{sv}) */
  int i;
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *channel = g_ptr_array_index (channels, i);

      char *path;
      GHashTable *map;

      tp_value_array_unpack (channel, 2,
          &path,
          &map);

      const char *type = tp_asv_get_string (map,
          TP_PROP_CHANNEL_CHANNEL_TYPE);
      const char *service = tp_asv_get_string (map,
          TP_PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME);

      if (tp_strdiff (type, TP_IFACE_CHANNEL_TYPE_DBUS_TUBE) ||
          tp_strdiff (service, SERVICE_NAME))
        {
          continue;
        }

      g_print ("Got tube channel: %s\n", path);

      TpChannel *chan = tp_channel_new_from_properties (conn, path, map,
          &error);
      if (error != NULL)
        {
          g_error ("%s", error->message);
        }
      tp_channel_call_when_ready (chan, tube_channel_ready, self);
    }

  g_object_unref (conn);
  g_object_unref (bus);

  tp_svc_client_handler_return_from_handle_channels (context);
}

static void
example_handler_get_property (GObject    *self,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ExampleHandlerPrivate *priv = GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_INTERFACES:
        g_print (" :: interfaces\n");
        g_value_set_boxed (value, client_interfaces);
        break;

      case PROP_CHANNEL_FILTER:
        g_print (" :: channel-filter\n");

        /* this is the same map as the Python handler example */
        GPtrArray *array = g_ptr_array_new ();
        GHashTable *map = tp_asv_new (
              TP_PROP_CHANNEL_CHANNEL_TYPE, G_TYPE_STRING,
                  TP_IFACE_CHANNEL_TYPE_DBUS_TUBE,
              TP_PROP_CHANNEL_TARGET_HANDLE_TYPE, G_TYPE_UINT,
                  TP_HANDLE_TYPE_ROOM,
              TP_PROP_CHANNEL_REQUESTED, G_TYPE_BOOLEAN,
                  FALSE,
              TP_PROP_CHANNEL_TYPE_DBUS_TUBE_SERVICE_NAME, G_TYPE_STRING,
                  SERVICE_NAME,
              NULL
            );

        g_ptr_array_add (array, map);
        g_value_take_boxed (value, array);
        break;

      case PROP_BYPASS_APPROVAL:
        g_print (" :: bypass-approval\n");
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_HANDLED_CHANNELS:
        g_print (" :: handled-channels\n");

          {
            GPtrArray *array = g_ptr_array_new ();
            GList *ptr;

            for (ptr = priv->channels; ptr; ptr = ptr->next)
              {
                g_ptr_array_add (array,
                    g_strdup (tp_proxy_get_object_path (TP_PROXY (ptr->data))));
              }

            g_value_take_boxed (value, array);
          }

        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
example_handler_class_init (ExampleHandlerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = example_handler_get_property;

  /* D-Bus properties are exposed as GObject properties through the
   * TpDBusPropertiesMixin */
  /* properties on the Client interface */
  static TpDBusPropertiesMixinPropImpl client_props[] = {
        { "Interfaces", "interfaces", NULL },
        { NULL }
  };

  /* properties on the Client.Handler interface */
  static TpDBusPropertiesMixinPropImpl client_handler_props[] = {
        { "HandlerChannelFilter", "channel-filter", NULL },
        { "BypassApproval", "bypass-approval", NULL },
        { "HandledChannels", "handled-channels", NULL },
        { NULL }
  };

  /* complete list of interfaces with properties */
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
        { TP_IFACE_CLIENT,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_props
        },
        { TP_IFACE_CLIENT_HANDLER,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_handler_props
        },
        { NULL }
  };

  g_object_class_install_property (object_class, PROP_INTERFACES,
      g_param_spec_boxed ("interfaces",
                          "Interfaces",
                          "Available D-Bus Interfaces",
                          G_TYPE_STRV,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class, PROP_CHANNEL_FILTER,
      g_param_spec_boxed ("channel-filter",
                          "Channel Filter",
                          "Filter for channels we observe",
                          TP_ARRAY_TYPE_CHANNEL_CLASS_LIST,
                          G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_BYPASS_APPROVAL,
      g_param_spec_boolean ("bypass-approval",
                            "Bypass Approval",
                            "Whether or not this Client should bypass approval",
                            FALSE,
                            G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_HANDLED_CHANNELS,
      g_param_spec_boxed ("handled-channels",
                          "Handled Channels",
                          "List of channels we're handling",
                          TP_ARRAY_TYPE_OBJECT_PATH_LIST,
                          G_PARAM_READABLE));

  /* call our mixin class init */
  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleHandlerClass, dbus_props_class));

  g_type_class_add_private (klass, sizeof (ExampleHandlerPrivate));
}

static void
example_handler_init (ExampleHandler *self)
{
}

static void
observer_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcClientHandlerClass *klass = (TpSvcClientHandlerClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_handler_implement_##x (klass, \
    example_handler_##x)
  IMPLEMENT (handle_channels);
#undef IMPLEMENT
}

ExampleHandler *
example_handler_new (void)
{
  return g_object_new (TYPE_EXAMPLE_HANDLER, NULL);
}
