#!/usr/bin/env python

import sys

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
import telepathy.client
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_REQUESTS, \
                                 CHANNEL, \
                                 CHANNEL_TYPE_TEXT, \
                                 CHANNEL_INTERFACE_MESSAGES, \
                                 CHANNEL_INTERFACE_GROUP
from telepathy.constants import CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED, \
                                HANDLE_TYPE_CONTACT

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

class TextChannel (telepathy.client.Channel):
    def __init__ (self, parent, channel_path):
        self.parent = parent
        conn = parent.conn

        super (TextChannel, self).__init__ (conn.service_name, channel_path)
        channel = self

        channel[DBUS_PROPERTIES].Get(CHANNEL, 'Interfaces',
                                     reply_handler = self.interfaces_cb,
                                     error_handler = self.parent.error_cb)

    def interfaces_cb (self, interfaces):
        channel = self

        print "Channel Interfaces:"
        for interface in interfaces:
            print " - %s" % interface

        if CHANNEL_INTERFACE_MESSAGES in interfaces:
            channel[CHANNEL_INTERFACE_MESSAGES].connect_to_signal(
                'MessageReceived', self.message_received_cb)
            channel[CHANNEL_INTERFACE_MESSAGES].connect_to_signal(
                'PendingMessagesRemoved', self.pending_messages_removed_cb)

            # find out if we have any pending messages
            channel[DBUS_PROPERTIES].Get(CHANNEL_INTERFACE_MESSAGES,
                'PendingMessages',
                reply_handler = self.get_pending_messages,
                error_handler = self.parent.error_cb)

    def get_pending_messages (self, messages):
        for message in messages:
            self.message_received_cb (message)

    def message_received_cb (self, message):
        channel = self

        # we need to acknowledge the message
        msg_id = message[0]['pending-message-id']
        channel[CHANNEL_TYPE_TEXT].AcknowledgePendingMessages([msg_id],
            reply_handler = self.parent.generic_reply,
            error_handler = self.parent.error_cb)

        print '-' * 78
        print 'Received Message:'
        for d in message:
            print '{'
            for k, v in d.iteritems():
                print '  %s: %s' % (k, v)
            print '}'
        print '-' * 78

    def pending_messages_removed_cb (self, message_ids):
        print "Acked messages %s" % message_ids

class Example (object):
    def __init__ (self, account, password):
        """e.g. account  = 'bob@example.com/test'
                password = 'bigbob'
        """

        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        # get the gabble Connection Manager
        self.cm = cm = reg.GetManager('gabble')

        # get the parameters required to make a Jabber connection
        cm[CONNECTION_MANAGER].RequestConnection('jabber',
            {
                'account':  account,
                'password': password,
            },
            reply_handler = self.request_connection_cb,
            error_handler = self.error_cb)

        self.loop = gobject.MainLoop()
        try:
            self.loop.run()
        except KeyboardInterrupt:
            print "Terminating connection..."
            self.disconnect()
            # reengage the mainloop so that we can disconnect cleanly
            self.loop.run()

    def generic_reply (self, *args): pass

    def error_cb (self, error):
        print "Error:", error
        self.disconnect()

    def disconnect (self):
        self.conn[CONNECTION].Disconnect(reply_handler = self.generic_reply,
                                         error_handler = self.error_cb)

    def request_connection_cb (self, bus_name, object_path):
        print bus_name, object_path
        self.conn = conn = telepathy.client.Connection(bus_name, object_path)

        conn[CONNECTION].connect_to_signal('StatusChanged',
            self.status_changed_cb)

        print "Establishing connection..."
        conn[CONNECTION].Connect(reply_handler = self.generic_reply,
                                 error_handler = self.error_cb)

    def status_changed_cb (self, status, reason):
        conn = self.conn

        if status == CONNECTION_STATUS_DISCONNECTED:
            print "Disconnected!"
            self.loop.quit()

        if status != CONNECTION_STATUS_CONNECTED: return

        print 'Carrier Detected' # remember dialup modems?
        print 'Ctrl-C to disconnect'

        # get a list of interfaces on this connection
        conn[CONNECTION].GetInterfaces(reply_handler = self.get_interfaces_cb,
                                       error_handler = self.error_cb)

    def get_interfaces_cb (self, interfaces):
        conn = self.conn

        print "Connection Interfaces:"
        for interface in interfaces:
            print " - %s" % interface

        if CONNECTION_INTERFACE_REQUESTS in interfaces:
            conn[CONNECTION_INTERFACE_REQUESTS].connect_to_signal('NewChannels',
                self.new_channels_cb)

    def new_channels_cb (self, channels):
        for channel, props in channels:
            if props[CHANNEL + '.ChannelType'] == CHANNEL_TYPE_TEXT:
                print 'New chat from %s' % props[CHANNEL + '.TargetID']
                # let's hook up to this channel
                TextChannel(self, channel)

if __name__ == '__main__':
    import getpass
    password = getpass.getpass()
    Example(sys.argv[1], password)
