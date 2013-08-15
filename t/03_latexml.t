use strict;
use warnings;

use Test::More tests => 1;

use XML::LibXML;
use LLaMaPUn::LaTeXML;

my $parser=XML::LibXML->new();
$parser->load_ext_dtd(0);
my $tokenized_document = "t/documents/sample_tokenized.xml";
my $tei_document = "t/documents/sample_TEI_xhtml.xhtml";
my $tokenized_dom = $parser->parse_file($tokenized_document);
my $expected_tei = $parser->parse_file($tei_document);

my $tei_xhtml_dom = xml_to_TEI_xhtml($tokenized_dom);
is($tei_xhtml_dom->toString(1),$expected_tei->toString(1));