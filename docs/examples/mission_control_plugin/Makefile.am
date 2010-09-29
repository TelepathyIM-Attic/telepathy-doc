AM_CPPFLAGS = \
	$(TELEPATHY_GLIB_CFLAGS)

pluginsdir = $(MISSION_CONTROL_PLUGINS_DIR)
plugins_LTLIBRARIES = \
	mcp-account-manager-socialweb.la

mcp_account_manager_socialweb_la_SOURCES = \
	mission-control-plugin.c \
	mcp-account-manager-example.c mcp-account-manager-example.h

mcp_account_manager_socialweb_la_LIBADD = \
	$(MISSION_CONTROL_MODULE_LIBS)

mcp_account_manager_socialweb_la_LDFLAGS = \
	-module \
	-avoid-version
