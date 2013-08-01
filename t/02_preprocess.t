use strict;
use warnings;

use Test::More tests => 4;

use XML::LibXML;
use LLaMaPUn::Preprocessor;
use LLaMaPUn::Preprocessor::Purify;

# Generated via:
# latexmlc --nopost --parse=no $tex_fragment
my $tex_fragment = <<'EOL';
literal:\documentclass{article}
\begin{document}
\begin{abstract}A test for LLaMaPUn::Preprocessor\end{abstract}
\section{Sample sentence}
This is a sample sentence with ASCII 1+2=3 math. And a $\sqrt{x}$ square root.
And then an example of text in math $P(jaguar\mid mouse)=0$.
\section{Sample structure}
\begin{tabular}{ll}
Apples & Oranges \\
15 & 10 \\
\end{tabular}
\end{document}
EOL

my $test_fragment = <<'EOL';
<?xml version="1.0" encoding="UTF-8"?>
<?latexml searchpaths="/tmp,/home/dreamweaver/git/LLaMaPUn"?>
<?latexml class="article"?>
<?latexml RelaxNGSchema="LaTeXML"?>
<document xmlns="http://dlmf.nist.gov/LaTeXML">
  <resource src="LaTeXML.css" type="text/css"/>
  <resource src="ltx-article.css" type="text/css"/>
  <abstract name="Abstract">
    <p>A test for LLaMaPUn::Preprocessor</p>
  </abstract>
  <section refnum="1" xml:id="S1">
    <title><tag close=" ">1</tag>Sample sentence</title>
    <para xml:id="S1.p1">
      <p>This is a sample sentence with ASCII 1+2=3 math. And a <Math mode="inline" xml:id="S1.p1.m1" tex="\sqrt{x}"><XMath><XMApp><XMTok meaning="square-root"/><XMArg><XMTok role="UNKNOWN" font="italic">x</XMTok></XMArg></XMApp></XMath></Math> square root.
And then an example of text in math <Math mode="inline" xml:id="S1.p1.m2" tex="P(jaguar\mid mouse)=0"><XMath><XMTok role="UNKNOWN" font="italic">P</XMTok><XMTok role="OPEN">(</XMTok><XMTok role="UNKNOWN" font="italic">j</XMTok><XMTok role="UNKNOWN" font="italic">a</XMTok><XMTok role="UNKNOWN" font="italic">g</XMTok><XMTok role="UNKNOWN" font="italic">u</XMTok><XMTok role="UNKNOWN" font="italic">a</XMTok><XMTok role="UNKNOWN" font="italic">r</XMTok><XMTok name="mid" role="VERTBAR">â£</XMTok><XMTok role="UNKNOWN" font="italic">m</XMTok><XMTok role="UNKNOWN" font="italic">o</XMTok><XMTok role="UNKNOWN" font="italic">u</XMTok><XMTok role="UNKNOWN" font="italic">s</XMTok><XMTok role="UNKNOWN" font="italic">e</XMTok><XMTok role="CLOSE">)</XMTok><XMTok meaning="equals" role="RELOP">=</XMTok><XMTok meaning="0" role="NUMBER">0</XMTok></XMath></Math>.</p>
    </para>
  </section>
  <section refnum="2" xml:id="S2">
    <title><tag close=" ">2</tag>Sample structure</title>
    <para xml:id="S2.p1">
      <tabular>
        <thead>
          <tr>
            <td align="left" thead="true">Apples</td>
            <td align="left" thead="true">Oranges</td>
          </tr>
        </thead>
        <tbody>
          <tr>
            <td align="left">15</td>
            <td align="left">10</td>
          </tr>
        </tbody>
      </tabular>
    </para>
  </section>
</document>
EOL

my $parser=XML::LibXML->new(no_blanks=>1);
my $test_dom = $parser->parse_string($test_fragment);

# Purification tests:
my $purified_dom = LLaMaPUn::Preprocessor::Purify::purify_noparse($test_dom,verbose=>0);

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
my $marktokens = LLaMaPUn::Preprocessor::MarkTokens->new(document=>$purified_dom);
my $tokenized_dom = $marktokens->process_document;

ok($tokenized_dom);