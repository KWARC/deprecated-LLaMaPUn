#!/usr/bin/perl
use strict;
use warnings;

use lib 'blib/lib';

use XML::LibXML;
use LLaMaPUn::Preprocessor;
use LLaMaPUn::Preprocessor::Purify;
use LLaMaPUn::Preprocessor::MarkTokens;
use LLaMaPUn::LaTeXML;
use Scalar::Util qw/blessed/;

# 02 Preprocess
system('cpanm . -n'); # Install so that we can see the resource directory.
my $tex_fragment = "t/documents/sample_document.tex";
my $test_fragment = "t/documents/sample_document.xml";
# Generate sample_document.xml
system("latexmlc --preload=[ids]latexml.sty --nopost --parse=no --nocomments --nodefaultresources --timestamp=0 --destination=$test_fragment $tex_fragment ");
# Generate entry216.xml
my $planetmath_source = "t/documents/entry216.tex";
my $planetmath_doc = "t/documents/entry216.xml";
system("latexmlc --preload=[ids]latexml.sty --nopost --parse=no --nocomments --nodefaultresources --timestamp=0 --destination=$planetmath_doc $planetmath_source");

my $parser=XML::LibXML->new(no_blanks=>1);
$parser->load_ext_dtd(0);
my $test_dom = $parser->parse_file($test_fragment);
my $planetmath_dom = $parser->parse_file($planetmath_doc);

# Purification tests:
my $test_purified_dom = LLaMaPUn::Preprocessor::Purify::purify_noparse($test_dom,verbose=>0);
my $planetmath_purified_dom = LLaMaPUn::Preprocessor::Purify::purify_noparse($planetmath_dom,verbose=>0);

# MarkTokens, Test I
my $marktokens = LLaMaPUn::Preprocessor::MarkTokens->new(document=>$test_purified_dom);
my $test_tokenized_dom = $marktokens->process_document;
open my $fh, ">", "t/documents/sample_tokenized.xml";
print $fh $test_tokenized_dom->toString();
close $fh;
print STDERR "Wrote t/documents/sample_tokenized.xml\n";
# MarkTokens, Test II
$marktokens = LLaMaPUn::Preprocessor::MarkTokens->new(document=>$planetmath_purified_dom);
my $planetmath_tokenized_dom = $marktokens->process_document;
open $fh, ">", "t/documents/entry216_tokenized.xml";
print $fh $planetmath_tokenized_dom->toString();
close $fh;
print STDERR "Wrote t/documents/entry216_tokenized.xml\n";

# 03 LaTeXML
my $tei_xhtml_dom = xml_to_TEI_xhtml($test_tokenized_dom);

open $fh, ">", "t/documents/sample_TEI_xhtml.xhtml";
print $fh $tei_xhtml_dom->toString();
close $fh;
print STDERR "Wrote t/documents/sample_TEI_xhtml.xhtml\n";

# Test 2, PlanetMath entry 216
my $planetmath_tei_dom = xml_to_TEI_xhtml($planetmath_tokenized_dom);
open $fh, ">", "t/documents/entry216.xhtml";
print $fh $planetmath_tei_dom->toString();
close $fh;
print STDERR "Wrote t/documents/entry216.xhtml\n";