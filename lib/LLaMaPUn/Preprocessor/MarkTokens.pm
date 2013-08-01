# /=====================================================================\ #
# |  LLaMaPUn                                                            | #
# | Mark Tokens in LaTeXML XML documents                                | #
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

package LLaMaPUn::Preprocessor::MarkTokens;
use strict;
use warnings;
use Carp;
use Encode;
use Scalar::Util qw(blessed);
use LLaMaPUn::Util;
use LLaMaPUn::Preprocessor;
use LLaMaPUn::Tokenizer;

use vars qw($VERSION);
$VERSION = "0.0.1";
#**********************************************************************

sub new {
  my($class,%options)=@_;
  my ($doc,$name,$path)=(undef,undef,undef);
  #Initialize a preprocessor:
  my $preprocessor = LLaMaPUn::Preprocessor->new(replacemath=>"position");
  #Initialize a tokenizer:
  my $tokenizer=LLaMaPUn::Tokenizer->new(input=>"text",token=>"sentence",mergeMath=>0, normalize=>1);
  

  if ($options{document}) {
    $doc=$options{document};
    if (blessed($options{document}) ne 'XML::LibXML::Document') {
      $doc=parse_file($options{document});
      ($path,$name)=splitFilepath($options{document}); }
  }
  bless {DOCUMENT=>$doc,
   NAME=>$name,
   PATH=>$path,
   TOKENIZER=>$tokenizer,
   PREPROCESSOR=>$preprocessor,
   RDFEXTRACTED=>0,
   TOKENSMARKED=>0,
   PURE=>0,
   W=>LLaMaPUn::Tokenizer::Word->w,
   RAW=>$options{raw}}, $class; }

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

sub setDocument {
  my ($self,$source)=@_;
  $self->{DOCUMENT}=$source;
  if (blessed($source) ne "XML::LibXML::Document") {
    $self->{DOCUMENT}=parse_file($source);
    my ($path,$name)=splitFilepath($source);
    $self->{NAME}=$name;
    $self->{PATH}=$path;
  }
  $self->{TOKENSMARKED}=0;
}

# Recursive extracting of text content - avoids weird spacing problems
sub __recTextContent {
  my ($node) = @_;
  return '' unless defined $node;
  return " " if ($node->nodeName =~ /#comment/);
  if ($node->childNodes) {
    my $tc = "";
    foreach my $child($node->childNodes) {
      my $chpart = join(" ",split("\n",__recTextContent($child)))." ";
      $tc .= $chpart;
    }
    $tc;
  }
  else {
    my ($part) = @{LLaMaPUn::Tokenizer::Sentence->normalize([$node->textContent])};
    $part=~s/^\s//; #Detected problem with native ->textContent of LibXML -  apparently an extra space is deposited at the beginning.
    $part;
  }
}

sub mark_tokens {
  my ($self)=@_;
  return if $self->{TOKENSMARKED};
  my $root = $self->{DOCUMENT}->getDocumentElement;
  my $original = $root->cloneNode(1); #Copy the original document for reverse engeneering the math elements.
  my $origdoc = XML::LibXML::Document->new("1.0","UTF-8");
  $origdoc->adoptNode($original);
  $origdoc->setDocumentElement($original);
  my $preprocessor = $self->{PREPROCESSOR};
  $preprocessor->setDocument($self->{DOCUMENT});

  my $normDoc = $preprocessor->getNormalizedDocumentObject; #Normalize with position math replacements.
  $root = $normDoc->getDocumentElement;

  #Assumption: All discourse is eventually embedded into a "textual" element as seen in @textual below.
  # Current list: p td caption contact date personname quote title toctitle keywords classification acknowledgements
  # Do inform the author if coverage is incomplete!

  #Algorithm: while @textual elements exist:
  # - Take the parent of the first found <textual>
  # - Merge all <textual>s in the parent by taking their textContent
  # - Perform sentence tokenization with normalization
  # - Construct the token-explicit markup as sentence-trees:
  #     * The sentences are marked with <sentence> tags
  #     * Tokens with <token> tags (atomic words and punctuation)
  #     * <Math> expressions remain the same
  #     * Word scopes with <word> tags
  #  Example: <word><Math>$\alpha$</Math><token>-</token><token>stable</token></word>
  # - Unbind the <textual> and bind the new <sentence> trees instead

  my $tokenizer=$self->{TOKENIZER};
  my $w = $self->{W};

  #Get an XPath context
  my $xpc = XML::LibXML::XPathContext->new($original);

  #Capture all text in "textual" elements, such as <p>, <quote>, <date>, etc.
  my @textual = qw(td caption contact date personname quote title toctitle keywords classification acknowledgements);
  my @global_ps=$root->getElementsByTagName('p');

  #Maybe we also need something done for bibliographies?
  foreach my $telement(@textual) {
     my @more = map ($_->firstChild,$root->getElementsByTagName($telement)); 
     @global_ps = (@global_ps,@more);
  }  

  while (@global_ps) {
    my $p = shift @global_ps;
    next unless $p; # Skip if undefined
    #print STDERR "\n----\nCurrent textual element: ",$p->toString(1),"\n";
    my $block = $p->parentNode;
    next unless $block && !($block->nodeName eq "#document-fragment" || $block->nodeName eq "bibliography"); # No top level elements (also skips <textual>s that are already unbound, but still in the array)! Also, skip bibliography for now.
    
    my @ps = grep ($_->nodeName !~/(enumerate|itemize|tag|#comment)$/,$block->childNodes);

    my @rest = grep ($_->nodeName =~/(enumerate|itemize|tag)$/,$block->childNodes);
    my $text = join(" ",map(__recTextContent($_),@ps));

    my $sentsRef = $tokenizer->tokenize($text);
    #print STDERR "Sentences: ",join("\n",@$sentsRef);
    my @txts=();
    foreach my $pp(@ps) {
      if (blessed($pp) ne 'XML::LibXML::Text') {
        #push @txts, grep ($_->nodeName =~/text|emph/,$pp->childNodes);
        @txts = (@txts,$pp->childNodes); }
      else {
        push @txts, $pp; }
    }

    #Prepare original text elements in order to ensure attribute preservation:
    my $current_txtnode = shift @txts;
    my $current_txtcontent = __recTextContent($current_txtnode);# unless $current_txtnode=~/XML::LibXML::Text/;
    my $mathidtoken = LLaMaPUn::Preprocessor->mathidtoken;

    #First cleanup existing children, then introduce <sentence>s instead.
    foreach my $child(@ps) {
      $child->unbindNode; }

    foreach my $sentence (@$sentsRef) {
      my $sentnode = XML::LibXML::Element->new('sentence');
      my @baselayer = split(/\s+/,$sentence);
      foreach my $base(@baselayer) {
        my $basenode;
        if ($base=~/^MathExpr(.+)\d$/) { #Formula
          $basenode = XML::LibXML::Element->new('formula'); }
        else { #Word
          $basenode = XML::LibXML::Element->new('word'); }
        #Form and attach atomic tokens
        #Hardest: MathExpr-doc0-m1-MathExpr-doc0-m2-stable-MathExpr-doc0-m3
        my @tokens;
        #Use regexps to compositionally extract the atomic tokens
        my @parts = split("-",$base);#split by dashes, as they are the only compositional markers
        @parts=($base) if $base =~/^(-+)$/;
        while (@parts>0) {
          my $part = shift @parts;
          next unless (defined $part && ($part ne ''));
          if ($part=~/^MathExpr/) {
            while (@parts && $parts[0]=~/\d|NoID$/) {
              my $idpart = shift @parts;
              if ($idpart!~/^[^0-9]/){ #Ids always start with a letter, avoids things such as $K$-3 i.e. "MathExpr-m1-3"
                #TODO: Figure out what to do with these things in principle!
                next;
              }
              $part.="-$idpart";
            }
            #Full id gathered, now recover <Math|equation|equationgroup> origin:
            my @idparts=split(/-/,$part);
            shift @idparts; #remove MathExpr?
            my $file=shift @idparts; #follows filename
            if ($file eq 'NoID') { # Missing id of formula, give up here
              my $token = XML::LibXML::Element->new('token');
              $token->appendTextNode('MissingFormula');
              $basenode->addChild($token); }
            else {
              my $xmlid=join(".",@idparts); #return id in original dotted form
              my @mnodes = ();
              my $mnode; my $xmlid_t = $xmlid;
              while (!$mnode && $xmlid_t) {
                @mnodes = $xpc->findnodes('//*[@xml:id="'.$xmlid.'"]');
                $mnode= shift @mnodes;
                chop $xmlid_t unless $mnode;
              }
              if (!$mnode) {
                #TODO: Clear out from Preprocessor/Purifier things like $X$3 that ruin the IDs.
                carp "$xmlid leads to undefined node!\n" unless $mnode;
                next;
              }
              while ($mnode->parentNode && ($mnode->parentNode->nodeName=~/^equation|equationgroup$/)) {
                $mnode=$mnode->parentNode;
              }
              my $newnode = $mnode->cloneNode(1);
              $basenode->addChild($newnode);        
          }} 
          else {
            while (defined $part && ($part ne '') && $part=~s/(($w|\d)+|.)//) {
              my $token = XML::LibXML::Element->new('token');
              $token->appendTextNode($1);
              my $toktext = $1;
              while (($current_txtcontent) && ($current_txtcontent !~ /\Q$toktext\E/)) {#Align to the appropriate text node
                $current_txtnode = shift @txts;
                $current_txtcontent = __recTextContent($current_txtnode);
              }
              $current_txtcontent =~ s/\Q$toktext\E//;#Subtract the current token from the text node content
              $current_txtcontent =~ s/^(\s|$mathidtoken)//g;
              
              if (blessed($current_txtnode) ne 'XML::LibXML::Text') {
                my @txtattrs = $current_txtnode->attributes; #Transfer all node attributes to the new token
                foreach my $attr (@txtattrs) {
                  next unless defined $attr;
                  $token->setAttribute($attr->nodeName,$attr->value)
                }
                if ($current_txtnode->nodeName eq "emph") { # Do some magic to handle emph properly
                  my $fattr = $token->getAttribute("font");
                  if (defined $fattr) {
                    $fattr.=" emph"; }
                  else {
                    $fattr="emph"; }
                  if (defined $current_txtnode->firstChild) {
                    my @innerattrs = $current_txtnode->firstChild->attributes;
                    foreach my $inattr (@innerattrs) {
                      next unless defined $inattr;
                      if ($inattr->nodeName eq "font") {
                        $fattr.=" ".$inattr->value; }
                      else {
                        $token->setAttribute($inattr->nodeName,$inattr->value); }
                    }
                  }
                  $token->setAttribute("font",$fattr);
                }
              }
              $basenode->addChild($token); #Finally, add the token to the DOM tree
            }
          }
        }
        if (@parts > 0) {
          my $token = XML::LibXML::Element->new('token');
          $token->appendTextNode("-");

          #UGH, had to copy-paste code :( ideally, should be refactored (becomes unmaintainable...)
          my $toktext="-";
          while ($current_txtcontent !~ /\Q$toktext\E/) {#Align to the appropriate text node
            $current_txtnode = shift @txts;
            $current_txtcontent = __recTextContent($current_txtnode);
          }
          $current_txtcontent =~ s/\Q$toktext\E//;#Subtract the current token from the text node content
          $current_txtcontent =~ s/^(\s|$mathidtoken)//g;
          if (blessed($current_txtnode) ne 'XML::LibXML::Text') {
            my @txtattrs = $current_txtnode->attributes; #Transfer all node attributes to the new token
            foreach my $attr (@txtattrs) {
              next unless defined $attr;
              $token->setAttribute($attr->nodeName,$attr->value)
            }
            if ($current_txtnode->nodeName eq "emph") { # Do some magic to handle emph properly
              my $fattr = $token->getAttribute("font");
              if (defined $fattr) {
                $fattr.=" emph"; }
              else {
                $fattr="emph"; }
              if (defined $current_txtnode->firstChild) {
                my @innerattrs = $current_txtnode->firstChild->attributes;
                foreach my $inattr (@innerattrs) {
                  next unless defined $inattr;
                  if ($inattr->nodeName eq "font") {
                    $fattr.=" ".$inattr->value; }
                  else {
                    $token->setAttribute($inattr->nodeName,$inattr->value) }
                }
              }
              $token->setAttribute("font",$fattr);
            }
            #END OF PASTE
            $basenode->addChild($token);
          } 
        }
        #Handle trailing dashes (morse code motivated)
        if ($base=~/[^-]/ && $base=~/-$/) {
          my $token = XML::LibXML::Element->new('token');
          $token->appendTextNode("-");
    
          #UGH, had to copy-paste code :( ideally, should be refactored (becomes unmaintainable...)
          my $toktext="-";
          while ($current_txtcontent !~ /\Q$toktext\E/) {#Align to the appropriate text node
            $current_txtnode = shift @txts;
            $current_txtcontent = __recTextContent($current_txtnode);
          }
          $current_txtcontent =~ s/\Q$toktext\E//;#Subtract the current token from the text node content
          $current_txtcontent =~ s/^(\s|$mathidtoken)//g;
          if (blessed($current_txtnode) ne 'XML::LibXML::Text') {
            my @txtattrs = $current_txtnode->attributes; #Transfer all node attributes to the new token
            foreach my $attr (@txtattrs) {
              next unless defined $attr;
              $token->setAttribute($attr->nodeName,$attr->value)
            }
            if ($current_txtnode->nodeName eq "emph") { # Do some magic to handle emph properly
              my $fattr = $token->getAttribute("font");
              if (defined $fattr) {
                $fattr.=" emph"; }
              else {
                $fattr="emph"; }
              if (defined $current_txtnode->firstChild) {
                my @innerattrs = $current_txtnode->firstChild->attributes;
                foreach my $inattr (@innerattrs) {
                  next unless defined $inattr;
                  if ($inattr->nodeName eq "font") {
                    $fattr.=" ".$inattr->value; }
                  else {
                    $token->setAttribute($inattr->nodeName,$inattr->value) }
                }
              }
              $token->setAttribute("font",$fattr);
            }
          }
          #END OF PASTE
          $basenode->addChild($token);
        }
        my @base_children = $basenode->childNodes;
        next unless scalar(@base_children); #skip if empty!

        #Generalize attributes of Word nodes, if possible:
        if ($basenode->nodeName eq "word") {
          my $ftok = $basenode->firstChild;
          my @testat = $ftok->attributes;
          foreach my $attr(@testat) {
            my $atname = $attr->nodeName;
            my $atval = $attr->value;
            my $global = 1;
            foreach my $child($basenode->childNodes) {
              next if ($child->getAttribute($atname) eq $atval);
              $global = 0;
            }
            if ($global) {
              foreach my $child($basenode->childNodes) {
                $child->removeAttribute($atname);
              }
              $basenode->setAttribute($atname,$atval);
            }
          }
        }

        #Attach basenode to sentence
        $sentnode->addChild($basenode);
      }
      $block->addChild($sentnode);
    }
    foreach my $r(@rest) {
      $r->unbindNode;
      $block->addChild($r);
    }
  }

  my @allels = $root->findnodes('//*');
  my %id=();
  foreach (@allels) {
    my $name = $_->localname;
    $id{$name}++;
    $_->setAttribute('xml:id',"$name.".$id{$name}) unless $_->getAttribute('xml:id');
  }
  # TODO : Add sentence ids and whichever else you need...
  $self->{DOCUMENT}=$normDoc;
  $self->{TOKENSMARKED}=1;  
}

sub process_document {
  #Purification - optional?
  $_[0]->mark_tokens unless $_[0]->{TOKENSMARKED};
  return $_[0]->{DOCUMENT};
}

sub get_document {
  $_[0]->{DOCUMENT};
}

1;

__END__

=pod 

=head1 NAME

C<LLLaMaPUn::Preprocessor::MarkTokens> - In-place tokenization for LaTeXML XML documents

=head1 SYNOPSIS

    use LLaMaPUn::Preprocessor::MarkTokens;
    $marktokens = LLaMaPUn::Preprocessor::MarkTokens->new(document=>"filePath"|$LibXML_Doc);
    $marktokens->setDocument("filePath"|$LibXML_Doc);
    $dom = $marktokens->process_document;
    $dom = $marktokens->get_document;

=head1 DESCRIPTION

Not currently supported (and known) issues (or features?):
 - ourQED, ourreference, ourcitation are generically replacing, respectively, the QED UTF-8 symbols, <ref> and <cite>.
 - All types of UTF-8 dashes and doublequotes are turned into the generic ASCII variants. (Are those important semantically?)
 - <inline-block> information is lost at the moment. This messes up vertical boxes from LaTeX.
 - It is not clear what we want to do with the <table> elements, so we are leaving them as-is.
 - The LaTeX ``quote'', given a noparse document, will boil down to ''quote'', where each pair of apostrophes would be
   tokenized as two distinct words, each containing a single ' token.
 - FIXME: The purification procedures have problems accessing explicit text blocks with attributes and needs to be extended to do so.

=head2 METHODS

=over 4

=item C<< my $marktokens = LLaMaPUn::Architecture::Import->new(document=>"filepath"|$LibXML_DOC); >>

Creates a new RDF marktokens object.

document  : If supplied, accepted as filepath of source for Preprocessing.
            Also, a XML::LibXML::Document object could be directly given on input.
            Alternatively, use "setDocument".

raw       : When 1, purification is not used when process_document is invoked. Default is 0.

=item C<< $marktokens->setDocument("filePath"|$LibXML_Doc); >>

Sets a new LaTeXML .tex.xml|noparse.xml document by $filePath. 
Alternatively, sets a XML::LibXML::Document object, given on input.
This allows reusing the same $preprocessor object for processing multiple documents.

=item C<< $marktokens->mark_tokens; >>

Note: This function is usually implicit, use "process_document" for a high-level conversion.

Refactors the input document to make all atomic and base layer tokens explicit, as follows:

 1. Normalizes the document using LLaMaPUn::Preprocessor with "position" math replacements
 2. Proceeds with token extraction using the algorithm:

 Assumption: All discourse is eventually embedded into a "textual" element as seen in @textual below.
 Current list: p td caption contact date personname quote title toctitle keywords classification acknowledgements
 Please, do inform the author if coverage is incomplete!

 Algorithm: while @textual elements exist:
  - Take the parent of the first found <textual>
  - Merge all <textual>s in the parent by taking their textContent
  - Perform sentence tokenization with normalization
  - Construct the token-explicit markup as sentence-trees:
      * The sentences are marked with <sentence> tags
      * Tokens with <token> tags (atomic words and punctuation)
      * <Math> expressions remain the same
      * Word scopes with <word> tags
   Example: <word><Math>$\alpha$</Math><token>-</token><token>stable</token></word>
  - Unbind the <textual> and bind the new <sentence> trees instead

=item C<< $marktokens->process_document; >>

Top level conversion driver. Returns the in-place tokenized DOM.
  - The token-marked document can also be retrieved via "$marktokens->get_document"

=item C<< $doc = $marktokens->get_document; >>

Getter method, returns the current state of the Document object.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
