# /=====================================================================\ #
# |  LLaMaPUn                                                            | #
# | Preprocessor Module                                                 | #
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
package LLaMaPUn::Preprocessor;
use warnings;
use strict;
use feature 'switch';

use Carp;
use Encode;
use Scalar::Util qw/blessed/;
use LLaMaPUn::Util;

use vars qw($VERSION);
$VERSION = "0.0.1";
#**********************************************************************

#Regexp for MathIDs:
our $mathidtoken = qr/MathExpr(\w*)([-](\w+)(\d+))+/;
our $zero = 0; # I am amazed I had to resort to this...
sub mathidtoken { $mathidtoken; }

sub new {
  my($class,%options)=@_;
  my ($doc,$name,$path)=(undef,undef,undef);
  if ($options{document}) {
    $doc=$options{document};
    if (! blessed($doc)) {
      ($path,$name)=splitFilepath($doc);
      $doc=parse_file($doc);    
    }}
  $options{replacemath}="position" unless $options{replacemath};
  bless {
    DOCUMENT=>$doc,
    NAME=>$name,
    PATH=>$path,
    REPLACEMATH=>$options{replacemath},
    NORMALIZED=>0,
    MATHCOUNT=>0,
    CITECOUNT=>0,
    REFCOUNT=>0,
    MATHMAP=>{},
    MATHIDMAP=>{},
    CITEMAP=>{},
    CITEIDMAP=>{},
    REFMAP=>{},
    REFIDMAP=>{}}, $class; }

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

sub setDocument {
  my ($self,$source)=@_;
  return unless $source;
  if (blessed($source)) {
    $self->{DOCUMENT}=$source; }
  else {
    my ($path,$name)=splitFilepath($source);
    $self->{DOCUMENT}=parse_file($source);
    $self->{NAME}=$name;
    $self->{PATH}=$path; }
  $self->{NORMALIZED}=0; }

#Core: The complex normalization towards plain text in all <p> elements
#      also supporting some structures on the logic level
sub normalize {
  my ($self,$target)=@_;
  return if $self->{NORMALIZED}; # Only normalize once per document
  $self->{NORMALIZED}=1; # Mark as done.
  my $doc = $self->{DOCUMENT};
  my $fid=$self->{NAME}||"doc0";
  my $fakeeq=0; #Fake equations, exposed after purification?
  my $citeid=0; #Citation counter
  my $refid=0; #Ref counter
  $fid.="0" unless ($fid=~/\d$/);
  my $root = $doc->getDocumentElement;

  #Abolish all <note>s until we get a better idea what to do.
  #TODO: Do something smart with <note>
  my @notes = $root->getElementsByTagName('note');
  foreach my $note(@notes) {
    $note->unbindNode; }

  my @equations = $root->getElementsByTagName('equationgroup');
  foreach my $segment(@equations) {
    my $mark="";
    my $id="";
    if ($self->{REPLACEMATH} eq "position") {
      $id=$segment->getAttribute('xml:id');
      if (!defined $id) {
        my ($math_node)=$segment->getElementsByTagName('equation');
        $id=$math_node->getAttribute('xml:id');
        if (!defined $id) {
          ($math_node)=$math_node->getElementsByTagName('Math');
          $id=$math_node->getAttribute('xml:id');
        }}
      if ($id) {
        $id=~s/\.(\d+)\./.p$1./g; # No single digits are allowed, assuming P
        $id=~s/(\D)\./$1$zero./g; # Same problem with words without trailing digits
        $id=~s/\./-/g;
        $mark="$fid-$id"; }
      else {
        $mark="NoID"; }
      #my @xmath_nodes=$segment->getElementsByTagName('XMath');
      #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
      $self->{MATHIDMAP}{"$mark"} = $segment; }
    elsif ($self->{REPLACEMATH} eq "syntax") {
      #my @xmath_nodes=$segment->getElementsByTagName('XMath');
      #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
      my $mathseg = $segment->toString;
      $mathseg=~s/\n+//g;
      $mathseg=~s/(\s+)/ /g;
      $self->{MATHCOUNT}+=1;
      $self->{MATHMAP}{$mathseg} = scalar(keys %{$self->{MATHMAP}})+1 unless defined $self->{MATHMAP}{$mathseg};
      $mark=$self->{MATHMAP}{$mathseg}; }
    else {
      $mark=""; }  
    $mark="-$mark" if $mark;
    my $para_node = XML::LibXML::Element->new("p");
    $para_node->appendTextNode(" MathExprEquation$mark ");
    $segment->replaceNode($para_node);
  }
  @equations = $root->getElementsByTagName('equation');
  foreach my $segment(@equations) {
    my $id;
    my $mark;
    if ($self->{REPLACEMATH} eq "position") {
      $id =$segment->getAttribute('xml:id');
      if (!defined $id) {
        my ($math_node)=$segment->getElementsByTagName('Math');
        if ($math_node) {
          $id=$math_node->getAttribute('xml:id'); }
        else {
          # "Fake" equations, used for italics
          # The Purify.pm module should grow to abolish tese to begin with.
          $fakeeq++;
          $id="fake$fakeeq"; } }
      if ($id) {
        $id=~s/\.(\d+)\./.p$1./g; # No single digits are allowed, assuming P
        $id=~s/(\D)\./$1$zero./g; # Same problem with words without trailing digits
        $id=~s/\./-/g;
        $mark="$fid-$id"; }
      else {
        $mark="NoID"; }

      #my @xmath_nodes=$segment->getElementsByTagName('XMath');
      #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
      $self->{MATHIDMAP}{"$mark"} = $segment; }
    elsif ($self->{REPLACEMATH} eq "syntax") {
      #my @xmath_nodes=$segment->getElementsByTagName('XMath');
      #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
      my $mathseg = $segment->toString;
      $mathseg=~s/\n+//g;
      $mathseg=~s/(\s+)/ /g;
      $self->{MATHCOUNT}+=1;
      $self->{MATHMAP}{$mathseg} = scalar(keys %{$self->{MATHMAP}})+1 unless defined $self->{MATHMAP}{$mathseg};
      $mark=$self->{MATHMAP}{$mathseg}; }
    else {
      $mark=""; }  
    $mark="-$mark" if $mark;
    my $para_node = XML::LibXML::Element->new("p");
    $para_node->appendTextNode(" MathExprEquation$mark ");
    $segment->replaceNode($para_node);
  }
  #Gather textual segments:
  my @textual = qw(td caption contact date personname quote title toctitle p);
  my @segments;
  foreach my $telement(@textual) {
     my @more = $root->getElementsByTagName($telement); 
     @segments = (@segments,@more);
  }
  
  # Admissible noise : (  emph  |  acronym  |  rule  |  anchor  |  ref  |  cite  |  bibref)
  #                   meta: (note  |  indexmark  |  ERROR)
  foreach my $segment(@segments) {
    #test for occurances of mathematical fragments
    my @tonormalize=$segment->getElementsByTagName('Math');
    foreach my $math_node(@tonormalize) {
      my $id= $math_node->getAttribute('xml:id');
      #Skip, if nested <Math><Math>
      next if $id=~/m(\d+)\.m(\d+)/;
      my $mark;
      if ($self->{REPLACEMATH} eq "position") {
        $id=$math_node->getAttribute('xml:id');
        if (!$id) {
          $mark="NoID"; }
        else {
          $id=~s/\.(\d+)\./.p$1./g; # No single digits are allowed, assuming P
          $id=~s/(\D)\./$1$zero./g; # Same problem with words without trailing digits
          $id=~s/\./-/g; }
        $mark="$fid-$id";
        #my @xmath_nodes=$math_node->getElementsByTagName('XMath');
        #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
        $self->{MATHIDMAP}{"$mark"} = $math_node; }
      elsif ($self->{REPLACEMATH} eq "syntax") {
        #my @xmath_nodes=$math_node->getElementsByTagName('XMath');
        #my $mathseg=join('',map($_->toString,@xmath_nodes)); 
        my $mathseg = $math_node->toString;
        $mathseg=~s/\n+//g;
        $mathseg=~s/(\s+)/ /g;
        $self->{MATHCOUNT}+=1;
        $self->{MATHMAP}{$mathseg} = scalar(keys %{$self->{MATHMAP}})+1 unless defined $self->{MATHMAP}{$mathseg};
        $mark=$self->{MATHMAP}{$mathseg}; }
      else {
        $mark=""; }
      $mark="-$mark" if $mark;
      my $text_node = XML::LibXML::Text->new(" MathExpr$mark");
      $math_node->replaceNode($text_node);
    }
    @tonormalize=$segment->getElementsByTagName('cite');
    foreach my $cite_node(@tonormalize) {
      my $mark;
      if ($self->{REPLACEMATH} eq "position") {
        $mark="$fid-c$citeid";
        $citeid++;
        $self->{CITEIDMAP}{"$mark"} = $cite_node; }
      elsif ($self->{REPLACEMATH} eq "syntax") {
        my $citeseg=$cite_node->toString; 
        $citeseg=~s/\n+//g; #Normalize empty lines
        $citeseg=~s/(\s+)/ /g;#And spaces
        $self->{CITECOUNT}+=1;
        $self->{CITEMAP}{$citeseg} = scalar(keys %{$self->{CITEMAP}})+1 unless defined $self->{CITEMAP}{$citeseg};
        $mark=$self->{CITEMAP}{$citeseg}; }
      else {
        $mark=""; }
      $mark="-$mark" if $mark;
      my $text_node = XML::LibXML::Text->new(" ourcitation$mark ");
      $cite_node->replaceNode($text_node);
    }
    @tonormalize=$segment->getElementsByTagName('ref');
    foreach my $ref_node(@tonormalize) {
      my $mark;
      if ($self->{REPLACEMATH} eq "position") {
        $mark="$fid-r$refid";
        $refid++;
        $self->{REFIDMAP}{"$mark"} = $ref_node; }
      elsif ($self->{REPLACEMATH} eq "syntax") {
        my $refseg=$ref_node->toString; 
        $refseg=~s/\n+//g; #Normalize empty lines
        $refseg=~s/(\s+)/ /g;#And spaces
        $self->{REFCOUNT}+=1;
        $self->{REFMAP}{$refseg} = scalar(keys %{$self->{REFMAP}})+1 unless defined $self->{REFMAP}{$refseg};
        $mark=$self->{REFMAP}{$refseg}; }
      else {
        $mark=""; }

      $mark="-$mark" if $mark;
      my $text_node = XML::LibXML::Text->new(" ourreference$mark ");
      $ref_node->replaceNode($text_node);
    }
    # Pluck out any unsupported tag:
    my @unsupported_tags = qw/acronym anchor rule note break/;
    foreach my $tag(@unsupported_tags) {
      my @unsupported_elements = $segment->getElementsByTagName($tag);
      foreach my $unsupported_element(@unsupported_elements) {
        my $space = ($tag eq 'break') ? "\n" : " ";
        my $text_node = XML::LibXML::Text->new($space);
        $unsupported_element->replaceNode($text_node);
      }
    }
  }
  $self->{DOCUMENT}=$doc;
}

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
# Getter methods interface:
sub getNormalizedElement {
  my ($self,$target)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my $doc = $self->{DOCUMENT};
  my $fid=$self->{NAME}||"doc0";
  $fid.="0" unless ($fid=~/\d$/);
  
  my $root = $doc->getDocumentElement;
  my ($element)=$root->getElementsByTagName($target);
  my $init="";
  my $result=\$init;
  $$result = $element->textContent if $element;
  $$result = trimspaces($$result);
  $result;
}

sub getNormalizedElements {
  my ($self,$target)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my $doc = $self->{DOCUMENT};
  my $fid=$self->{NAME}||"doc0";
  $fid.="0" unless ($fid=~/\d$/);

  my $root = $doc->getDocumentElement;
  my @elements=$root->getElementsByTagName($target);
  my $result=[];
  foreach my $element(@elements) {
   push @$result, trimspaces($element->textContent) if $element;
  }
  $result;
}

sub getNormalizedBody {
  my ($self,$target)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my $doc = $self->{DOCUMENT};
  my $fid=$self->{NAME}||"doc0";
  $fid.="0" unless ($fid=~/\d$/);
  my $droot=$doc->getDocumentElement;
  my $root=$droot->cloneNode(1);
  my $init="";
  my $result=\$init;
  my @elementsToUnbind;
  push @elementsToUnbind, $root->getElementsByTagName('abstract');
  push @elementsToUnbind, $root->getElementsByTagName('title');
  push @elementsToUnbind, $root->getElementsByTagName('toctitle');
  push @elementsToUnbind, $root->getElementsByTagName('creator');
  push @elementsToUnbind, $root->getElementsByTagName('keywords');
  push @elementsToUnbind, $root->getElementsByTagName('classification');
  push @elementsToUnbind, $root->getElementsByTagName('acknowledgements');
  push @elementsToUnbind, $root->getElementsByTagName('bibliography');
  foreach my $element(@elementsToUnbind) {$element->unbindNode();}
  my @segments = $root->getElementsByTagName('p');
  foreach my $segment(@segments) {
    my $rawTextContent = $segment->textContent if $segment;
    $rawTextContent.=" ";
    $rawTextContent=join(" ",split("\n",$rawTextContent));
    $$result.=$rawTextContent;
  }
  $$result = trimspaces($$result);
  return $result; 
}

sub getNormalized {
  my ($self,$target)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my $doc = $self->{DOCUMENT};
  my $fid=$self->{NAME}||"doc0";
  $fid.="0" unless ($fid=~/\d$/);
  
  my $root=$doc->getDocumentElement;
  my $init="";
  my $result=\$init;
  $$result=encode('UTF-8',trimspaces($root->textContent)) if $root;
  $result;
}

sub getNormalizedDocumentObject {
  my ($self)=@_;
  $self->normalize unless $self->{NORMALIZED};
  $self->{DOCUMENT};
}

sub getEntry {
  my ($self,$id)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my $type;
  given ($id) {
    when (/^MathExpr/) {$type = "MATH";}
    when (/^ourcitation/) {$type = "CITE";}
    when (/^ourreference/) {$type = "REF";}
  }
  if ($self->{REPLACEMATH} eq "syntax") {
    $id=~s/(.+)-(\d+)$/$2/;
    while (my ($math, $mid) = each %{$self->{uc($type)."MAP"}}) {
      return $math if $mid==$id;
    }
    return ""; }
  elsif ($self->{REPLACEMATH} eq "position") {
    $id=~s/\s+$//g; #remove trailing \n and whitespace
    $id=~s/\)$//;#remove ending )
    my @parts=split(/-/,$id);
    shift @parts; #remove MathExpr/ourcitation/ourreference
    my $mark=join("-",@parts); #retrieve original mark
    return $self->{uc($type)."IDMAP"}{$mark};
  }
  else { return ""; }
}

sub getEntries {
  my ($self)=@_;
  $self->normalize unless $self->{NORMALIZED};
  my %entries;
  foreach my $type(qw/MATH REF CITE/) {
    if ($self->{REPLACEMATH} eq "syntax") {
      %entries = (%entries,%{$self->{uc($type)."MAP"}});
    }
    elsif ($self->{REPLACEMATH} eq "position") {
      %entries=(%entries,%{$self->{uc($type)."IDMAP"}});
    }
  }
  return %entries;
}

sub xmlid {
  my ($self,$math) = @_;
  $math=~s/\s+$//g; #remove trailing \n and whitespace
  chop $math if $math=~/\)$/;#remove )
  my @parts=split(/-/,$math);
  shift @parts if $parts[0]=~/^MathExpr/; #remove MathExpr if present
  my $file=shift @parts; #remove the following filename
  my $xmlid=join(".",@parts); #return id in original dotted form
  $xmlid=~s/^s/S/;
  $xmlid; }

sub trimspaces {
  # Trim spaces
  my $string = shift;
  $string =~ s/\s\s+/ /g;
  $string =~ s/^\s+//g;
  $string =~ s/\s+$//g;
  return $string; }

1;

__END__

=pod 

=head1 NAME

C<LLaMaPUn::Preprocessor> - Preprocessing module for LaTeXML tex|noparse.xml documents

=head1 SYNOPSIS

    use LLaMaPUn::Preprocessor;
    $preprocessor=LLaMaPUn::Preprocessor->new(replacemath=>"position|syntax|generic",document=>"filePath"|$LibXML_Doc);
    $preprocessor->setDocument("filePath"|$LibXML_Doc);
    $preprocessor->normalize; # usually left implicit
    $normalizedBody = $preprocessor->getNormalizedBody;
    $normalizedDoc = $preprocessor->getNormalized;
    $normalizedObject = $preprocessor->getNormalizedDocumentObject;
    $someEnvironment = $preprocessor->getNormalizedElement('someEnvironment');
    $allEnvironments = $preprocessor->getNormalizedElements('someEnvironment');
    $math = $preprocessor->getMathEntry($replacement_id);
    %mathmap = $preprocessor->getMathEntries;
    $citebody = $preprocessor->getCiteEntry($replacement_id);
    %citemap = $preprocessor->getCiteEntries;
    $refbody = $preprocessor->getRefEntry($replacement_id);
    %refmap = $preprocessor->getRefEntries;
    $mathidtoken = LLaMaPUn::Preprocessor->mathidtoken;
    $xmlid = LLaMaPUn::Preprocessor->xmlid($replacement_id);

=head1 DESCRIPTION

Preprocessing module carrying a sane transition from LaTeXML ".tex.xml|.noparse.xml" documents to plain text.

=head2 METHODS

=over 4

=item C<< $preprocessor = LLaMaPUn::Preprocessor->new(%options); >>

Creates a new LLaMaPUn preprocessor object for normalizing LaTeXML-based XML documents

document  : If supplied, accepted as filepath of source for Preprocessing.
            Also, a XML::LibXML::Document object could be directly given on input.
            Alternatively, use "setDocument".

replacemath : Determine the normalization behaviour when substituting XML <Math> blocks.
              Three options:
              - 'position': give a unique substitution based on filename and xml:id attribute
                 Example: MathExpr-f000001-S1.p1.m1
              - 'generic': give a generic replacement, i.e. "MathExpr" or "MathExprEquation"
                 depending on whether we substitute a <Math> or <equation|equationgroup> block
              - 'syntax': Replace syntactically equivalent <Math> blocks with equivalent
                 text named-entities. 
                 WARNING: Currently uses <Math> as hash keys, could be dangerously memory-intensive!

=item C<< $preprocessor->setDocument("filePath"|$LibXML_Doc); >>

Sets a new LaTeXML .tex.xml|noparse.xml document by $filePath. 
Alternatively, sets a XML::LibXML::Document object, given on input.
This allows reusing the same $preprocessor object for processing multiple documents.

=item C<< $preprocessor->normalize; >>

Explicitly normalize the document set at the $preprocessor object. 
The normalization uses generic substitutions to create a plain text representation
of the original LaTeXML .tex/noparse.xml document.

Note that invoking the normalization is usually done internally and is not required.

=item C<< $normalizedBody = $preprocessor->getNormalizedBody; >>

Returns a scalar reference to the normalized "body" of the document. While
the body is not an explicit XML block, the method weeds out all metadata logical level
blocks and returns the textual content of the remaining document which we consider the "body".

=item C<< $normalizedDoc = $preprocessor->getNormalized; >>

Returns a scalar reference to the entire normalized document

=item C<< $normalizedObject = $preprocessor->getNormalizedDocumentObject; >>

Returns the XML::LibXML::Document representation of the normalized document.

=item C<< $someEnvironment = $preprocessor->getNormalizedElement('someEnvironment'); >>

Returns a scalar reference to the normalization of the first environment 'someEnvironment' occuring in the document.
Useful for potentially unique environments such as 'title', 'abstract', 'bibliography', etc.

=item C<< $allEnvironments = $preprocessor->getNormalizedElements('someEnvironment'); >>

Returns an array reference to all normalized environments 'someEnvironment'

=item C<< $math = $preprocessor->getMathEntry($replacement_id); >>

1. When replacemath=>"syntax" : Given an entry id on input, retrieves the corresponding Math string.
2. When replacemath=>"position" : Given a MathExpr-... id on input, retrieves the corresponding Math LibXML element.

=item C<< %mathmap = $preprocessor->getMathEntries; >>

For replacemath=>"syntax|position":
Retrieves the hash containing the mapping between the Math map representation and its respective substitute id.

=item C<< $citebody = $preprocessor->getCiteEntry($replacement_id); >>

Analog to 'getMathEntry', but for citation elements.

=item C<< %citemap = $preprocessor->getCiteEntries; >>

Analog to 'getMathEntries', but for citation elements.

=item C<< $refbody = $preprocessor->getRefEntry($replacement_id); >>

Analog to 'getMathEntry', but for ref elements.

=item C<< %refmap = $preprocessor->getRefEntries; >>

Analog to 'getMathEntries', but for ref elements.

=item C<< $mathidtoken = LLaMaPUn::Preprocessor->mathidtoken; >>

Getter method for the internal regexp matching the normalized Math IDs, obtained when preprocessing with replacemath=>"position".

=item C<< $xmlid = LLaMaPUn::Preprocessor->xmlid($replacement_id); >>

Converts the replacement math id to the original xml:id attribute of the source document.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
