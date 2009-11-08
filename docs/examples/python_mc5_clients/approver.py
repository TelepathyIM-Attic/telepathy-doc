import dbus.glib
import gobject
import telepathy

from telepathy._generated.Client_Approver import ClientApprover
from telepathy.server.properties import DBusProperties
from telepathy.interfaces import CLIENT, \
                                 CLIENT_APPROVER, \
                                 CHANNEL

DBUS_PROPERTIES = "org.freedesktop.DBus.Properties"

class ExampleApprover(ClientApprover, DBusProperties):
    properties = {
        CLIENT: {
            'Interfaces': [ CLIENT_APPROVER ],
        },
        CLIENT_APPROVER: {
            'ApproverChannelFilter': dbus.Array([
                    dbus.Dictionary({
                    }, signature='sv')
                ], signature='a{sv}')
        },
    }

    def __init__(self, client_name):
        bus_name = '.'.join ([CLIENT, client_name])
        object_path = '/' + bus_name.replace('.', '/')

        bus_name = dbus.service.BusName(bus_name, bus=dbus.SessionBus())
        ClientApprover.__init__(self, bus_name, object_path)

    def GetAll(self, interface):
        print "GetAll", interface
        if interface in self.properties:
            return self.properties[interface]
        else:
            return {}

    def Get(self, interface, property):
        print "Get", interface, property
        if interface in self.properties and \
           property in self.properties[interface]:
            return self.properties[interface][property]
        else:
            return 0

    def AddDispatchOperation(self, channels, dispatch, properties):
        print "Incoming channels:"
        for object, props in channels:
            print " - %s :: %s" % (props[CHANNEL + '.ChannelType'],
                                   props[CHANNEL + '.TargetID'])
	# if we return from our approver without raising an error, it's assumed
	# that we accept the request and we're waiting for user feedback.
	# if this is the only approver, and we then do nothing with the request
	# it will get stuck and never be handled.
	raise Exception("not interested")

def start():
    ExampleApprover("ExampleApprover")
    return False

if __name__ == '__main__':
    gobject.timeout_add(0, start)
    loop = gobject.MainLoop()
    loop.run()
