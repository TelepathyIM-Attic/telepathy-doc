example_files = example_conf_planet_testinputs.xml \
                example_conf_worker.xml \
                example_conf_usage_conference_manager.xml \
                example_conf_usage_conference_worker.xml \
                example_conf_usage_ondemand_manager.xml

EXTRA_DIST = $(example_files)

# Install the examples/ directory next to the DocBook XML.
# TODO: Is there a better place to put this, still with the same relative link 
# For the HTML and PDF?
# TODO: Do this for other translations too.
examplesdir   = $(datadir)/gnome/help/flumotion/C/examples
examples_DATA = $(example_files)


