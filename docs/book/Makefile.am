### This part of Makefile.am can be customized by you.

# gnome-doc-utils standard variables:
include $(top_srcdir)/gnome-doc-utils.make
dist-hook: doc-dist-hook

# The name of the directory in /usr/share/gnome/help/, 
# and the name of the main .xml file:
# TODO: Use telepathy-with-examples for all languages.
DOC_MODULE = telepathy

# The names of any files included via entity declarations.
DOC_ENTITIES =

# The names of any files included by xincluded (preferred):
DOC_INCLUDES = \
	legal.xml \
	introduction.xml \
	basics.xml \
	accounts.xml \
	channel-dispatcher.xml \
	connection.xml \
	channel.xml \
	tubes.xml

# these are figures that need to be rasterised
DOC_FIGURES_SRC = \
	telepathy-architecture-overview.svg \
	bus-conceptual.svg \
	bus-hierarchy-conceptual.svg

# The names of any pictures:
DOC_FIGURES = 

# The names of any locales for which documentation translations exist:
DOC_LINGUAS = de

# Create a DocBook source file that doesn't have the examples' comments blocks:
C/telepathy-with-examples.xml: C/telepathy.xml $(_DOC_C_DOCS) insert-links.py insert-examples.py
	$(srcdir)/insert-links.py $< | $(srcdir)/insert-examples.py $(top_srcdir)/docs/examples/ >$@


# main xml file for the doc
MAIN = C/$(DOC_MODULE)-with-examples.xml
HTML_STYLESHEET = book.css

XSLTPROC = xsltproc
DOCBOOK_STYLESHEET ?= $(shell pkg-config --variable db2html gnome-doc-utils)
ICONS_DIR ?= $(shell pkg-config --variable icondir gnome-doc-utils)/hicolor/48x48

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
	$(mkinstalldirs) html/icons
	cp $(ICONS_DIR)/* html/icons/
	$(XSLTPROC) \
		--param db.chunk.chunk_top 0 \
		--stringparam db.chunk.basename "index" \
		--stringparam db.chunk.extension ".html" \
		--stringparam theme.icon.admon.path "icons/" \
		-o html/index.html --nonet --xinclude \
		$(DOCBOOK_STYLESHEET) \
		$<

# $(XSLTPROC) \
# 	--param toc.section.depth 1	\
# 	--stringparam html.stylesheet "$(HTML_STYLESHEET)"	\
# 	--stringparam admon.graphics 1	\
# 	--stringparam admon.graphics.path "icons/"	\
# 	--stringparam admon.graphics.extension ".png"	\
# 	--stringparam chunker.output.indent yes	\
# 	--stringparam chunker.output.encoding UTF-8	\
# 	--stringparam navig.graphics yes	\
# 	--stringparam navig.graphics.extension ".png"	\
# 	--stringparam navig.graphics.path "icons/"	\
# 	--stringparam toc.list.type "ul"	\
# 	--stringparam use.id.as.filename "1" \
# 	--stringparam refentry.generate.name "0" \
# 	--stringparam refentry.generate.title "1" \
# 	-o html/ --xinclude --catalogs $(DOCBOOK_STYLESHEET)	\
# 	$<

html: html/index.html

# We have to generate the pdf in a subdirectory (e.g. pdf/) because the tutorial
# specifies the path to the figures as '../figures' so if we build it in this
# directory, it won't find the images.

DB2PDF = docbook2pdf
XMLLINT = xmllint

# docbook2pdf can't process xi:include, so we process them first with xmllint:
# Setting SP_ENCODING to "XML" avoids errors about unicode characters from jade.
pdf/telepathy.pdf: $(MAIN)
	rm -rf pdf
	$(mkinstalldirs) pdf
	cp -r ../examples/ pdf/
	xmllint --xinclude $(MAIN) > C/telepathy_manual.xml
	SP_ENCODING="XML" $(DB2PDF) --output pdf C/telepathy_manual.xml

pdf: pdf/telepathy.pdf


# Validate the source XML, not the one with the examples added in:
validate: C/$(DOC_MODULE).xml
	$(XMLLINT) --xinclude --postvalid --noout $<

include $(top_srcdir)/docs/rsync-properties.make
include $(top_srcdir)/docs/generate-figures.make

post-online: post-html post-pdf

post-html: html
	rsync $(rsync_args) -r html/ $(rsync_target_html)

post-pdf: pdf
	rsync $(rsync_args) -r pdf/ $(rsync_target_pdf)

CLEANFILES = C/telepathy-with-examples.xml
EXTRA_DIST = 
