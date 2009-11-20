example_dirs = \
	glib_list_protocols \
	glib_get_user_defined_groups \
	glib_get_roster \
	glib_salut_ft \
	glib_jabber_muc \
	glib_telepathy_properties \
	glib_dbus_tube \
	glib_stream_tube \
	glib_mc5_connections \
	glib_mc5_dbus_tube_handler \
	glib_mc5_observer \
	glib_mc5_ft_handler \
	gtk_presence_app \
	pygtk_chat_client \
	python_get_parameters \
	python_iface_messaging \
	python_irc_roomlist \
	python_mc5_clients \
	python_simple_presence

SUBDIRS = $(example_dirs)

prep-rsync:
	rm -rf $(builddir)/dist/
	mkdir -p $(builddir)/dist/
	for subdir in $(example_dirs); do \
		$(MAKE) -C $$subdir $@ DIR=$(builddir)/dist/$$subdir; \
	done

.PHONY: prep-rsync
