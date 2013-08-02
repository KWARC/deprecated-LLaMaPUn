use strict;
use warnings;

use Test::More tests => 4;

use XML::LibXML;
use LLaMaPUn::Preprocessor;
use LLaMaPUn::Preprocessor::Purify;

# Generated via:
# latexmlc --nopost --parse=no $tex_fragment
my $tex_fragment = "t/documents/sample_document.tex";

my $parser=XML::LibXML->new(no_blanks=>1);
my $test_fragment = "t/documents/sample_document.xml";
my $test_dom = $parser->parse_file($test_fragment);

# Purification tests:
my $purified_dom = LLaMaPUn::Preprocessor::Purify::purify_noparse($test_dom,verbose=>0);
my $saved_dom = $purified_dom->cloneNode(1);

my $preprocessor=LLaMaPUn::Preprocessor->new(replacemath=>"position");
$preprocessor->setDocument($purified_dom);

my $abstract = $preprocessor->getNormalizedElement('abstract');
#my $titles = $preprocessor->getNormalizedElements('title');
my $body = $preprocessor->getNormalizedBody;

is($$abstract,"A test for LLaMaPUn::Preprocessor");
#is_deeply($titles,["Sample sentence","Sample structure"]); # TODO: Figure out how to deal with <tag>s
is($$body,"This is a sample sentence with ASCII MathExpr-doc0-purify0-m1 math. And a MathExpr-doc0-S1-p1-m1 square root. And then an example of text in math MathExpr-doc0-S1-p1-m2.");

# Test the math lookup table
my $second_math = $preprocessor->getMathEntry('MathExpr-doc0-S1-p1-m2');
# AND test if the purifier found "jaguar" and "mouse" and united them
is($second_math,'<Math mode="inline" xml:id="S1.p1.m2" tex="P(jaguar\mid mouse)=0"><XMath><XMTok role="UNKNOWN" font="italic">P</XMTok><XMTok role="OPEN">(</XMTok><XMTok role="UNKNOWN" meaning="UNKNOWN" font="italic">jaguar</XMTok><XMTok name="mid" role="VERTBAR">∣</XMTok><XMTok role="UNKNOWN" meaning="UNKNOWN" font="italic">mouse</XMTok><XMTok role="CLOSE">)</XMTok><XMTok meaning="equals" role="RELOP">=</XMTok><XMTok meaning="0" role="NUMBER">0</XMTok></XMath></Math>');

#print STDERR "Tokenized Dom: \n",$purified_dom->toString(1),"\n\n";
# Test the in-place tokenization
use LLaMaPUn::Preprocessor::MarkTokens;
my $marktokens = LLaMaPUn::Preprocessor::MarkTokens->new(document=>$saved_dom);
my $tokenized_dom = $marktokens->process_document;

my $expected_tokenized = $parser->parse_file("t/documents/sample_tokenized.xml");
is($tokenized_dom->toString(1),$expected_tokenized->toString(1));