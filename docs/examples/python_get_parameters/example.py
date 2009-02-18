#!/usr/bin/env python

import gobject
import dbus.mainloop.glib
dbus.mainloop.glib.DBusGMainLoop(set_as_default = True)

# begin example.connection.get_parameters
import telepathy
from telepathy.interfaces import CONN_MGR_INTERFACE
from telepathy.constants import CONN_MGR_PARAM_FLAG_DBUS_PROPERTY, \
                                CONN_MGR_PARAM_FLAG_HAS_DEFAULT, \
                                CONN_MGR_PARAM_FLAG_REGISTER, \
                                CONN_MGR_PARAM_FLAG_REQUIRED, \
                                CONN_MGR_PARAM_FLAG_SECRET

def print_params (params):
    for name, flags, signature, default in params:
        print "%s (%s)" % (name, signature),
    
        if flags & CONN_MGR_PARAM_FLAG_REQUIRED: print "required",
        if flags & CONN_MGR_PARAM_FLAG_REGISTER: print "register",
        if flags & CONN_MGR_PARAM_FLAG_SECRET: print "secret",
        if flags & CONN_MGR_PARAM_FLAG_DBUS_PROPERTY: print "dbus-property",
        if flags & CONN_MGR_PARAM_FLAG_HAS_DEFAULT: print "has-default(%s)" % default,
    
        print

    loop.quit ()

def error_cb (*args):
    print "Error:", args
    loop.quit ()

reg = telepathy.client.ManagerRegistry()
reg.LoadManagers()

# get the gabble Connection Manager
cm = reg.GetManager('gabble')

# get the parameters required to make a Jabber connection
cm[CONN_MGR_INTERFACE].GetParameters('jabber',
    reply_handler = print_params, error_handler = error_cb)
# end example.connection.get_parameters

loop = gobject.MainLoop()
loop.run()
