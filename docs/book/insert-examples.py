#!/usr/bin/env python
#
# Insert examples into a Docbook document.
#
# This program searches the code for instances of:
#    <example file="some file">
# and adds the contents of that file to the example.
#
# Additionally, if an id is defined for the example. It will only attempt
# to include the section between the commented lines: 'begin id' and 'end id'
#
# Copyright (C) 2009 Collabora Ltd.
#
# Authors: Davyd Madeley <davyd.madeley@collabora.co.uk>

import sys
import os.path
from lxml import etree

doc = etree.parse (sys.stdin)
examplesdir = sys.argv[1]

included_files = {}

# find all example elements that have the 'file' attribute
examples = doc.xpath ("//example[@file]")

for example in examples:

	id = example.get ('id')

	nicename = example.get ('file')
	filename = os.path.join (examplesdir, nicename)

	# del (example.attrib['file']) # unset the file property

	# attempt to load the file
	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print >> sys.stderr, e
		continue
	
	if nicename not in included_files:
		included_files[nicename] = []
	if id is not None:
		included_files[nicename].append (id)

	if id:
		print >> sys.stderr, "Including `%s' from `%s'..." % (id, filename)
		
		begin = False
		lines = []
		for line in contents.split ('\n'):
			if begin and line.find ('end %s' % id) != -1:
				break
			elif begin:
				lines.append (line)
			elif not begin and line.find ('begin %s' % id) != -1:
				begin = True
				continue

		if lines != []: contents = '\n'.join (lines)

	else:
		print >> sys.stderr, "Including file `%s'..." % filename

	etree.SubElement (example, 'programlisting').text = etree.CDATA (contents)
	p = etree.SubElement (example, 'para')
	etree.SubElement (p, 'link', linkend='appendix.source-code.%s' % nicename).text = "Complete Source Code"

	f.close ()

# build an appendix of source files
appendix = doc.xpath ('/book/appendix[@id="source-code"]')[0]
for nicename in included_files:
	filename = os.path.join (examplesdir, nicename)

	s = etree.SubElement (appendix, 'sect1',
				id='appendix.source-code.%s' % nicename)
	etree.SubElement (s, 'title').text = nicename

	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print >> sys.stderr, e
		continue
	
	# FIXME - we want to rewrite begin/end comments with xrefs
	etree.SubElement (s, 'programlisting').text = etree.CDATA (contents)

sys.stdout.write (etree.tostring (doc))
