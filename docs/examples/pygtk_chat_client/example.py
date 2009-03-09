#!/usr/bin/env python

import sys

import gtk
import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

import telepathy
import telepathy.client
from telepathy.interfaces import CONNECTION_MANAGER, \
                                 CONNECTION, \
                                 CONNECTION_INTERFACE_REQUESTS, \
                                 CONNECTION_INTERFACE_CONTACTS, \
                                 CONNECTION_INTERFACE_ALIASING, \
                                 CONNECTION_INTERFACE_SIMPLE_PRESENCE, \
                                 CHANNEL, \
                                 CHANNEL_TYPE_CONTACT_LIST, \
                                 CHANNEL_TYPE_TEXT, \
                                 CHANNEL_INTERFACE_MESSAGES, \
                                 CHANNEL_INTERFACE_GROUP
from telepathy.constants import CONNECTION_STATUS_CONNECTED, \
                                CONNECTION_STATUS_DISCONNECTED, \
                                HANDLE_TYPE_CONTACT, \
                                HANDLE_TYPE_LIST, \
                                HANDLE_TYPE_GROUP

DBUS_PROPERTIES = 'org.freedesktop.DBus.Properties'

def generic_reply(*args): pass

class Connection(telepathy.client.Connection):
    def __init__(self, sm, bus_name, object_path):
        super(Connection, self).__init__(bus_name, object_path)

        self.sm = sm

        self[CONNECTION].connect_to_signal('StatusChanged', self.status_changed)
        self[CONNECTION].Connect(reply_handler = generic_reply,
                                 error_handler = self.sm.error)

    def status_changed(self, status, reason):
        if status == CONNECTION_STATUS_DISCONNECTED:
            print 'Disconnected!'
            # FIXME: signal disconnection with the SM
            gtk.main_quit()
            return

        elif status != CONNECTION_STATUS_CONNECTED:
            return

        print 'Carrier Detected'
        # NB. this should be a property eventually
        self[CONNECTION].GetInterfaces(reply_handler = self._interfaces_cb,
                                       error_handler = self.sm.error)

    def _interfaces_cb(self, interfaces):
        self.interfaces = interfaces

        self.sm.connection_ready(self)

    def disconnect(self):
        self[CONNECTION].Disconnect(reply_handler = generic_reply,
                                    error_handler = self.sm.error)

    def ensure_channel(self, channel_obj, handle_type, target_id, props = {}):

        d = {
            CHANNEL + '.ChannelType'      : channel_obj.channel_type,
            CHANNEL + '.TargetHandleType' : handle_type,
            CHANNEL + '.TargetID'         : target_id,
        }

        d.update(props)

        def _ensure_channel_error(self, error):
            print 'Channel could not be created for dict: %s' % d

        if CONNECTION_INTERFACE_REQUESTS in self.interfaces:
            self[CONNECTION_INTERFACE_REQUESTS].EnsureChannel(d,
                reply_handler = lambda a, b, c: channel_obj(self, b, a),
                error_handler = self._ensure_channel_error)
        else:
            self.sm.error("Requests interface unavailable, get a better CM")

class Channel(telepathy.client.Channel):
    def __init__(self, conn, object_path, yours = False):
        self.conn = conn
        self.sm = conn.sm
        self.yours = yours

        print 'Channel came up... requesting interfaces'
        super(Channel, self).__init__(conn.service_name, object_path)

        self[DBUS_PROPERTIES].Get (CHANNEL, 'Interfaces',
                                   reply_handler = self._interfaces_cb,
                                   error_handler = self.sm.error)
    
    def _interfaces_cb(self, interfaces):
        self.interfaces = interfaces

        self.ready()

    def ready(self):
        pass

class ContactList(Channel):
    channel_type = CHANNEL_TYPE_CONTACT_LIST

    def ready(self):
        # get the list of handles
        if CHANNEL_INTERFACE_GROUP in self.interfaces:
            self[DBUS_PROPERTIES].Get(CHANNEL_INTERFACE_GROUP, 'Members',
                reply_handler = self._members_cb,
                error_handler = self.sm.error)
        else:
            print 'Channel does not implement Group... strange'

    def _members_cb(self, handles):
        # look them up via the contacts interface
        if CONNECTION_INTERFACE_CONTACTS in self.conn.interfaces:
            self.conn[CONNECTION_INTERFACE_CONTACTS].GetContactAttributes(
                handles,
                [
                 CONNECTION,
                 CONNECTION_INTERFACE_ALIASING,
                 CONNECTION_INTERFACE_SIMPLE_PRESENCE,
                ],
                False,
                reply_handler = self._attributes_cb,
                error_handler = self.sm.error)

    def _attributes_cb(self, map):
        for handle, attributes in map.iteritems():
            contact = Contact (self.sm, handle, attributes)
            self.sm.contacts[handle] = contact

        self.sm.contacts_updated(map.keys())

class Contact(object):
    def __init__(self, sm, handle, attributes={}):
        self.sm = sm
        self.handle = handle

        for key, value in attributes.iteritems():
            if key == CONNECTION + '/contact-id':
                self.contact_id = value
            elif key == CONNECTION_INTERFACE_ALIASING + '/alias':
                self.alias = value
            elif key == CONNECTION_INTERFACE_SIMPLE_PRESENCE + '/presence':
                self.presence = value

    def get_status(self):
        return self.presence[1]

class StateMachine(object):
    def __init__(self, account, password):
        """e.g. account  = 'bob@example.com/test'
                password = 'bigbob'
        """

        self.contacts = {}

        reg = telepathy.client.ManagerRegistry()
        reg.LoadManagers()

        # get the gabble Connection Manager
        self.cm = cm = reg.GetManager('gabble')

        print 'Connecting...'
        cm[CONNECTION_MANAGER].RequestConnection('jabber',
            {
                'account':  account,
                'password': password,
            },
            reply_handler = lambda a, b: Connection (self, a, b),
            error_handler = self.error)

    def error(self, error):
        print "Telepathy Error: %s" % error
        print "Disconnecting..."
        self.disconnect()

    def connection_ready(self, conn):
        print "Connection Ready"
        self.conn = conn

        # set up signals for interfaces we're interested in
        if CONNECTION_INTERFACE_ALIASING in conn.interfaces:
            conn[CONNECTION_INTERFACE_ALIASING].connect_to_signal(
                'AliasesChanged', self._aliases_changed)

        if CONNECTION_INTERFACE_SIMPLE_PRESENCE in conn.interfaces:
            conn[CONNECTION_INTERFACE_SIMPLE_PRESENCE].connect_to_signal(
                'PresencesChanged', self._presences_changed)

        # request the contact lists
        print 'Requesting roster...'
        self.conn.ensure_channel (ContactList, HANDLE_TYPE_LIST, 'subscribe')
        self.conn.ensure_channel (ContactList, HANDLE_TYPE_LIST, 'publish')

    def _aliases_changed(self, aliases):
        for handle, alias in aliases:
            if handle not in self.contacts: continue
            self.contacts[handle].alias = alias

        self.contacts_updated(map(lambda (h, a): h, aliases))

    def _presences_changed(self, presences):
        for handle, presence in presences.iteritems():
            if handle not in self.contacts: continue
            self.contacts[handle].presence = presence

        self.contacts_updated(presences.keys())

    def contacts_updated(self, handles):
        print ' -- Contacts updated --'

        for contact in [ self.contacts[h] for h in handles if h in self.contacts]:
            print "%s: %s (%s)" % (
                contact.contact_id, contact.alias, contact.get_status())

    def disconnect(self):
        try:
            self.conn.disconnect()
        except:
            gtk.main_quit()

if __name__ == '__main__':
    import getpass
    account = sys.argv[1]
    password = getpass.getpass()
    
    sm = StateMachine(account, password)
        
    try:
        print 'Running...'
        gtk.main()
    except KeyboardInterrupt:
        print "Terminating connection..."
        sm.disconnect()
        # reengage the mainloop so that we can disconnect cleanly
        gtk.main()
