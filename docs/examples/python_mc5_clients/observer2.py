#
# This example Telepathy Observe tracks the lifetimes of text conversations.
#
# For each incoming channel it creates an EChannel object, and records the
# timestamp of the first and last message received by this channel.
#
# EConnections and EChannels are tracked by the EObserver, their invalidation
# is signalled by GObject signals. EObserver.ObserveChannels uses asynchronous
# return functions to only return from the D-Bus method once all of the channels
# are prepared, this gives the Observer time to investigate the pending message
# queue before any Handlers can acknowledge it.
#
# Author: Danielle Madeley <danielle.madeley@collabora.co.uk>
#
import dbus, dbus.glib
import gobject

import telepathy
from telepathy.interfaces import *
from telepathy.constants import *

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

def error(e):
    print 'Error:', e

class EConnection(gobject.GObject,
                  telepathy.client.Connection):
    __gsignals__ = {
        'disconnected': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ()),
    }

    def __init__(self, path, ready_handler):
        service_name = path.replace('/', '.')[1:]

        self.ready_handler = ready_handler
        self.signals = []

        gobject.GObject.__init__(self)
        telepathy.client.Connection.__init__(self, service_name, path,
            ready_handler=self._connection_ready)

    def __repr__(self):
        return 'EConnection(%s)' % self.object_path

    def _status_changed(self, status, reason):
        if status == CONNECTION_STATUS_DISCONNECTED:
            for signal in self.signals:
                signal.remove()

            self.emit('disconnected')

    def _connection_ready(self, conn):
        def reply(interfaces):
            self.contact_attribute_interfaces = interfaces

            self.ready_handler(self)

        # get the value of ContactAttributeInterfaces
        self[DBUS_PROPERTIES].Get(CONNECTION_INTERFACE_CONTACTS,
            'ContactAttributeInterfaces',
            reply_handler=reply, error_handler=error)

        self.signals.append(self[CONNECTION].connect_to_signal(
            'StatusChanged', self._status_changed))

    def do_disconnect(self):
        # required so that we don't transmit this over D-Bus
        pass

gobject.type_register(EConnection)

class EChannel(gobject.GObject,
               telepathy.client.Channel):
    __gsignals__ = {
        'closed': (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, ()),
    }

    def __init__(self, account_path, conn, path, properties, ready_handler):
        self.ready_handler = ready_handler
        self.account_path = account_path
        self.conn = conn
        self.properties = properties
        self.signals = []

        gobject.GObject.__init__(self)
        telepathy.client.Channel.__init__(self, conn.service_name, path,
            ready_handler=self._channel_ready)

        self._first_timestamp = None
        self._last_timestamp = None

    def __repr__(self):
        return 'EChannel(%s)' % self.object_path

    def _handle_message(self, timestamp):
        if self._first_timestamp is None:
            self._first_timestamp = timestamp

        self._last_timestamp = timestamp

        print '%s: %u -> %u' % (
            self._target_alias, self._first_timestamp, self._last_timestamp)

    def _received_message(self, id, timestamp, sender, type, flags, text):
        self._handle_message(timestamp)

    def _sent_message(self, timestamp, type, text):
        self._handle_message(timestamp)

    def _channel_closed(self):
        for signal in self.signals:
            signal.remove()

        self.emit('closed')

    def _channel_ready(self, chan):
        handle = self.properties[CHANNEL + '.TargetHandle']

        def pending_messages_reply(messages):
            # handle these messages
            for message in messages:
                self._received_message(*message)

            # we're ready
            self.ready_handler(self)

        def get_target_handle_alias(attributes_map):
            if handle not in attributes_map:
                error('Handle %u not known, weird' % handle)
                self._target_alias = self.properties[CHANNEL + '.TargetID']
            else:
                attributes = attributes_map[handle]

                self._target_alias = \
                    attributes.get(CONNECTION_INTERFACE_ALIASING + '/alias',
                        self.properties[CHANNEL + '.TargetID'])

            # get the pending messages
            self[CHANNEL_TYPE_TEXT].ListPendingMessages(False,
                reply_handler=pending_messages_reply, error_handler=error)

        # look up the TargetHandle
        self.conn[CONNECTION_INTERFACE_CONTACTS].GetContactAttributes(
            [ handle ],
            self.conn.contact_attribute_interfaces,
            False,
            reply_handler=get_target_handle_alias, error_handler=error)

        # connect the signals we care about
        self.signals.append(self[CHANNEL].connect_to_signal(
            'Closed', self._channel_closed))
        self.signals.append(self[CHANNEL_TYPE_TEXT].connect_to_signal(
            'Received', self._received_message))
        self.signals.append(self[CHANNEL_TYPE_TEXT].connect_to_signal(
            'Sent', self._sent_message))

    def do_closed(self):
        # required so that we don't transmit this over D-Bus
        pass

gobject.type_register(EChannel)

class EObserver(telepathy.server.Observer,
                telepathy.server.DBusProperties):

    def __init__(self, *args):
        telepathy.server.Observer.__init__(self, *args)
        telepathy.server.DBusProperties.__init__(self)

        self._implement_property_get(CLIENT, {
            'Interfaces': lambda: [ CLIENT_OBSERVER ],
          })
        self._implement_property_get(CLIENT_OBSERVER, {
            'ObserverChannelFilter': lambda: dbus.Array([
                    dbus.Dictionary({
                        CHANNEL + '.ChannelType': CHANNEL_TYPE_TEXT,
                    }, signature='sv')
                ], signature='a{sv}')
          })

        self.connections = {}
        self.channels = {}

    @dbus.service.method(CLIENT_OBSERVER,
        in_signature='ooa(oa{sv})oaoa{sv}', out_signature='',
        async_callbacks=('_success', '_error'))
    def ObserveChannels(self, account, connection, channels, dispatch_operation,
                        requests_satisfied, observer_info, _success, _error):

        # this is a list of pending channel requests that we're waiting to be
        # ready for this request before we return _success()
        pending_channels = []

        def channel_closed(chan):
            print '%s closed' % chan
            del self.channels[chan.object_path]

        def channel_ready(chan):
            print '%s ready' % chan

            pending_channels.remove(chan)
            chan.connect('closed', channel_closed)

            if len(pending_channels) == 0:
                print 'All channels ready'

                _success()

        def open_channels(conn):
            print "Opening channels"
            for path, properties in channels:
                chan = self.channels[path] = EChannel(account, conn, path,
                    properties, channel_ready)
                pending_channels.append(chan)

        def connection_disconnected(conn):
            print '%s disconnected' % conn
            del self.connections[conn.object_path]

        def connection_ready(conn):
            print '%s ready' % conn

            conn.connect('disconnected', connection_disconnected)

            open_channels(conn)

        # look to see if we need to create the connection
        if connection not in self.connections:
            self.connections[connection] = EConnection (connection,
                connection_ready)
        else:
            open_channels(self.connections[connection])

def publish(client_name):
    service_name = '.'.join ([CLIENT, client_name])
    object_path = '/' + service_name.replace('.', '/')

    bus_name = dbus.service.BusName(service_name, bus=dbus.SessionBus())

    EObserver(bus_name, object_path)
    return False

if __name__ == '__main__':
    gobject.timeout_add(0, publish, 'ExampleObserver2')
    loop = gobject.MainLoop()
    loop.run()
