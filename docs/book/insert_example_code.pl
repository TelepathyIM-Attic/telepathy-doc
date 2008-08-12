#! /usr/bin/perl -w

#sub main()
{
  my $examples_base = shift(@ARGV);

  $examples_base .= "/" unless($examples_base =~ /\/$/);

  foreach $file (@ARGV)
  {
    open(FILE, $file);

    while(<FILE>)
    {
      print $_;

      #Beginning of comment:
      # Look for
      # <para><ulink url="&url_examples_base;helloworld.xml">Example File</ulink></para>

      if(/<para><ulink url=\"&url_examples_base;(.*)\">(.*)<\/ulink><\/para>/)
      {
        #List all the source files in that directory:
        my $source_file = $examples_base . $1;

        # print "<para>File: <filename>${source_file}</filename>\n";
        # print "</para>\n";
        print "<programlisting>\n";

        &process_source_file("${source_file}");

        print "</programlisting>\n";

        print "<!-- end inserted example code -->\n";
      }
    }

    close(FILE);
  }

  exit 0;
}

sub process_source_file($)
{
  my ($source_file) = @_;
  my $found_start = 0;

  open(SOURCE_FILE, $source_file);

  while(<SOURCE_FILE>)
  {
    s/&/&amp;/g;
    s/</&lt;/g;
    s/>/&gt;/g;
    s/"/&quot;/g;

    print $_;
  }

  close(SOURCE_FILE);
}

