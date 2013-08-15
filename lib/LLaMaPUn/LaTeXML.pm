# /=====================================================================\ #
# |  LLaMaPUn                                                            | #
# | LaTeXML convenience Module                                          | #
# |=====================================================================| #
# | Part of the LLaMaPUn project: http://kwarc.info/projects/LLaMaPUn/    | #
# |  Research software, produced as part of work done by the            | #
# |  KWARC group at Jacobs University,                                  | #
# | Copyright (c) 2009 LLaMaPUn group                                    | #
# | Released under the GNU Public License                               | #
# |---------------------------------------------------------------------| #
# | Deyan Ginev <d.ginev@jacobs-university.de>                  #_#     | #
# | http://kwarc.info/people/dginev                            (o o)    | #
# \=========================================================ooo==U==ooo=/ #

package LLaMaPUn::LaTeXML;
#use warnings;
use strict;
use FindBin;
use Getopt::Long qw(:config no_ignore_case);
use LaTeXML;
use LaTeXML::Util::Pathname;
# latexmlpost
use File::Spec;
use LaTeXML::Post;
use LaTeXML::Util::ObjectDB;

use XML::LibXML;
use Encode;
use Carp;
use Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw(&xmath_to_pmml &tex_to_pmml &tex_to_noparse &tex_to_xmath &parse_math_structure 
  &xml_to_xhtml &xml_to_TEI_xhtml);

our ($INSTALLDIR) = grep(-d $_, map("$_/LLaMaPUn", @INC));
use vars qw($LaTeXML_nsURI);
$LaTeXML_nsURI = "http://dlmf.nist.gov/LaTeXML";
our $nsURI = $LaTeXML_nsURI;

#Important! : This is an experimental sub that borrows (and hacks through) latexmlmath
#             in order to process an array of TeX math formulas to PMML
# TODO: Generalize preloading, and beware of impossible to convert elements

sub tex_doc_constructor {
  my $texbody="";
  foreach my $tex(@_) {
    # We need to determine whether the TeX we're given needs to be wrapped in \[...\]
    # Does it have $'s around it? Does it have a display math environment?
    # The most elegant way would be to notice as soon as we start adding to the doc
    # and switch to math mode if necessary, but that's tricky.
    # Let's just try a manual hack, looking for known switches...
    $tex =~ s/^\s+//;
    $tex =~ s/\s+$//;

    our $MATHENVS = 'math|displaymath|equation*?|eqnarray*?'
      .'|multline*?|align*?|falign*?|alignat*?|xalignat*?|xxalignat*?|gather*?';
    if(($tex =~ /^\$/) && ($tex =~ /\$$/)){} # Wrapped in $'s
    elsif(($tex =~ /^\\\(/) && ($tex =~ /\\\)$/)){} # Wrapped in \(...\)
    elsif(($tex =~ /^\\\[/) && ($tex =~ /\\\]$/)){} # Wrapped in \[...\]
    elsif(($tex =~ /^\\begin\{($MATHENVS)\}/) && ($tex =~ /\\end\{$1\}$/)){}
    else {
      $tex = '\[ '.$tex.' \]'; }
    $texbody.=$tex.' \\\\ '."\n";
  }
my $texdoc = <<EODoc;
\\documentclass{article}
\\begin{document}
\\newcounter{equation}
\\newcounter{Unequation}
$texbody
\\end{document}
EODoc
  return $texdoc;
}

sub latexml_doc_constructor {
 my $filename="__latexml_doc_constructor.xml";
 open (STUB,">$filename");
my $frontmatter = <<EODoc;
<?xml version="1.0" encoding="UTF-8"?>
<?latexml searchpaths="."?>
<?latexml class="article"?>
<?latexml RelaxNGSchema="LaTeXML"?>
<document xmlns="http://dlmf.nist.gov/LaTeXML">
  <title>XMath to PMML conversion</title>
  <section refnum="1" xml:id="S1">
    <title>Body</title>
    <para xml:id="S1.p1">
EODoc

my $backmatter = <<EODoc;
</para>
</section>
</document>
EODoc

print STUB $frontmatter;

 foreach my $math(@_) {
   #Make sure we close our eyes for equations and equationgroups:
   # DG: Why? Shouldn't we be able to digest anything?
   #if ($math!~/<equation(.+)>/) {
   print STUB "<p>".encode('UTF-8',$math)."</p>\n\n";
   #}
   #else {
   #   print STUB "<p><Math><XMath><XMTok name='equation' role='UNKNOWN'/></XMath></Math></p>\n\n";
   #}
 }
 print STUB $backmatter;
 close(STUB);

 my $model = LaTeXML::Model->new(); 
 $model->registerNamespace('ltx',$nsURI);
 my $document = LaTeXML::Document->new_from_file($model,$filename);
 unlink($filename);
 $document;
}

sub tex_to_pmml {
  conversion_driver("pmml",@_); }
sub tex_to_noparse {
  conversion_driver("noparse",@_); }
sub tex_to_xmath {
  conversion_driver("xmath",@_); }

sub xmath_to_pmml {
  post_driver(whatsin=>"xmath",whatsout=>"pmml",sources=>[@_]); }
sub xml_to_xhtml {
  post_driver(whatsin=>"xml",whatsout=>'xhtml',sources=>[@_]); }
sub xml_to_TEI_xhtml {
  post_driver(whatsin=>"xml",whatsout=>'TEI-xhtml',sources=>[@_]); }

sub conversion_driver {
  my ($type,@args) = @_;

  # Construct artificial TeX document
  my $texdoc = tex_doc_constructor(@args);
  
  #======================================================================
  # Digest the TeX
  #======================================================================

  # Hacky preloading in hope we will cover anything that comes from the corpus
  # TODO: If this module goes mainstream, introduce sufficient preloading generality from input here
  my  @preload = ("amsart.cls", "amsmath.sty", "amsthm.sty", "amstext.sty", "amssymb.sty", "eucal.sty");
  my @paths=(".");
  my $verbosity=-5;
  my $linelength;
  
  @paths = map(pathname_canonical($_),@paths);
  my $mathparse = ($type eq "noparse") ? 'no' : 'RecDescent';
  my $latexml= LaTeXML->new(preload=>['LaTeX.pool',@preload], searchpaths=>[@paths],
			    verbosity=>$verbosity, strict=>0,
			    includeComments=>0,includeStyles=>undef,
			    documentid=>undef,
          mathparser=>LaTeXML::MathParser->new(),
			    mathparse=>$mathparse);
  
  # Digest and convert to LaTeXML's XML
  my $doc = LaTeXML::Post::Document->new($latexml->convertFile("literal:$texdoc"),
					 nocache=>1);
  if($type=~/^(xmath|noparse)$/){
    # extract the xmath and return node
    my $latexmlpost = LaTeXML::Post->new(verbosity=>$verbosity||0);
    my ($post) = $latexmlpost->ProcessChain($doc);
    return $post->findnodes('//ltx:XMath');
  }

  #Defaulted (TODO: make more general) to pmml if this block is reached

  #======================================================================
  # Postprocess to convert the math to whatever desired forms.
  #======================================================================
  # Since we can't easily find & extract all the various formats at once,
  # let's just process each one separately.
  
  our %OPTIONS = (verbosity=>$verbosity||0);

  require 'LaTeXML/Post/MathML.pm';
  my $latexmlpost = LaTeXML::Post->new(verbosity=>$verbosity||0);
  my($post) = $latexmlpost->ProcessChain($doc,
      LaTeXML::Post::MathML::Presentation->new(
          (defined $linelength ? (linelength=>$linelength):()),
	   %OPTIONS));
  
 return $post->findnodes('//m:math');
  
#**********************************************************************

}

sub post_driver {
  my (%options) = @_;
  my @sources = grep {defined} @{$options{sources}||[]};
  return unless @sources;
  my $whatsin = $options{whatsin};
  my $ltxmldoc;
  my %PostOPS = (validate=>0, noresources=>1, parameters=>{}, verbosity=>-1,
    sourceDirectory=>'.',siteDirectory=>".",nocache=>1,destination=>'.');
  if ($whatsin eq 'xmath') {
    # Math mode:
    # Construct artificial LaTeXML document
    $ltxmldoc = latexml_doc_constructor(@sources);
    $ltxmldoc = $ltxmldoc->finalize();
    my $doc = LaTeXML::Post::Document->new($ltxmldoc,
    				 nocache=>1);
    #Defaulted (TODO: make more general) to pmml if this block is reached
    #======================================================================
    # Postprocess to convert the math to whatever desired forms.
    #======================================================================
    # Since we can't easily find & extract all the various formats at once,
    # let's just process each one separately.

    require LaTeXML::Post::MathML;
    my $latexmlpost = LaTeXML::Post->new(%PostOPS);
    my($post) = $latexmlpost->ProcessChain($doc,
        LaTeXML::Post::MathML::Presentation->new(%PostOPS));
    
   return $post->findnodes('//m:math'); }
  elsif ($whatsin eq 'xml') {
    my $whatsout = $options{whatsout};
    my $stylesheet = "$INSTALLDIR/resources/LaTeXML/LaTeXML-$whatsout.xsl";
    # TODO: This should really all be supported in LaTeXML::Converter
    # ... sigh ... punting and waiting for more time to incorporate it!
    $ltxmldoc = shift @sources;
    my $doc = LaTeXML::Post::Document->new($ltxmldoc,
        nocache=>1,%PostOPS);
    my $latexmlpost = LaTeXML::Post->new(%PostOPS);
    require LaTeXML::Post::MathML;
    require LaTeXML::Post::XMath;
    require LaTeXML::Post::XSLT;
    # TODO: Just hardwire XHTML in there, punting anything more meaningful for later

    my($post) = $latexmlpost->ProcessChain($doc,
        LaTeXML::Post::MathML::Presentation->new(%PostOPS),
        LaTeXML::Post::XMath->new(%PostOPS),
        LaTeXML::Post::XSLT->new(stylesheet=>$stylesheet,%PostOPS)
        );
    return $post->getDocument;
  }  
#**********************************************************************
}


#Formula-tree plugin for LaTeXML Math Parser
sub parse_math_structure {
  my ($source,$destination) = @_;
  my($verbosity,$strict,$comments,$noparse,$includestyles)=(1,0,1,0,0);
  my ($format,$help,$showversion)=('xml');
  my ($documentid);
  my $inputencoding;
  my $mode = 'auto';
  my @paths = ('.');
  my (@preload,@debugs);
  if (! $destination) {
    $destination = $source;
    $destination =~ s/\.(.+)$/.tex.xml/;
  }
  my $latexml= LaTeXML->new(preload=>[@preload], searchpaths=>[@paths],
			    verbosity=>$verbosity, strict=>$strict,
			    includeComments=>$comments,inputencoding=>$inputencoding,
			    includeStyles=>$includestyles,
          mathparser=>LaTeXML::MathParser->new(),
			    documentid=>$documentid,
			    mathparse=>'Marpa');
  $LaTeXML::Global::STATE = $$latexml{state};
  $LaTeXML::MathParser::DEBUG = 0;
  my $dom = $latexml->parseMath_Document($source); 
  my $serialized = $dom->toString(1);
  print STDERR $latexml->getStatusMessage,"\n";
  open(PARSEDOUT, ">",$destination);
  print PARSEDOUT $serialized if $serialized;
  close PARSEDOUT;
}
package LaTeXML;
sub parseMath_Document {
  my($self,$filename)=@_;
  $self->withState(sub {
     my($state)=@_;
     my $model    = $state->getModel;   # The document model.
     my $document  = LaTeXML::Document->new_from_file($model,$filename);
     $model->loadSchema(); # If needed?
     print STDERR "Document : $document\n\n";
     local $LaTeXML::DOCUMENT = $document;
     $state->getMathParser->parseMath($document,parser=>'Marpa');
     $document->finalize(); });
}
1;

package LaTeXML::Document;
use XML::LibXML;
sub new_from_file {
  my($class,$model,$filename)=@_;
  Fatal("File $filename can't be accessed!\n") unless (-e $filename);
  my $doc = XML::LibXML->load_xml(location=>$filename);
  $model = LaTeXML::Model->new(); 
  $model->registerNamespace('ltx',$nsURI);
  # parse the document
  bless { document=>$doc, node=>$doc, model=>$model,
          idstore=>{}, labelstore=>{},
	  node_fonts=>{}, node_boxes=>{}, node_properties=>{}, 
	  pendixng=>[], progress=>0}, $class; }
1;

package LLaMaPUn::LaTeXML;
sub tex_normalize {
  my @results;
  foreach my $tex(@_) {
    my $latexml= LaTeXML->new(preload=>[], searchpaths=>["."],
                              verbosity=>0, strict=>0,
                              includeComments=>0,includeStyles=>undef,
                              documentid=>undef);
  
    # Digest, then UnTeX
    my $digested =  $latexml->digestFile("literal:$tex");
    my $serialized;
    $serialized = LaTeXML::Global::UnTeX($digested) if $digested;
    push @results, $serialized;
  }
  @results;
}

1;

__END__

=pod 

=head1 NAME

C<LLaMaPUn::LaTeXML> - Convenience functionality building on top of the LaTeXML libraries

=head1 SYNOPSIS

    use LLaMaPUn::LaTeXML;
    @pmml = tex_to_pmml(@texfragments);
    @xmath = tex_to_xmath(@texfragments);
    @noparse = tex_to_noparse(@texfragments);
    @tex = tex_normalize(@texfragments);
    parse_math_structure($filename);
    
=head1 DESCRIPTION

 This module utilizes the functionality provided by the LaTeXML libraries 
 to provide native speed up and customizations for a range of convenience
 functions.

=head2 METHODS

=over 4

=item C<< @pmml = tex_to_pmml(@texfragments); >>

Given an array of tex formulas, returns the corresponding array of their Presentational MathML equivalents. 
Note: Returns XML::LibXML objects. To get the string representation use:

@string_pmml = map ($_->toString, @pmml);

=item C<< @xmath = tex_to_xmath(@texfragments); >>

Given an array of tex formulas, returns the corresponding array of their XMath equivalents. 
Note: Returns XML::LibXML objects. To get the string representation use:

@string_xmath = map ($_->toString, @xmath);

=item C<< @noparse = tex_to_noparse(@texfragments); >>

Given an array of tex formulas, returns the corresponding array of their unparsed XMath equivalents. 
Note: Returns XML::LibXML objects. To get the string representation use:

@string_noparse = map ($_->toString, @noparse);

=item C<< @tex = tex_normalize(@texfragments); >>

Given an array of tex fragments, returns a normalized (i.e. expanded and non-redundant) TeX representation.
Useful for comparing two similar TeX expressions that might be the same, e.g. $1+1$ and $1 + 1$.

=item C<< parse_math_structure($filename); >>

Assumes an XML document created with "latexml --noparse" as input. Creates a .tex.xml equivalent by plugging the document
back into the native LaTeXML math parser.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
