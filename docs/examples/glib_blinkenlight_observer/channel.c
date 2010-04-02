/*
 * channel.c - wraps TpChannel to do preparation and unread message counting
 *             for a single channel we're observing
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

#include "channel.h"

#define GET_PRIVATE(obj)	(G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_CHANNEL, ChannelPrivate))

G_DEFINE_TYPE (Channel, channel, TP_TYPE_CHANNEL);

enum /* properties */
{
  PROP_0,
  PROP_UNREAD
};

typedef struct
{
  TpIntSet *pending;
} ChannelPrivate;

typedef struct
{
  TpChannelWhenReadyCb callback;
  gpointer user_data;
} ReadyCallbackData;

static void
_channel_update_pending_msgs (Channel *self)
{
  ChannelPrivate *priv = GET_PRIVATE (self);

  g_debug ("%s: pending messages %u",
      tp_proxy_get_object_path (self),
      tp_intset_size (priv->pending));

  g_object_notify (G_OBJECT (self), "unread");
}

static void
_channel_msg_received (TpChannel *self,
    guint id,
    guint timestamp,
    guint sender,
    guint type,
    guint flags,
    const char *text,
    gpointer user_data,
    GObject *weak_obj)
{
  ChannelPrivate *priv = GET_PRIVATE (self);

  /* The id is a number assigned to each message in the channel that is used
   * to acknowledge the message. Messages are acknowledged by the Handler when
   * it is sure that the user has seen them. When the client acknowledges a
   * message, we will receive the signal PendingMessageRemoved, and we can
   * remove this id from our pending list.
   *
   * For this to work it requires a well-behaved client that doesn't just
   * acknowledge messages immediately. For instance, recent Empathy */

  if (type == TP_CHANNEL_TEXT_MESSAGE_TYPE_NORMAL)
    {
      tp_intset_add (priv->pending, id);

      _channel_update_pending_msgs (CHANNEL (self));
    }
}

static void
_channel_pending_msg_removed (TpChannel *self,
    const GArray *ids,
    gpointer user_data,
    GObject *weak_obj)
{
  ChannelPrivate *priv = GET_PRIVATE (self);
  guint i;

  /* The client has acknowledged @ids, and we can remove them from the list
   * of messages we know the user hasn't seen */

  for (i = 0; i < ids->len; i++)
    {
      guint id = g_array_index (ids, guint, i);

      tp_intset_remove (priv->pending, id);
    }

  _channel_update_pending_msgs (CHANNEL (self));
}

static void
_channel_list_pending_messages (TpChannel *self,
    const GPtrArray *messages,
    const GError *error,
    gpointer user_data,
    GObject *weak_obj)
{
  ReadyCallbackData *data = user_data;

  if (error == NULL)
    {
      guint i;

      /* Iterate the pending messages */
      for (i = 0; i < messages->len; i++)
        {
          guint id, timestamp, sender, type, flags;
          const char *text;

          tp_value_array_unpack (g_ptr_array_index (messages, i), 6,
              &id, &timestamp, &sender, &type, &flags, &text);

          _channel_msg_received (self,
              id, timestamp, sender, type, flags, text,
              NULL, NULL);
        }
    }
  else
    {
      g_warning ("Failed to get pending messages: %s", error->message);
    }

  /* call the readiness callback */
  data->callback (self, error, data->user_data);
  g_slice_free (ReadyCallbackData, data);
}

static void
_channel_ready (TpChannel *self,
    const GError *error,
    gpointer user_data)
{
  ReadyCallbackData *data = user_data;
  GError *lerror = NULL;

  if (error != NULL)
    {
      data->callback (self, error, data->user_data);
      g_slice_free (ReadyCallbackData, data);
      return;
    }

  /* get the pending messages on the channel */
  tp_cli_channel_type_text_call_list_pending_messages (self, -1, FALSE,
      _channel_list_pending_messages,
      data, NULL, NULL);

  /* connect to the signals we need to listen to */
  tp_cli_channel_type_text_connect_to_received (self,
      _channel_msg_received,
      NULL, NULL, NULL, &lerror);
  g_assert_no_error (lerror);

  tp_cli_channel_interface_messages_connect_to_pending_messages_removed (self,
      _channel_pending_msg_removed,
      NULL, NULL, NULL, &lerror);
  g_assert_no_error (lerror);
}

void
channel_call_when_ready (Channel *self,
    TpChannelWhenReadyCb callback,
    gpointer user_data)
{
  ReadyCallbackData *data = g_slice_new0 (ReadyCallbackData);

  data->callback = callback;
  data->user_data = user_data;

  /* prepare the TpChannel */
  tp_channel_call_when_ready (TP_CHANNEL (self), _channel_ready, data);
}

static void
channel_get_property (GObject *self,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  ChannelPrivate *priv = GET_PRIVATE (self);

  switch (property_id)
    {
      case PROP_UNREAD:
        g_value_set_uint (value, tp_intset_size (priv->pending));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (self, property_id, pspec);
        break;
    }
}

static void
channel_dispose (GObject *self)
{
  ChannelPrivate *priv = GET_PRIVATE (self);

  if (priv->pending != NULL)
    {
      tp_intset_destroy (priv->pending);
      priv->pending = NULL;
    }

  G_OBJECT_CLASS (channel_parent_class)->dispose (self);
}

static void
channel_class_init (ChannelClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = channel_get_property;
  object_class->dispose = channel_dispose;

  g_object_class_install_property (object_class, PROP_UNREAD,
      g_param_spec_uint ("unread",
        "Unread",
        "Number of unread messages on this channel",
        0, G_MAXUINT, 0,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_type_class_add_private (object_class, sizeof (ChannelPrivate));
}

static void
channel_init (Channel *self)
{
  ChannelPrivate *priv = GET_PRIVATE (self);

  priv->pending = tp_intset_new ();
}

Channel *
channel_new (TpConnection *conn,
    const char *path,
    const GHashTable *properties,
    GError **error)
{
  /* this function is copied from
   * telepathy-glib/channel.c:tp_channel_new_from_properties */
  Channel *self = NULL;

  g_return_val_if_fail (TP_IS_CONNECTION (conn), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (properties != NULL, NULL);

  if (!tp_dbus_check_valid_object_path (path, error))
    goto finally;

  self = g_object_new (TYPE_CHANNEL,
      "connection", conn,
      "dbus-daemon", TP_PROXY (conn)->dbus_daemon,
      "bus-name", TP_PROXY (conn)->bus_name,
      "object-path", path,
      "handle-type", (guint) TP_UNKNOWN_HANDLE_TYPE,
      "channel-properties", properties,
      NULL);

finally:
  return self;
}
