/*
 * observer.c - a Telepathy Observer, observes text channels to count their
 *              unread messages
 *
 * Copyright (C) 2010 Collabora Ltd. <http://www.collabora.co.uk/>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Authors:
 *   Danielle Madeley <danielle.madeley@collabora.co.uk>
 */

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/svc-generic.h>
#include <telepathy-glib/svc-client.h>

#include "observer.h"
#include "channel.h"

static void client_iface_init (gpointer, gpointer);
static void observer_iface_init (gpointer, gpointer);

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_OBSERVER, ObserverPrivate))

G_DEFINE_TYPE_WITH_CODE (Observer, observer, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_DBUS_PROPERTIES,
      tp_dbus_properties_mixin_iface_init);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_OBSERVER, observer_iface_init);
    );

static const char *client_interfaces[] = {
    TP_IFACE_CLIENT_OBSERVER,
    NULL
};

enum /* properties */
{
  PROP_0,
  PROP_INTERFACES,
  PROP_CHANNEL_FILTER
};

typedef struct
{
  GList *channels;
} ObserverPrivate;

typedef struct
{
  Observer *self;
  GList *pending;
  DBusGMethodInvocation *context;
} ReadyCallbackData;

static void
_update_count (Observer *self)
{
  ObserverPrivate *priv = GET_PRIVATE (self);
  GList *ptr;
  guint total = 0;

  for (ptr = priv->channels; ptr != NULL; ptr = ptr->next)
    {
      guint unread;

      g_object_get (ptr->data,
          "unread", &unread,
          NULL);

      total += unread;
    }

  /* this is where code to interface with the blinking light/whatever goes */
  g_print ("TOTAL UNREAD MESSAGES: %u\n", total);
}

static void
_channel_update_count (GObject *channel,
    GParamSpec *pspec,
    Observer *self)
{
  _update_count (self);
}

static void
_channel_closed (TpProxy *channel,
    guint domain,
    guint code,
    char *message,
    Observer *self)
{
  ObserverPrivate *priv = GET_PRIVATE (self);

  g_debug ("Channel closed: %s", tp_proxy_get_object_path (channel));

  /* remove this channel from the list of active channels */
  priv->channels = g_list_remove (priv->channels, channel);
  _update_count (self);

  g_object_unref (channel);
}

static void
_channel_ready (TpChannel *channel,
    const GError *error,
    gpointer user_data)
{
  ReadyCallbackData *data = user_data;
  Observer *self = data->self;
  ObserverPrivate *priv = GET_PRIVATE (self);

  /* remove the channel from the pending list */
  data->pending = g_list_remove (data->pending, channel);

  if (error == NULL)
    {
      g_debug ("Channel ready: %s", tp_proxy_get_object_path (channel));

      /* put this channel in the list of active channels */
      priv->channels = g_list_prepend (priv->channels, channel);

      tp_g_signal_connect_object (channel, "notify::unread",
          G_CALLBACK (_channel_update_count), self, 0);
      tp_g_signal_connect_object (channel, "invalidated",
          G_CALLBACK (_channel_closed), self, 0);

      _update_count (self);
    }
  else
    {
      g_warning ("Channel ready failed: %s",
          tp_proxy_get_object_path (channel));

      /* drop the ref, this channel is dead */
      g_object_unref (channel);
    }

  if (data->pending == NULL)
    {
      g_debug ("All channels ready");

      /* if all channels for this dispatch are ready, we can return from
       * ObserveChannels */
      tp_svc_client_observer_return_from_observe_channels (data->context);

      g_slice_free (ReadyCallbackData, data);
    }
}

static void
observer_observe_channels (TpSvcClientObserver *self,
    const char *account_path,
    const char *connection_path,
    const GPtrArray *channels,
    const char *dispatch_op_path,
    const GPtrArray *requests_satisfied,
    GHashTable *observer_info,
    DBusGMethodInvocation *context)
{
  TpDBusDaemon *dbus = NULL;
  TpConnection *conn = NULL;
  ReadyCallbackData *data = NULL;
  GError *error = NULL;
  guint i;

  dbus = tp_dbus_daemon_dup (&error);
  if (error != NULL)
    goto error;

  conn = tp_connection_new (dbus, NULL, connection_path, &error);
  if (error != NULL)
    goto error;

  /* returning from the D-Bus method ObserveChannels passes the channels
   * onwards towards the Handler. We want to have the chance to read the
   * initial pending message queue before it gets acknowledged, so we defer
   * returning from ObserveChannels until all the channels are ready */
  data = g_slice_new0 (ReadyCallbackData);
  data->self = OBSERVER (self);
  data->context = context;

  /* build a list of channels and queue them for preparation */
  for (i = 0; i < channels->len; i++)
    {
      GValueArray *channel = g_ptr_array_index (channels, i);
      Channel *chan;
      char *path;
      GHashTable *map;

      tp_value_array_unpack (channel, 2,
          &path, &map);

      chan = channel_new (conn, path, map, &error);
      if (error != NULL)
        continue;

      /* place this channel in the list of channels pending preparation
       * for this dispatch */
      data->pending = g_list_prepend (data->pending, chan);
      channel_call_when_ready (chan, _channel_ready, data);
    }

  goto finally;

error:
  dbus_g_method_return_error (context, error);

  g_error_free (error);

finally:
  if (dbus != NULL)
    g_object_unref (dbus);

  if (conn != NULL)
    g_object_unref (conn);
}

static void
observer_get_property (GObject *self,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  switch (property_id)
    {
      case PROP_INTERFACES:
        g_value_set_boxed (value, client_interfaces);
        break;

      case PROP_CHANNEL_FILTER:
        {
          GPtrArray *array = g_ptr_array_new ();

          /* Observe all text channels */
          g_ptr_array_add (array, tp_asv_new (
                TP_IFACE_CHANNEL ".ChannelType",
                G_TYPE_STRING,
                TP_IFACE_CHANNEL_TYPE_TEXT,

                NULL));

          g_value_take_boxed (value, array);
          break;
        }

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
observer_class_init (ObserverClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = observer_get_property;

  /* D-Bus properties are exposed as GObject properties through the
   * TpDBusPropertiesMixin */
  /* properties on the Client interface */
  static TpDBusPropertiesMixinPropImpl client_props[] = {
        { "Interfaces", "interfaces", NULL },
        { NULL }
  };

  /* properties on the Client.Observer interface */
  static TpDBusPropertiesMixinPropImpl client_observer_props[] = {
        { "ObserverChannelFilter", "channel-filter", NULL },
        /* NB: eventually we'll want to support the Recover property:
         *     https://bugs.freedesktop.org/show_bug.cgi?id=24768 */
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
      G_STRUCT_OFFSET (ObserverClass, dbus_props_class));

  g_type_class_add_private (object_class, sizeof (ObserverPrivate));
}

static void
observer_init (Observer *self)
{
}

static void
observer_iface_init (gpointer g_iface, gpointer iface_data)
{
  TpSvcClientObserverClass *klass = (TpSvcClientObserverClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_observer_implement_##x (klass, \
    observer_##x)
  IMPLEMENT (observe_channels);
#undef IMPLEMENT
}

Observer *
observer_new (void)
{
  return g_object_new (TYPE_OBSERVER, NULL);
}
