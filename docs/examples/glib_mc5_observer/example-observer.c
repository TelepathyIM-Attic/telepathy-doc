#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/gtypes.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-client.h>

#include "example-observer.h"

static void client_iface_init (gpointer, gpointer);
static void observer_iface_init (gpointer, gpointer);

// #define EXAMPLE_OBSERVER_GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), EXAMPLE_TYPE_OBSERVER, ExampleObserverPrivate))

G_DEFINE_TYPE_WITH_CODE (ExampleObserver, example_observer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_OBSERVER, observer_iface_init);
    );

static const char *client_interfaces[] = {
    TP_IFACE_CLIENT_OBSERVER,
    NULL
};

enum
{
  PROP_0,
  PROP_INTERFACES,
  PROP_CHANNEL_FILTER
};

// typedef struct _ExampleObserverPrivate ExampleObserverPrivate;
// struct _ExampleObserverPrivate
// {
// };

static void
example_observer_observe_channels (TpSvcClientObserver   *self,
                                   const char            *account,
                                   const char            *connection,
                                   const GPtrArray       *channels,
                                   const char            *dispatch_op,
                                   const GPtrArray       *requests_satisfied,
                                   GHashTable            *observer_info,
                                   DBusGMethodInvocation *context)
{
  g_print (" > example_observer_observe_channels\n");
  g_print ("     account    = %s\n", account);
  g_print ("     connection = %s\n", connection);
  g_print ("     dispatchop = %s\n", dispatch_op);

  /* channels is of type a(oa{sv}) */
  int i;
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *channel = g_ptr_array_index (channels, i);

      char *path = g_value_get_boxed (g_value_array_get_nth (channel, 0));
      GHashTable *map = g_value_get_boxed (g_value_array_get_nth (channel, 1));

      g_print ("     channel    = %s\n", path);
    }

  tp_svc_client_observer_return_from_observe_channels (context);
}

static void
example_observer_get_property (GObject    *self,
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

        /* create an empty filter - which means all channels */
        GPtrArray *array = g_ptr_array_new ();
        GHashTable *map = g_hash_table_new (NULL, NULL);

        g_ptr_array_add (array, map);
        g_value_set_boxed (value, array);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
example_observer_class_init (ExampleObserverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = example_observer_get_property;

  /* D-Bus properties are exposed as GObject properties through the
   * TpDBusPropertiesMixin */
  /* properties on the Client interface */
  static TpDBusPropertiesMixinPropImpl client_props[] = {
        { "Interfaces", "interfaces", NULL },
        { NULL }
  };

  static TpDBusPropertiesMixinPropImpl client_observer_props[] = {
        { "ObserverChannelFilter", "channel-filter", NULL },
        { NULL }
  };

  /* complete list of interfaces with properties */
  static TpDBusPropertiesMixinIfaceImpl prop_interfaces[] = {
        { TP_IFACE_CLIENT,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_props
        },
        { TP_IFACE_CLIENT_OBSERVER,
          tp_dbus_properties_mixin_getter_gobject_properties,
          NULL,
          client_observer_props
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
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  /* call our mixin class init */
  klass->dbus_props_class.interfaces = prop_interfaces;
  tp_dbus_properties_mixin_class_init (object_class,
      G_STRUCT_OFFSET (ExampleObserverClass, dbus_props_class));
}

static void
example_observer_init (ExampleObserver *self)
{
}

static void
observer_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcClientObserverClass *klass = (TpSvcClientObserverClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_observer_implement_##x (klass, \
    example_observer_##x)
  IMPLEMENT (observe_channels);
#undef IMPLEMENT
}

ExampleObserver *
example_observer_new (void)
{
  return g_object_new (TYPE_EXAMPLE_OBSERVER, NULL);
}
