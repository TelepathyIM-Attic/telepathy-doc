dnl configure.ac for template xml docbook
AC_INIT(flumotion-doc, 0.5.0.1)
AM_INIT_AUTOMAKE([-Wno-portability])

dnl Add parameters for aclocal
AC_SUBST(ACLOCAL_AMFLAGS, "-I m4")

dnl add a parameter to set output formats
OUTPUT_FORMATS="xml html txt" # ps and pdf disabled until it works
AC_ARG_WITH(docbook-output-formats,
  AC_HELP_STRING([--with-docbook-output-formats],
                 [list of docbook output formats to generate]),
  [OUTPUT_FORMATS=`echo ${withval} | sed 's/,/ /'`])
AC_MSG_NOTICE(Generating DocBook in: $OUTPUT_FORMATS)

dnl declare which output formats we want to be able to generate
AS_DB_SET_OUTPUT_FORMATS($OUTPUT_FORMATS)

dnl add a parameter to set image formats
dnl this is just for testing, in reality you decide this here based on
dnl your used source images
# IMAGE_FORMATS="fig png ps"
# AC_ARG_WITH(docbook-image-formats,
#  AC_HELP_STRING([--with-docbook-image-formats],
#                 [list of docbook source image formats to use]),
#   [IMAGE_FORMATS=${withval}])
#  IMAGE_FORMATS=`echo $IMAGE_FORMATS | sed 's/,/ /'`
# AC_MSG_NOTICE(Generating images from source formats: $IMAGE_FORMATS)

# dnl declare which image source formats we want to use as source
# AS_DB_SET_IMAGE_FORMATS($IMAGE_FORMATS)

# dnl hook checks for all the image generation programs we need
# AS_DB_CHECK_IMAGE(:,[
#   AC_MSG_ERROR(Cannot generate images)
# ])

dnl Use gnome-doc-utils:
GNOME_DOC_INIT([0.9.0])

# Get the path to perl, so we can use perl scripts without hard-coding the path.
FLUMOTION_DOCS_CHECK_PERL([5.6.0])

# Get the path to python, so we can use python scripts without hard-coding the path.
AS_PATH_PYTHON(2.3)

# This would be needed if we built the component documentation each time.
# dnl check for flumotion
# PKG_CHECK_MODULES(FLUMOTION, flumotion >= 0.5.0)

AC_OUTPUT([
  Makefile
  m4/Makefile
  doc/Makefile
  doc/examples/Makefile
  doc/manual/Makefile
  flumotion-doc.spec
])
