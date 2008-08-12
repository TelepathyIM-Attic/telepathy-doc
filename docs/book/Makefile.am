### This part of Makefile.am can be customized by you.

# gnome-doc-utils standard variables:
include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

# The name of the directory in /usr/share/gnome/help/, 
# and the name of the main .xml file:
# TODO: Use flumotion-with-examples for all languages.
DOC_MODULE = flumotion

# The names of any files included via entity declarations.
DOC_ENTITIES =

# The names of any files included by xincluded (preferred):
DOC_INCLUDES = bibliography.xml legal.xml \
               component-asf-muxer.xml \
               component-ical-bouncer.xml \
               component-screencast-producer.xml \
               component-audiotest-producer.xml \
               component-icecast-producer.xml \
               component-shout2-consumer.xml \
               component-av-basic-watchdog-combiner.xml \
               component-ivtv-producer.xml \
               component-single-basic-watchdog-combiner.xml \
               component-av-pattern-switch-combiner.xml \
               component-jpeg-encoder.xml \
               component-single-switch-combiner.xml \
               component-av-switch-combiner.xml \
               component-loop-producer.xml \
               component-smoke-encoder.xml \
               component-bouncer.xml \
               component-mp3-encoder.xml \
               component-soundcard-producer.xml \
               component-composite-combiner.xml \
               component-mulaw-encoder.xml \
               component-speex-encoder.xml \
               component-dirac-encoder.xml \
               component-multipart-muxer.xml \
               component-theora-encoder.xml \
               component-disk-consumer.xml \
               component-ogg-muxer.xml \
               component-tokentest-bouncer.xml \
               component-firewire-producer.xml \
               component-overlay-converter.xml \
               component-tvcard-producer.xml \
               component-flv-encoder.xml \
               component-pipeline-converter.xml \
               component-unixdomain-producer.xml \
               component-flv-muxer.xml \
               component-pipeline-producer.xml \
               component-videotest-producer.xml \
               component-gdp-consumer.xml \
               component-playlist-producer.xml \
               component-vorbis-encoder.xml \
               component-gdp-producer.xml \
               component-porter.xml \
               component-vp6-encoder.xml \
               component-htpasswdcrypt-bouncer.xml \
               component-preview-consumer.xml \
               component-webcam-producer.xml \
               component-http-digest-bouncer.xml \
               component-repeater.xml \
               component-windows-media-server-producer.xml \
               component-http-server.xml \
               component-rtsp-producer.xml \
               component-wma-encoder.xml \
               component-http-streamer.xml \
               component-saltsha256-bouncer.xml \
               component-wmv-encoder.xml

# The names of any pictures:
DOC_FIGURES = \
	figures/fluendo.png \
	figures/sshot-wizard-startnew.png \
	figures/sshot-wizard-connect-to-new.png \
	figures/sshot-wizard-scenario.png \
	figures/sshot-wizard-consumption.png \
	figures/sshot-wizard-httpstreamer.png \
	figures/sshot-wizard-source.png \
	figures/sshot-wizard-videocompprop.png \
	figures/sshot-connection-begin.png \
	figures/sshot-wizard-encoding.png \
	figures/sshot-wizard-overlay.png \
	figures/sshot-wizard-theora.png \
	figures/sshot-wizard-welcome.png \
	figures/sshot-wizard-production.png \
	figures/sshot-wizard-production-test-inputs.png \
	figures/sshot-wizard-webcam.png \
	figures/sshot-wizard-overlay.png \
	figures/sshot-wizard-soundcard.png \
	figures/sshot-wizard-encoding.png \
	figures/sshot-wizard-summary.png \
	figures/sshot-admin-window.png \
	figures/sshot-client-player.png \
	figures/sshot-client-player-test-inputs.png \
	figures/sshot-services-gui-fedora.png \
	figures/diagram-architecture.png \
	figures/diagram-architecture-with-workers.png

# The names of any locales for which documentation translations exist:
DOC_LINGUAS = de


# Create a DocBook source file that doesn't have the examples' comments blocks:
C/flumotion-with-examples.xml: C/flumotion.xml insert_example_code.pl
	$(PERL_PATH) $(srcdir)/insert_example_code.pl $(top_srcdir)/doc/examples $< >$@


# Commented-out because we don't want to do this every time:
# Generate docbook for components from the documentation in the Flumotion registry:
# C/component-%.xml: generate_component_docbook.py
	

generate-component-docbook:
	cd C && export PYTHONPATH=/usr/lib/flumotion/python && $(PYTHON) $(abs_srcdir)/generate_component_docbook.py


# main xml file for the doc
MAIN = C/$(DOC_MODULE)-with-examples.xml
HTML_STYLESHEET = manual.css

XSLTPROC = xsltproc
DOCBOOK_STYLESHEET ?= http://docbook.sourceforge.net/release/xsl/current/html/chunk.xsl

# Create a html generation of the C locale's version of the DocBook, 
# using our custom stylesheet and graphics:
# Set the use.id.as.filename param so that we don't use the chapter / section
# number as the filename, otherwise the URL will change every time anything is
# re-ordered or inserted in the documentation
#
# We set refentry.generate.name and refentry.generate.title to get the title 
# instead of "Name" as the title.
html/index.html: $(MAIN) Makefile.am
	rm -rf html
	$(mkinstalldirs) html
	cp -r ../examples/ html/
	cp -r C/figures html/
	cp -r C/icons html/
	-rm -rf html/*/.svn
	cp $(HTML_STYLESHEET) html/
	$(XSLTPROC) \
		--param toc.section.depth 1	\
		--stringparam html.stylesheet "$(HTML_STYLESHEET)"	\
		--stringparam admon.graphics 1	\
		--stringparam admon.graphics.path "icons/"	\
		--stringparam admon.graphics.extension ".png"	\
		--stringparam chunker.output.indent yes	\
		--stringparam chunker.output.encoding UTF-8	\
		--stringparam navig.graphics yes	\
		--stringparam navig.graphics.extension ".png"	\
		--stringparam navig.graphics.path "icons/"	\
		--stringparam toc.list.type "ul"	\
		--stringparam use.id.as.filename "1" \
		--stringparam refentry.generate.name "0" \
		--stringparam refentry.generate.title "1" \
		-o html/ --xinclude --catalogs $(DOCBOOK_STYLESHEET)	\
		$<

html: html/index.html

# We have to generate the pdf in a subdirectory (e.g. pdf/) because the tutorial
# specifies the path to the figures as '../figures' so if we build it in this
# directory, it won't find the images.

DB2PDF = docbook2pdf
XMLLINT = xmllint

# docbook2pdf can't process xi:include, so we process them first with xmllint:
# Setting SP_ENCODING to "XML" avoids errors about unicode characters from jade.
pdf/flumotion.pdf: $(MAIN)
	rm -rf pdf
	$(mkinstalldirs) pdf
	cp -r ../examples/ pdf/
	xmllint --xinclude $(MAIN) > C/flumotion_manual.xml
	SP_ENCODING="XML" $(DB2PDF) --output pdf C/flumotion_manual.xml

pdf: pdf/flumotion.pdf


# Validate the source XML, not the one with the examples added in:
validate: C/$(DOC_MODULE).xml
	$(XMLLINT) --xinclude --postvalid --noout $<

include $(top_srcdir)/doc/Makefile_web.am_fragment

post-online: post-html post-pdf

post-html: html
	rsync $(rsync_args) -r html/ murrayc@$(web_host):$(web_path_docs)/html/
	rsync $(rsync_args) -r pdf/ murrayc@$(web_host):$(web_path_docs)/pdf/

post-pdf: pdf
	rsync $(rsync_args) -r pdf/ murrayc@$(web_host):$(web_path_docs)/pdf/

CLEANFILES = C/flumotion-with-examples.xml C/flumotion_manual.xml
EXTRA_DIST = generate_component_docbook.py

