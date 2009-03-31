include $(top_srcdir)/docs/rsync-properties.make

example_dirs = \
	glib_list_protocols \
	glib_get_roster

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


