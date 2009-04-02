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
import re
from lxml import etree

try:
	import pygments
	import pygments.lexers
	import pygments.formatters
except ImportError:
	print >> sys.stderr, "WARNING: install python-pygments for syntax highlighting"

if pygments:
	class HtmlFormatter(pygments.formatters.HtmlFormatter):
		"We don't want our highlighted source wrapped in <pre>"
		def wrap (self, source, outfile):
			return source

doc = etree.parse (sys.stdin)
examplesdir = sys.argv[1]

included_files = {}

# find all example elements that have the 'file' attribute
examples = doc.xpath ("//example[@file]")

for example in examples:

	id = example.get ('id')

	nicename = example.get ('file')
	filename = os.path.join (examplesdir, nicename)

	del example.attrib['file'] # unset the file property

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

		begin_re = re.compile ('begin %s(\\s|$)' % re.escape (id), re.I)
		end_re = re.compile ('end %s(\\s|$)' % re.escape (id), re.I)

		begin = False
		lines = []
		for line in contents.split ('\n'):
			if begin and end_re.search (line):
				break
			elif begin:
				lines.append (line)
			elif not begin and begin_re.search (line):
				begin = True
				continue

		if lines != []:
			# trip the common indent off the front of the example
			def leading_space (s):
				n = 0
				for c in s:
					if not c.isspace (): break
					n += 1
				return n
			lines = map (lambda s: s.expandtabs(), lines)
			trim = min ([ leading_space(s) for s in lines if s != "" ])

			contents = '\n'.join (map (lambda s: s[trim:], lines))
	else:
		print >> sys.stderr, "Including file `%s'..." % filename

	if pygments:
		# syntax highlighting
		lexer = pygments.lexers.get_lexer_for_filename (filename)
		contents = pygments.highlight (contents, lexer,
					HtmlFormatter(noclasses=True))
	contents = "<embedhtml>%s</embedhtml>" % contents
	# print >> sys.stderr, contents
	# sys.exit(-1)

	etree.SubElement (example, 'programlisting').append (etree.XML (contents))
	p = etree.SubElement (example, 'para')
	xmlname = re.sub (r'/', '.', nicename)
	etree.SubElement (p, 'link', linkend='anchor.%s.%s' % (xmlname, id)).text = "Complete Source Code"

	f.close ()

# build an appendix of source files
appendix = doc.xpath ('/book/appendix[@id="source-code"]')[0]
for nicename in included_files:
	filename = os.path.join (examplesdir, nicename)
	xmlname = re.sub (r'/', '.', nicename)

	s = etree.SubElement (appendix, 'sect1',
				id='appendix.source-code.%s' % xmlname)
	etree.SubElement (s, 'title').text = nicename

	try:
		f = open (filename)
		contents = f.read ().rstrip ()
	except IOError, e:
		print >> sys.stderr, e
		continue

	# syntax highlight the contents
	if pygments:
		# syntax highlighting
		lexer = pygments.lexers.get_lexer_for_filename (filename)
		contents = pygments.highlight (contents, lexer,
					HtmlFormatter(noclasses=True))
	contents = "<embedhtml>%s</embedhtml>" % contents
	
	# find the starting offsets for each id in the file
	def get_tuple (prefix, id):
		str = '%s %s' % (prefix, id)
		m = re.search (str + '(\\s|$)', contents)
		if m is None: idx = None
		else:
			idx = m.span()[0]
		return (idx, prefix, id, len (str))
	

	for id in included_files[nicename]:
		sre = re.compile ('(\\W)begin (%s)([^a-zA-Z0-9_\\-])' % re.escape (id), re.I | re.M)
		contents = sre.sub(r'\1Begin <embeddb><anchor id="anchor.%s.%s"/><xref linkend="\2"/></embeddb>\3' % (xmlname, id), contents)

		sre = re.compile ('(\\W)end (%s)([^a-zA-Z0-9_\\-])' % re.escape (id), re.I | re.M)
		contents = sre.sub(r'\1End <embeddb><xref linkend="\2"/></embeddb>\3', contents)

	pl = etree.SubElement (s, 'programlisting')
	pl.append (etree.XML (contents))

sys.stdout.write (etree.tostring (doc))
