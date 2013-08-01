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

my $expected_tokenized = <<'EOL';
<?xml version="1.0" encoding="UTF-8"?>
<?latexml searchpaths="/tmp,/home/dreamweaver/git/LLaMaPUn"?>
<?latexml class="article"?>
<?latexml RelaxNGSchema="LaTeXML"?>
<document xmlns="http://dlmf.nist.gov/LaTeXML" xml:id="document.1">
  <resource src="LaTeXML.css" type="text/css" xml:id="resource.1"/>
  <resource src="ltx-article.css" type="text/css" xml:id="resource.2"/>
  <abstract name="Abstract" xml:id="abstract.1">
    <sentence xml:id="sentence.1">
      <word xml:id="word.1">
        <token xml:id="token.1">A</token>
      </word>
      <word xml:id="word.2">
        <token xml:id="token.2">test</token>
      </word>
      <word xml:id="word.3">
        <token xml:id="token.3">for</token>
      </word>
      <word xml:id="word.4">
        <token xml:id="token.4">LLaMaPUn</token>
      </word>
      <word xml:id="word.5">
        <token xml:id="token.5">:</token>
      </word>
      <word xml:id="word.6">
        <token xml:id="token.6">:</token>
      </word>
      <word xml:id="word.7">
        <token xml:id="token.7">Preprocessor</token>
      </word>
    </sentence>
  </abstract>
  <section refnum="1" xml:id="section.1">
    <title xml:id="title.1">
      <sentence xml:id="sentence.2">
        <word xml:id="word.8">
          <token xml:id="token.8">Sample</token>
        </word>
        <word xml:id="word.9">
          <token xml:id="token.9">sentence</token>
        </word>
      </sentence>
      <tag close=" " xml:id="tag.1">1</tag>
    </title>
    <para xml:id="para.1">
      <sentence xml:id="sentence.3">
        <word xml:id="word.10">
          <token xml:id="token.10">This</token>
        </word>
        <word xml:id="word.11">
          <token xml:id="token.11">is</token>
        </word>
        <word xml:id="word.12">
          <token xml:id="token.12">a</token>
        </word>
        <word xml:id="word.13">
          <token xml:id="token.13">sample</token>
        </word>
        <word xml:id="word.14">
          <token xml:id="token.14">sentence</token>
        </word>
        <word xml:id="word.15">
          <token xml:id="token.15">with</token>
        </word>
        <word xml:id="word.16">
          <token xml:id="token.16">ASCII</token>
        </word>
        <formula xml:id="formula.1">
          <Math xmlns="http://dlmf.nist.gov/LaTeXML" xml:id="Math.1">
            <XMath xml:id="XMath.1">
              <token meaning="1" role="NUMBER" xml:id="token.17">1</token>
              <token meaning="plus" role="ADDOP" xml:id="token.18">+</token>
              <token meaning="2" role="NUMBER" xml:id="token.19">2</token>
              <token meaning="equals" role="RELOP" xml:id="token.20">=</token>
              <token meaning="3" role="NUMBER" xml:id="token.21">3</token>
            </XMath>
          </Math>
        </formula>
        <word xml:id="word.17">
          <token xml:id="token.22">math</token>
        </word>
        <word xml:id="word.18">
          <token xml:id="token.23">.</token>
        </word>
      </sentence>
      <sentence xml:id="sentence.4">
        <word xml:id="word.19">
          <token xml:id="token.24">And</token>
        </word>
        <word xml:id="word.20">
          <token xml:id="token.25">a</token>
        </word>
        <formula xml:id="formula.2">
          <Math xmlns="http://dlmf.nist.gov/LaTeXML" mode="inline" xml:id="Math.2" tex="\sqrt{x}">
            <XMath xml:id="XMath.2">
              <XMApp xml:id="XMApp.1">
                <token meaning="square-root" xml:id="token.26"/>
                <XMArg xml:id="XMArg.1">
                  <token role="UNKNOWN" font="italic" xml:id="token.27">x</token>
                </XMArg>
              </XMApp>
            </XMath>
          </Math>
        </formula>
        <word xml:id="word.21">
          <token xml:id="token.28">square</token>
        </word>
        <word xml:id="word.22">
          <token xml:id="token.29">root</token>
        </word>
        <word xml:id="word.23">
          <token xml:id="token.30">.</token>
        </word>
      </sentence>
      <sentence xml:id="sentence.5">
        <word xml:id="word.24">
          <token xml:id="token.31">And</token>
        </word>
        <word xml:id="word.25">
          <token xml:id="token.32">then</token>
        </word>
        <word xml:id="word.26">
          <token xml:id="token.33">an</token>
        </word>
        <word xml:id="word.27">
          <token xml:id="token.34">example</token>
        </word>
        <word xml:id="word.28">
          <token xml:id="token.35">of</token>
        </word>
        <word xml:id="word.29">
          <token xml:id="token.36">text</token>
        </word>
        <word xml:id="word.30">
          <token xml:id="token.37">in</token>
        </word>
        <word xml:id="word.31">
          <token xml:id="token.38">math</token>
        </word>
        <formula xml:id="formula.3">
          <Math xmlns="http://dlmf.nist.gov/LaTeXML" mode="inline" xml:id="Math.3" tex="P(jaguar\mid mouse)=0">
            <XMath xml:id="XMath.3">
              <token role="UNKNOWN" font="italic" xml:id="token.39">P</token>
              <token role="OPEN" xml:id="token.40">(</token>
              <XMTok role="UNKNOWN" meaning="UNKNOWN" font="italic" xml:id="XMTok.1">jaguar</XMTok>
              <token name="mid" role="VERTBAR" xml:id="token.41">â£</token>
              <XMTok role="UNKNOWN" meaning="UNKNOWN" font="italic" xml:id="XMTok.2">mouse</XMTok>
              <token role="CLOSE" xml:id="token.42">)</token>
              <token meaning="equals" role="RELOP" xml:id="token.43">=</token>
              <token meaning="0" role="NUMBER" xml:id="token.44">0</token>
            </XMath>
          </Math>
        </formula>
        <word xml:id="word.32">
          <token xml:id="token.45">.</token>
        </word>
      </sentence>
    </para>
  </section>
  <section refnum="2" xml:id="section.2">
    <title xml:id="title.2">
      <sentence xml:id="sentence.6">
        <word xml:id="word.33">
          <token xml:id="token.46">Sample</token>
        </word>
        <word xml:id="word.34">
          <token xml:id="token.47">structure</token>
        </word>
      </sentence>
      <tag close=" " xml:id="tag.2">2</tag>
    </title>
    <para xml:id="para.2">
      <tabular xml:id="tabular.1">
        <thead xml:id="thead.1">
          <tr xml:id="tr.1">
            <td align="left" thead="true" xml:id="td.1">
              <sentence xml:id="sentence.7">
                <word xml:id="word.35">
                  <token xml:id="token.48">Apples</token>
                </word>
              </sentence>
            </td>
            <td align="left" thead="true" xml:id="td.2">
              <sentence xml:id="sentence.8">
                <word xml:id="word.36">
                  <token xml:id="token.49">Oranges</token>
                </word>
              </sentence>
            </td>
          </tr>
        </thead>
        <tbody xml:id="tbody.1">
          <tr xml:id="tr.2">
            <td align="left" xml:id="td.3">
              <sentence xml:id="sentence.9">
                <word xml:id="word.37">
                  <token xml:id="token.50">15</token>
                </word>
              </sentence>
            </td>
            <td align="left" xml:id="td.4">
              <sentence xml:id="sentence.10">
                <word xml:id="word.38">
                  <token xml:id="token.51">10</token>
                </word>
              </sentence>
            </td>
          </tr>
        </tbody>
      </tabular>
    </para>
  </section>
</document>
EOL
ok($tokenized_dom);
print STDERR $tokenized_dom->toString(1);
#is($tokenized_dom->toString(1),$expected_tokenized);