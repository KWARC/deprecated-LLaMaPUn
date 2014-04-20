use strict;
use warnings;

use Test::More tests => 3;

use XML::LibXML;
use LLaMaPUn::LaTeXML;
use File::Slurp;

my $parser=XML::LibXML->new(no_blanks=>1);
$parser->load_ext_dtd(0);
my $tokenized_document = "t/documents/sample_tokenized.xml";
my $tei_document = "t/documents/sample_TEI_xhtml.xhtml";
my $tokenized_dom = $parser->parse_file($tokenized_document);
my $tei_xhtml_dom = xml_to_TEI_xhtml($tokenized_dom);

my $expected_tei_content = read_file($tei_document);
is($tei_xhtml_dom->toString(1),$expected_tei_content , "Simple TEI XHTML conversion");

# Test 2, PlanetMath entry 216
my $planetmath_document = "t/documents/entry216_tokenized.xml";
my $planetmath_tei = "t/documents/entry216.xhtml";
my $planetmath_dom = $parser->parse_file($planetmath_document);
my $planetmath_tei_dom = xml_to_TEI_xhtml($planetmath_dom);

my $planetmath_expected_content = read_file($planetmath_tei);
is($planetmath_tei_dom->toString(1),$planetmath_expected_content, "PlanetMath TEI XHTML conversion");

# Test 3, arXiv entry 1311.6767
my $arxiv_document = "t/documents/1311.6767_tokenized.xml";
my $arxiv_tei = "t/documents/1311.6767.xhtml";
my $arxiv_dom = $parser->parse_file($arxiv_document);
my $arxiv_tei_dom = xml_to_TEI_xhtml($arxiv_dom);

my $arxiv_expected_content = read_file($arxiv_tei);
is($arxiv_tei_dom->toString(1),$arxiv_expected_content, "arXiv TEI XHTML conversion");

# open O, ">", "/tmp/test.xhtml";
# print O $arxiv_tei_dom->toString(1);
# close O;