import gtk
import gobject

class RosterWindow(gtk.Window):

    def __init__(self, sm):
        self.sm = sm
        super(RosterWindow, self).__init__()

        vbox = gtk.VBox()
        self.add(vbox)

        sw = gtk.ScrolledWindow()
        vbox.pack_start (sw)

        tv = gtk.TreeView()
        sw.add(tv)

        vbox.show_all()

        self.contacts = gtk.ListStore(gobject.TYPE_PYOBJECT)
        self.sm.connect('contacts-updated', self._contacts_updated)

    def _contacts_updated(self, sm):
        print '::contacts-updated'

gobject.type_register(RosterWindow)
