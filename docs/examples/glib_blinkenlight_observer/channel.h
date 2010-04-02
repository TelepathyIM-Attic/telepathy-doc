/*
 * channel.h - wraps TpChannel to do preparation and unread message counting
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

#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <glib-object.h>
#include <telepathy-glib/channel.h>

G_BEGIN_DECLS

#define TYPE_CHANNEL	(channel_get_type ())
#define CHANNEL(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CHANNEL, Channel))
#define CHANNEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_CHANNEL, ChannelClass))
#define IS_CHANNEL(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CHANNEL))
#define IS_CHANNEL_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_CHANNEL))
#define CHANNEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CHANNEL, ChannelClass))

typedef struct _Channel Channel;
typedef struct _ChannelClass ChannelClass;

struct _Channel
{
  TpChannel parent;
};

struct _ChannelClass
{
  TpChannelClass parent_class;
};

GType channel_get_type (void);
Channel *channel_new (TpConnection *conn,
    const char *path,
    const GHashTable *properties,
    GError **error);

void channel_call_when_ready (Channel *channel,
    TpChannelWhenReadyCb callback,
    gpointer user_data);

G_END_DECLS

#endif
