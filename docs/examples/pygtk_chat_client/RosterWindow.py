import gtk
import gobject

class RosterWindow(gtk.Window):

    def __init__(self, sm):
        self.sm = sm
        super(RosterWindow, self).__init__()

        self.set_default_size(200, 400)

        vbox = gtk.VBox()
        self.add(vbox)

        sw = gtk.ScrolledWindow()
        vbox.pack_start (sw)

        self.contacts = gtk.ListStore(gobject.TYPE_PYOBJECT)
        tv = gtk.TreeView(self.contacts)
        sw.add(tv)

        vbox.show_all()

        self.sm.connect('contacts-updated', self._contacts_updated)

        # set up the treeview
        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn('Contacts', renderer)
        tv.append_column(column)

        def text_func(column, cell, model, iter):
            contact = model.get_value(iter, 0)
            cell.set_property('markup', '%s\n<span size="small" style="italic">%s</span>' % (
                contact.alias, contact.get_status()))

        column.set_cell_data_func(renderer, text_func)

        def sort_func(model, iter1, iter2):
            a = model.get_value(iter1, 0)
            b = model.get_value(iter2, 0)
            return cmp(a,b)

        self.contacts.set_default_sort_func(sort_func)
        self.contacts.set_sort_column_id(-1, gtk.SORT_ASCENDING)

    def _contacts_updated(self, sm, updated):
        # build a set of contacts in the state machine
        contacts = set(self.sm.contacts.values())
        updated = set(updated)

        # iterate through the liststore, updating the contacts
        for row in self.contacts:
            contact = row[0]

            if contact in updated or \
               contact not in contacts:
                self.contacts.remove(row.iter)
                continue

            contacts.remove(contact)

        # iterate through the remaining items in the set, adding them to the
        # list store
        for contact in contacts:
            self.contacts.append ((contact,))

gobject.type_register(RosterWindow)
