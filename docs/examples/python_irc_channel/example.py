#!/usr/bin/env python

import os
import sys

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
from telepathy.interfaces import *
from telepathy.constants import *

DBUS_PROPERTIES = dbus.PROPERTIES_IFACE

mainloop = gobject.MainLoop()

def ignore(*args): pass

class Room(telepathy.client.Channel):
    def __init__(self, connection, object_path, properties):
        self.conn = connection

        # Channel can't use properties in Python :(
        telepathy.client.Channel.__init__(self,
            connection.service_name,
            object_path,
            ready_handler = self._ready)

        # map of handles to aliases
        self._members = {}

    def _ready(self, channel):
        print "channel ready"

        self[CHANNEL_INTERFACE_GROUP].connect_to_signal('MembersChanged',
            self.members_changed)
        self[CHANNEL_INTERFACE_GROUP].GetMembers(
            reply_handler = self.update_members,
            error_handler = self.conn.error)

        self[CHANNEL_TYPE_TEXT].connect_to_signal('Received',
            self.received_message)

    def update_members(self, handles):
        self.conn[CONNECTION].InspectHandles (HANDLE_TYPE_CONTACT, handles,
            reply_handler = lambda ids: self._members.update(zip(handles, ids)),
            error_handler = self.conn.error)

    def members_changed(self, message, added, removed,
                        local_pending, remote_pending, actor, reason):

        for h in removed: del self._members[h]
        self.update_members (added)

    def received_message(self, id, timestamp, sender_handle, type, flags, text):
        sender = self._members.get(sender_handle, "???")
        print "<%s> %s" % (sender, text)

class Connection(telepathy.client.Connection):
    def __init__(self, *args):
        telepathy.client.Connection.__init__(self, *args,
            ready_handler = self._ready)

        self[CONNECTION].connect_to_signal('StatusChanged',
            self._status_changed)

        self[CONNECTION].Connect(reply_handler = ignore,
                                 error_handler = self.error)

    def _ready(self, connection):
        print "Connection ready"

        # open a channel
        self[CONNECTION_INTERFACE_REQUESTS].EnsureChannel({
            CHANNEL + ".ChannelType": CHANNEL_TYPE_TEXT,
            CHANNEL + ".TargetHandleType": HANDLE_TYPE_ROOM,
            CHANNEL + ".TargetID": "#telepathy",
            },
            reply_handler = self.got_channel,
            error_handler = self.error)

    def got_channel(self, yours, object_path, properties):
        Room(self, object_path, properties)

    def _status_changed(self, status, reason):
        if status == CONNECTION_STATUS_DISCONNECTED:
            print 'Disconnected'
            mainloop.quit()

        if status != CONNECTION_STATUS_CONNECTED: return

        print 'Connected'

    def disconnect(self):
        self[CONNECTION].Disconnect(reply_handler = ignore,
                                    error_handler = ignore)

    def error(self, *args):
        print "ERROR", self, args
        self.disconnect()

class Example(object):
    def __init__(self):
        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        self.cm = reg.GetManager('idle') # IRC

    def tp_connect(self, name, server):
        self.cm[CONNECTION_MANAGER].RequestConnection('irc',
            {
                'account'   : name,
                'server'    : server
            },
            reply_handler = self.got_connection,
            error_handler = self.error)

    def got_connection(self, bus_name, object_path):
        self.conn = Connection(bus_name, object_path)

    def disconnect(self):
        self.conn.disconnect()

    def error(self, *args):
        print "ERROR", self, args

if __name__ == '__main__':
    ex = Example()
    ex.tp_connect(sys.argv[1], sys.argv[2])
    try:
        mainloop.run()
    except KeyboardInterrupt:
        ex.disconnect()
        mainloop.run()
