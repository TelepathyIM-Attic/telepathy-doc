#ifndef __CONNECTIONS_MONITOR_H__
#define __CONNECTIONS_MONITOR_H__

#include <telepathy-glib/telepathy-glib.h>

G_BEGIN_DECLS

#define TYPE_CONNECTIONS_MONITOR	(connections_monitor_get_type ())
#define CONNECTIONS_MONITOR(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_CONNECTIONS_MONITOR, ConnectionsMonitor))
#define CONNECTIONS_MONITOR_CLASS(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), TYPE_CONNECTIONS_MONITOR, ConnectionsMonitorClass))
#define IS_CONNECTIONS_MONITOR(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_CONNECTIONS_MONITOR))
#define IS_CONNECTIONS_MONITOR_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_CONNECTIONS_MONITOR))
#define CONNECTIONS_MONITOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_CONNECTIONS_MONITOR, ConnectionsMonitorClass))

typedef struct _ConnectionsMonitor ConnectionsMonitor;
typedef struct _ConnectionsMonitorClass ConnectionsMonitorClass;

struct _ConnectionsMonitor
{
  GObject parent;
};

struct _ConnectionsMonitorClass
{
  GObjectClass parent_class;

  void (* connection) (ConnectionsMonitor *self,
      TpAccount *account,
      TpConnection *connection);
};

GType connections_monitor_get_type (void);
ConnectionsMonitor *connections_monitor_new (void);

G_END_DECLS

#endif
