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
