import dbus.glib
import gobject

import telepathy
from telepathy.interfaces import CLIENT, \
                                 CLIENT_OBSERVER, \
                                 CLIENT_HANDLER, \
                                 CHANNEL, \
                                 CHANNEL_TYPE_DBUS_TUBE

class ExampleObserver(telepathy.server.ClientObserver,
                      telepathy.server.ClientHandler,
                      telepathy.server.DBusProperties):

    def __init__(self, *args):
        dbus.service.Object.__init__(self, *args)
        telepathy.server.DBusProperties.__init__(self)

        self._implement_property_get(CLIENT, {
            'Interfaces': lambda: [ CLIENT_OBSERVER, CLIENT_HANDLER ],
          })
        self._implement_property_get(CLIENT_OBSERVER, {
            'ObserverChannelFilter': lambda: dbus.Array([
                    dbus.Dictionary({
                    }, signature='sv')
                ], signature='a{sv}'),
          })
        self._implement_property_get(CLIENT_HANDLER, {
            'HandlerChannelFilter': lambda: dbus.Array([
                    dbus.Dictionary({
              CHANNEL + '.ChannelType': CHANNEL_TYPE_DBUS_TUBE,
              CHANNEL_TYPE_DBUS_TUBE + '.ServiceName': 'org.freedesktop.Telepathy.Examples.TubeClient',
                    }, signature='sv')
                ], signature='a{sv}'),
            'BypassApproval': lambda: False,
            'Capabilities': lambda: dbus.Array([], signature='s'),
            'HandledChannels': self.get_handled_channels,
          })

    def Get(self, interface, property):
        v = telepathy.server.DBusProperties.Get(self, interface, property)
        print "Get", interface, property, v
        return v

    def GetAll(self, interface):
        v = telepathy.server.DBusProperties.GetAll(self, interface)
        print "GetAll", interface, v
        return v

    def get_handled_channels(self):
        return dbus.Array([], signature='o')

    def ObserveChannels(self, account, connection, channels, dispatch_operation,
                        requests_satisfied, observer_info):
        print "Incoming channels on %s:" % (connection)
        for object, props in channels:
            print " - %s :: %s" % (props[CHANNEL + '.ChannelType'],
                                   props[CHANNEL + '.TargetID'])

    def HandleChannels(self, account, connection, channels, requests_satisfied,
                       user_Action_time, handler_info):
        print "handle channels"

def publish(client_name):
    bus_name = '.'.join ([CLIENT, client_name])
    object_path = '/' + bus_name.replace('.', '/')

    bus_name = dbus.service.BusName(bus_name, bus=dbus.SessionBus())

    ExampleObserver(bus_name, object_path)
    return False

if __name__ == '__main__':
    gobject.timeout_add(0, publish, "ExampleHandler")
    loop = gobject.MainLoop()
    loop.run()
