import dbus.glib
import gobject

import telepathy
from telepathy.interfaces import CLIENT, \
                                 CLIENT_OBSERVER, \
                                 CHANNEL

# begin ex.services.python.object
class ExampleObserver(telepathy.server.ClientObserver,
                      telepathy.server.DBusProperties):

    def __init__(self, *args):
        telepathy.server.ClientObserver.__init__(self, *args)
        telepathy.server.DBusProperties.__init__(self)

        self._implement_property_get(CLIENT, {
            'Interfaces': lambda: [ CLIENT_OBSERVER ],
          })
        self._implement_property_get(CLIENT_OBSERVER, {
            'ObserverChannelFilter': lambda: dbus.Array([
                    dbus.Dictionary({
                    }, signature='sv')
                ], signature='a{sv}')
          })

    def ObserveChannels(self, account, connection, channels, dispatch_operation,
                        requests_satisfied, observer_info):
        print "Incoming channels on %s:" % (connection)
        for object, props in channels:
            print " - %s :: %s" % (props[CHANNEL + '.ChannelType'],
                                   props[CHANNEL + '.TargetID'])
# end ex.services.python.object

# begin ex.services.python.publishing
def publish(client_name):
    bus_name = '.'.join ([CLIENT, client_name])
    object_path = '/' + bus_name.replace('.', '/')

    bus_name = dbus.service.BusName(bus_name, bus=dbus.SessionBus())

    ExampleObserver(bus_name, object_path)
# end ex.services.python.publishing
    return False

if __name__ == '__main__':
    gobject.timeout_add(0, publish, "ExampleObserver")
    loop = gobject.MainLoop()
    loop.run()
