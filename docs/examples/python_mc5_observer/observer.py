import dbus.glib
import gobject
import telepathy

from telepathy.constants import CONNECTION_HANDLE_TYPE_CONTACT
from telepathy.interfaces import CLIENT, \
                                 CLIENT_OBSERVER, \
                                 CHANNEL

DBUS_PROPERTIES = "org.freedesktop.DBus.Properties"

class ExampleObserver(dbus.service.Object):
    properties = {
        CLIENT: {
            'Interfaces': [ CLIENT_OBSERVER ],
        },
        CLIENT_OBSERVER: {
            'ObserverChannelFilter': dbus.Array([
                    dbus.Dictionary({
                    }, signature='sv')
                ], signature='a{sv}')
        },
    }

    def __init__(self, client_name):
        bus_name = '.'.join ([CLIENT, client_name])
        object_path = '/' + bus_name.replace('.', '/')

        bus_name = dbus.service.BusName(bus_name, bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, object_path)
        self.nameref = bus_name

    @dbus.service.method(dbus_interface=DBUS_PROPERTIES,
                         in_signature='s',
                         out_signature='a{sv}')
    def GetAll(self, interface):
        print "GetAll", interface
        if interface in self.properties:
            return self.properties[interface]
        else:
            return {}

    @dbus.service.method(dbus_interface=DBUS_PROPERTIES,
                         in_signature='ss',
                         out_signature='v')
    def Get(self, interface, property):
        print "Get", interface, property
        if interface in self.properties and \
           property in self.properties[interface]:
            return self.properties[interface][property]
        else:
            return 0

    @dbus.service.method(dbus_interface=CLIENT_OBSERVER,
                         in_signature='ooa(oa{sv})oaoa{sv}',
                         out_signature='')
    def ObserveChannels(self, account, connection, channels, dispatch_operation,
                        requests_satisfied, observer_info):

        print "Incoming channels on %s:" % (connection)
        for object, props in channels:
            print " - %s :: %s" % (props[CHANNEL + '.ChannelType'],
                                   props[CHANNEL + '.TargetID'])

def start():
    ExampleObserver("ExampleObserver")
    return False

if __name__ == '__main__':
    gobject.timeout_add(0, start)
    loop = gobject.MainLoop()
    loop.run()
