include $(top_srcdir)/docs/Makefile_web.am_fragment

example_dirs = connect list_all_protocols list_contacts set_presence send_message

SUBDIRS = $(example_dirs)

EXTRA_DIST =  Makefile.am_fragment

exclude_args = --delete-excluded --exclude *.o --exclude .libs --exclude .deps --exclude core --exclude .cvsignore --exclude a.out --exclude Makefile --exclude Makefile.in --exclude example --exclude .svn
post-html:
	rsync $(rsync_args) -a $(exclude_args) $(example_dirs) $$USER@$(web_host):$(web_path)examples


