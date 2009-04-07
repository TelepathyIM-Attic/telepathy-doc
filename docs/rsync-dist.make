prep-rsync:
	mkdir -p ../$(DIR)
	for file in $(DIST_SOURCES) $(noinst_PYTHON); do \
		$(INSTALL) $$file ../$(DIR); \
	done

.PHONY: prep-rsync
