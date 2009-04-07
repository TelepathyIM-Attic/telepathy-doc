include $(top_srcdir)/docs/rsync-properties.make

example_dirs = \
	glib_list_protocols \
	glib_get_user_defined_groups \
	glib_get_roster \
	glib_salut_ft \
	pygtk_chat_client \
	python_get_parameters \
	python_iface_messaging \
	python_irc_roomlist \
	python_simple_presence

SUBDIRS = $(example_dirs)

exclude_args = \
	--delete-excluded \
	--exclude *.o \
	--exclude .libs \
	--exclude .deps \
	--exclude core \
	--exclude .cvsignore \
	--exclude a.out \
	--exclude Makefile \
	--exclude Makefile.in \
	--exclude example \
	--exclude .svn \
	--exclude .git

post-html:
	rsync $(rsync_args) -a $(exclude_args) $(example_dirs) $(rsync_target_html)/examples/


