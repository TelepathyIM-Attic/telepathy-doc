#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/dbus.h>

#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-client.h>

#include "example-handler.h"

static void client_iface_init (gpointer, gpointer);
static void handler_iface_init (gpointer, gpointer);

// #define EXAMPLE_HANDLER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), EXAMPLE_TYPE_HANDLER, ExampleHandlerPrivate))

G_DEFINE_TYPE_WITH_CODE (ExampleHandler, example_handler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_HANDLER, handler_iface_init);
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

// typedef struct _ExampleHandlerPrivate ExampleHandlerPrivate;
// struct _ExampleHandlerPrivate
// {
// };

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
  g_print (" > example_handler_handle_channels\n");
  g_print ("     account    = %s\n", account);
  g_print ("     connection = %s\n", connection);

  /* channels is of type a(oa{sv}) */
  int i;
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *channel = g_ptr_array_index (channels, i);

      char *path = g_value_get_boxed (g_value_array_get_nth (channel, 0));
      GHashTable *map = g_value_get_boxed (g_value_array_get_nth (channel, 1));

      g_print ("     channel    = %s\n", path);
    }

  tp_svc_client_handler_return_from_handle_channels (context);
}

static void
example_handler_get_property (GObject    *self,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  switch (property_id)
    {
      case PROP_INTERFACES:
        g_print (" :: interfaces\n");
        g_value_set_boxed (value, client_interfaces);
        break;

      case PROP_CHANNEL_FILTER:
        g_print (" :: channel-filter\n");

        /* we're only interested in FT channels */
          {
            GPtrArray *array = g_ptr_array_new ();
            GHashTable *map = tp_asv_new (
                TP_IFACE_CHANNEL ".ChannelType", G_TYPE_STRING,
                    TP_IFACE_CHANNEL_TYPE_FILE_TRANSFER,
                NULL);

            g_ptr_array_add (array, map);
            g_value_take_boxed (value, array);
          }
        break;

      case PROP_BYPASS_APPROVAL:
        g_print (" :: bypass-approval\n");
        g_value_set_boolean (value, FALSE);
        break;

      case PROP_HANDLED_CHANNELS:
        g_print (" :: handled-channels\n");

          {
            GPtrArray *array = g_ptr_array_new ();
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
                          "Filter for channels we handle",
                          TP_ARRAY_TYPE_CHANNEL_CLASS_LIST,
                          G_PARAM_READABLE));

  g_object_class_install_property (object_class, PROP_BYPASS_APPROVAL,
      g_param_spec_boolean ("bypass-approval",
                            "Bypass Approval",
                            "Whether or not to bypass the Approver",
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
}

static void
example_handler_init (ExampleHandler *self)
{
}

static void
handler_iface_init (gpointer g_iface, gpointer iface_data)
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
