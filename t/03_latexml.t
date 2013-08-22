use strict;
use warnings;

use Test::More tests => 2;

use XML::LibXML;
use LLaMaPUn::LaTeXML;
use File::Slurp;

my $parser=XML::LibXML->new(keep_blanks=>1);
$parser->load_ext_dtd(0);
$parser->keep_blanks(1);
my $tokenized_document = "t/documents/sample_tokenized.xml";
my $tei_document = "t/documents/sample_TEI_xhtml.xhtml";
my $tokenized_dom = $parser->parse_file($tokenized_document);
my $tei_xhtml_dom = xml_to_TEI_xhtml($tokenized_dom);

my $expected_tei_content = read_file($tei_document);
is($tei_xhtml_dom->toString(1),$expected_tei_content);

# Test 2, PlanetMath entry 216
my $planetmath_document = "t/documents/entry216_tokenized.xml";
my $planetmath_tei = "t/documents/entry216.xhtml";
my $planetmath_dom = $parser->parse_file($planetmath_document);
my $planetmath_tei_dom = xml_to_TEI_xhtml($planetmath_dom);

my $planetmath_expected_content = read_file($planetmath_tei);
is($planetmath_tei_dom->toString(1),$planetmath_expected_content);

# open O, ">", "/tmp/test.xhtml";
# print O $planetmath_tei_dom->toString(1);
# close O;