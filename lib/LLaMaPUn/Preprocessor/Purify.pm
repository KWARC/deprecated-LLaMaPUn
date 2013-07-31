# /=====================================================================\ #
# |  LLaMaPUn                                                            | #
# | Purification Module                                                 | #
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

package LLaMaPUn::Preprocessor::Purify;
use warnings;
use strict;
use Carp;
use Encode;
use XML::LibXML;
use WordNet::QueryData;
use LLaMaPUn::Util;
use LLaMaPUn::LaTeXML;
use Scalar::Util qw(blessed);

our @ISA = qw(Exporter);
our @EXPORT = qw( &purify_noparse &text_math_to_XMath &purify_tokens &text_XMath_to_text &merge_math);
our $verbose=0;

#my $wn = WordNet::QueryData->new(dir=>locate_external("WordNet-dict")."/", noload => 1);
our ($wordnetdir) = grep {-d $_} ('/usr/share/wordnet/','/usr/local/wordnet/dict/');
our %dirclause = ($wordnetdir ? (dir=>$wordnetdir) : ());
our $wn = WordNet::QueryData->new(%dirclause, noload => 1);

sub purify_noparse {
  my ($class) = @_;
  my ($dom,%options);
  if ($class eq 'LLaMaPUn::Preprocessor::Purify') {
    ($class,$dom,%options) = @_; 
  } else { ($dom,%options)=@_; }
  if (! blessed($dom)) {
      # We were given a filepath -> load as an XML::LibXML object
      #FIXME: There is a known issue with spaces in the filepaths under Unix
      #LibXML throws an error as it can not properly operate such filenames
      # For now avoid spaces in $dom
      croak "Error: Filenames containing spaces are not currently supported ".
      "by XML::LibXML!\n" if ($dom=~/\s/) ;
      $dom = XML::LibXML->load_xml({
      location => $dom });
  }
  croak "Invalid XML::LibXML document!" unless (blessed($dom) eq 'XML::LibXML::Document');
  #$dom->setEncoding('UTF-8');
  $verbose=$options{verbose};
  $options{text_to_math}=1 unless defined $options{text_to_math};
  $options{math_to_text}=1 unless defined $options{math_to_text};
  $options{complex_tokens}=1 unless defined $options{complex_tokens};
  $options{merge_math}=1 unless defined $options{merge_math};

  #Print a Session Log if verbose
  #Step 1. Text to XMath
  $dom = text_math_to_XMath($dom) if $options{text_to_math};
  #Step 2. Find Complex Tokens
  $dom = purify_tokens($dom) if $options{complex_tokens};
  #Step 3. XMath to Text
  $dom = text_XMath_to_text($dom) if $options{math_to_text};
  #Step 4. Merge adjacent Math blocks
  $dom = merge_math($dom) if $options{merge_math};
  #Step 5. Output purified document
  to_file($dom,$options{out}) if $options{out};
  print STDERR "\n\n[".niceCurrentTime()."]Purification complete!\n\n" if $verbose;
  return $dom;
}

##############################################
# 1. Plain text Math to XMath                #
##############################################

#1.1. Heuristics to recognize a mathematical construct in plain text
my $consnt = qr/[bcdfghjklmnpqrstvwxz]/;
my $CONSNT = qr/[BCDFGHJKLMNPQRSTVWXZ]/;
sub is_math_construct {
  my ($class,$word) = @_;
  ($word)=@_ unless ($class eq 'LLaMaPUn::Preprocessor::Purify');
  return 0 unless (length $word > 0);
  return 1 if ($word =~ 
  / (^\d+$)        #1. Simple numbers
  |                # or
    (^(\d+?)([,\.]\d+)$) # 1.1. Decimal fractions
  |
    (^\d+[\/]\d+$) #1.2 Common Fractions
  |                # or
    ([+=><\*])     #2.Constructs containing unambiguously mathematical symbols
  |                # or
    #3. Single letters hanging in midspace (i.e. math identifiers), except "a" and "i". 
    (^[bcdefghjklmnopqrstuvwxyz]$)
  |
    #4. Constellations of 2 or 3 consonants
    (^$consnt$consnt($consnt?)$)
  /ix);
  return 1 if ($word =~ 
  /^$consnt
    (( $CONSNT(\w?) )
  |
    (yr))$
  /x);
  #DEPRECATED: Inefficient if-else way. Upgraded to single regexp
  #   #1. Simple numbers
  #   if ($word=~/^\d+$/) { 1; }
  #   #1.1 Number Fractions
  #   elsif ($word=~/^\d+\/\d+$/) { 1; }
  #   #2.Constructs containing unambiguously mathematical symbols
  #   elsif ($word=~/[+=><\*]/) { 1; }
  #   #3. Single letters hanging in midspace (i.e. math identifiers), except "a" and "i".
  #   elsif ($word=~/^[bcdefghjklmnopqrstuvwxyz]$/i) { 1; }
  #   #Else: not a math construct
  #   else { 0; }

  0;  #Else: not a math construct
}

#1.2. Subroutine in charge of converting plain text mathematics into proper XMath
sub text_math_to_XMath {
  print STDERR "\n--------------- Text to XMath ---------------\n" if $verbose;
  my ($class,$dom) = @_;
  my $mathid=0;
  $dom=$class unless ($class eq 'LLaMaPUn::Preprocessor::Purify');
  #1.2.1  #Traverse each <p> paragraph in the document:
  my @paras = $dom->getElementsByTagName('p');
  foreach my $para(@paras) {
    my @nodes = $para->getChildNodes;
    my @textnodes=();
    foreach my $node(@nodes) {
      push (@textnodes,$node) if (blessed($node) eq "XML::LibXML::Text");
    }
    #1.2.2.   #For each text node inside a paragraph
    foreach my $textel(@textnodes) {
      my $body = $textel->textContent;
      my @purified_words=();
      #1.2.3.     #Split by space to get words (ignoring punctuation for the moment)
      #v0.2 improvement: (TODO:) Use LLaMaPUn::Tokenizer::Word to get the word tokens unambiguously
      my @words = split(/\s/,$body);
      push(@words," ") if ((chop $body) eq " ");
      #TODO: removing punctuation. This is a tricky issue which we decide to not handle at this stage.
      # Gain: Potentially not losing any math context
      # Lose: Could insert in-sentence punctuation into math constructs
      # Example : 3.14 vs n! or 1+1=2.
      #
      my @properwords;
      foreach my $word (@words) {
        if ($word=~/^(.+)([\.\?\,\;\:])$/) {
          push (@properwords,$1);
          push (@properwords,$2);
        } else {
          push(@properwords,$word);
        }
      }
      @words=@properwords;
      my $current_text_content="";
      #1.2.4.     #Split the current text node into a collection of text and math nodes
      while (@words) {
        my $word = shift @words;
        #1.2.5.       #Perform heuristics to determine if we are looking at a math symbol
        if (is_math_construct($word)) {
          print STDERR "Found math in text: $word\n" if $verbose;
          #1.2.6. If this is a math symbol, create a corresponding XMath element
          #1.2.6.1 The Scalar case
          my $mathel = XML::LibXML::Element->new('Math');
          $mathid++;
          $mathel->setAttribute("xml:id","purify0.m$mathid");
          if ($word=~/^[0-9]*[,\.]?[0-9]+$/) {
            my $xmathel = XML::LibXML::Element->new('XMath');
            my $xmtok = XML::LibXML::Element->new('XMTok');
            $xmtok->appendText($word);
            $xmtok->setAttribute('meaning',$word);
            $xmtok->setAttribute('role','NUMBER');
            $xmathel->addChild($xmtok);
            $mathel->addChild($xmathel); }
          #1.2.6.2 The Simple Identifier case
          elsif ($word=~/^[bcdefghjklmnopqrstuvwxyz]$/i) {
            my $xmathel = XML::LibXML::Element->new('XMath');
            my $xmtok = XML::LibXML::Element->new('XMTok');
            $xmtok->appendText($word);
            $xmtok->setAttribute('meaning','UNKNOWN');
            $xmtok->setAttribute('role','UNKNOWN');
            $xmathel->addChild($xmtok);
            $mathel->addChild($xmathel); }
          #1.2.6.3 The Complex case - use latexmlmath to do the job for us
          # Upgrade: Use LLaMaPUn::Util::tex_to_noparse for extra speed!
          else {
          # DEPRECATED: Slow way, through the shell
          #     system("latexmlmath --XMath=tempmath.xml --noparse \"$word\"");
          #     next unless (-e "tempmath.xml");
          #     my $parser = XML::LibXML->new();
          #     my $mdoc = $parser->parse_file("tempmath.xml");
          #     my ($xmathel)=$mdoc->getElementsByTagName('XMath');
          #     unlink "tempmath.xml";

            # New way: Using the LLaMaPUn::Util API
            my ($xmathel)=tex_to_noparse('$'.$word.'$');
            my $newel = $xmathel->cloneNode(1);
            $mathel->addChild($newel); }
          #1.2.6.4 Introduce the new element to the document
          my $currtextnode = XML::LibXML::Element->new('text');
          my $bodytext = "$current_text_content ";
          $bodytext=~s/\s+$/ /;
          $currtextnode->appendText($bodytext);
          push (@purified_words,$currtextnode) if $current_text_content;
          $current_text_content="";
          push (@purified_words,$mathel); }
        #1.2.7. If we are at a normal word - glue to the already traversed context and continue
        else {
          $current_text_content.=" " unless ($word=~/^[\.\?\!\,\;\:\-]$/);
          $current_text_content.=$word;}
      }
      #1.2.8. Introduce the purified <p> to the DOM.
      foreach my $pure_el(@purified_words) {
        $para->insertBefore($pure_el,$textel); }
      my $currtextnode = XML::LibXML::Element->new('text') if $current_text_content;
      my $bodytext="$current_text_content ";
      $bodytext=~s/\s+$/ /;
      $currtextnode->appendText($bodytext) if $current_text_content;
      $para->insertBefore($currtextnode,$textel) if $current_text_content;
      $para->removeChild($textel);
    }
  }
#1.2.9.     Return a pointer to the result.
  return $dom;
}

##############################################
# 2. Recognize complex Tokens                #
##############################################

#2.0.1. List of nonWordNet constructs that could be form a complex token
#Taken from statistics gathered from arXMLiv, word - occurance pairs.
our %nonWordNet = qw(the 4843200 and 1177131 for 659508 that 571889 with 479892 this 417051 from 284670 which 242112 where 141931 these 127545 than 96449 our 95782 when 63602 since 62018 into 58180 their 55130 would 53705 they 53525 should 43718 because 40704 could 32315 those 27537 spectra 23792 although 20943 during 20420 without 17957 how 17774 what 17671 them 17271 cannot 16130 whether 11865 higgs 11673 via 11454 whose 11320 chiral 11265 shall 8925 hamiltonian 8923 whereas 8914 among 8741 itself 8279 brane 7735 spacetime 7521 towards 7506 lagrangian 7007 perturbative 6914 nuclei 6764 hadronic 6603 matrices 6497 until 6258 radii 6086 parton 6033 radiative 5920 carlo 5913 vertices 5737 upon 5627 against 5585 supersymmetric 5544 momenta 5446 renormalization 5314 conformal 5255 metallicity 5135 onto 5117 massless 5084 moduli 4985 indices 4540 branes 4378 phenomena 4321 susy 4268 observables 4189 lim 4145 electroweak 3851 emitted 3758 lensing 3748 dimensionless 3628 toward 3595 others 3591 inferred 3455 versus 3420 ansatz 3365 timescale 3311 ising 3193 monopole 3128 phenomenological 3059 supergravity 3051 schwarzschild 2971 unless 2962 low 1);

#2.0.2. List of latex operators that should form a complex token
# Warning: needs to  be revisited whenever  XMath standard changes, hence is potentially dangerous
our %MathWords = ("arccos" => { role=>'OPFUNCTION', meaning=>'inverse-cosine'},
      "arcsin" => { role=>'OPFUNCTION', meaning=>'inverse-sine'},
      "arctan" => { role=>'OPFUNCTION', meaning=>'inverse-tangent'},
      "arg" => { role=>'OPFUNCTION',    meaning=>'argument'},
      "cos" => { role=>'TRIGFUNCTION', meaning=>'cosine'},
      "cosh" => { role=>'TRIGFUNCTION', meaning=>'hyperbolic-cosine'},
      "cot" => { role=>'TRIGFUNCTION', meaning=>'cotangent'},
      "coth" => { role=>'TRIGFUNCTION', meaning=>'hyperbolic-cotangent'},
      "csc" => { role=>'TRIGFUNCTION',  meaning=>'cosecant'},
      "deg" => { role=>'OPFUNCTION',    meaning=>'degree'},
      "det" => { role=>'LIMITOP',       meaning=>'determinant'},
      "dim" => { role=>'LIMITOP',       meaning=>'dimension'},
      "exp" => { role=>'OPFUNCTION',    meaning=>'exponential'},
      "gcd" => { role=>'OPFUNCTION',    meaning=>'gcd'},
      "hom" => { role=>'OPFUNCTION'},
      "inf" => { role=>'LIMITOP',       meaning=>'infimum'},
      "ker" => { role=>'OPFUNCTION',    meaning=>'kernel'},
#     "lg" => {role=>'OPFUNCTION'},#Ambiguous!
      "lim" => { role=>'LIMITOP',       meaning=>'limit'},
      "liminf" => { role=>'LIMITOP',   meaning=>'limit-infimum'},
      "limsup" => { role=>'LIMITOP',   meaning=>'limit-supremum'},
#     "ln" => { role=>'OPFUNCTION',    meaning=>'natural-logarithm'},#Ambiguous!
      "log" => { role=>'OPFUNCTION',    meaning=>'logarithm'},
#       "max" => { role=>'LIMITOP',       meaning=>'maximum'},#Ambiguous!
#     "min" => { role=>'LIMITOP',       meaning=>'minimum'},#Ambiguous!
      "Pr" => { role=>'OPFUNCTION'},
#     "sec" => { role=>'TRIGFUNCTION',  meaning=>'secant'},#Ambiguous!
      "sin" => { role=>'TRIGFUNCTION',  meaning=>'sine'},
      "sinh" => { role=>'TRIGFUNCTION', meaning=>'hyperbolic-sine'},
      "sup" => { role=>'LIMITOP',      meaning=>'supremum'},
      "tan" => { role=>'TRIGFUNCTION', meaning=>'tangent'},
      "tanh" => { role=>'TRIGFUNCTION', meaning=>'hyperbolic-tangent'},
      "mod" => { role=>'MODIFIEROP', meaning=>'modulo'});

#2.0.3. Subroutine checking whether a given input word occurs in the WordNet dictionary
sub check_wordnet_word {
  my ($possible_word)=@_;
  if (((length $possible_word)>3) #heuristic: length >3
      && (my @foundForms = ($wn && $wn->validForms($possible_word))))#heuristic: in WordNet
    { if (@foundForms) {1;} else {0;}} else {0;}}


#2.1. Subroutine checking whether a possible word in mathematics is actually admissible by our criteria.
#     Transform it into a complex XMTok when so.
sub check_math_word {
  my ($possible_word,$root,$start_,$end_)=@_;
  #2.1.1  MathWords case - highest precedence
  if (($MathWords{$possible_word}) && (!$start_->isSameNode($end_))) {
    print STDERR "Found MathOp in Math: $possible_word\n" if $verbose;
    my $cplxtok = XML::LibXML::Element->new('XMTok');
    $cplxtok->appendText($possible_word);
    my $role = $MathWords{$possible_word}{"role"};
    $role = 'UNKNOWN' unless $role;
    my $meaning = $MathWords{$possible_word}{"meaning"};
    $meaning = 'UNKNOWN' unless $meaning;
    $cplxtok->setAttribute('role',$role);
    $cplxtok->setAttribute('meaning',$meaning);
    $cplxtok->setAttribute('font','italic');
    $root->insertBefore($cplxtok,$start_);
    my $sibling=$start_->nextSibling();
    while (!($sibling->isSameNode($end_))) {
      my $newsibling=$sibling->nextSibling;
      $root->removeChild($sibling);
      $sibling=$newsibling;
    }
    $root->removeChild($end_);
    $root->removeChild($start_);
    return 0;
  }
#2.1.2  #WordNet Case - second precedence
  else {if (check_wordnet_word($possible_word)&& (!$start_->isSameNode($end_)))
    {
      print STDERR "Found WordNet Word in Math: $possible_word\n" if $verbose;
      #possible_word is indeed a word - merge in 1 XMTok
      my $cplxtok = XML::LibXML::Element->new('XMTok');
      $cplxtok->appendText($possible_word);
      $cplxtok->setAttribute('role','UNKNOWN');#can we do better?
      $cplxtok->setAttribute('meaning','UNKNOWN');#can we do better?
      $cplxtok->setAttribute('font','italic');
      $root->insertBefore($cplxtok,$start_);
      my $sibling=$start_->nextSibling();
      while (!($sibling->isSameNode($end_))) {
        my $newsibling=$sibling->nextSibling;
        $root->removeChild($sibling);
        $sibling=$newsibling;
      }
      $root->removeChild($end_);
      $root->removeChild($start_);
      return 1;
    }
#2.1.3  #nonWordNet Case - least precedence
  else {if ($nonWordNet{$possible_word} && (!$start_->isSameNode($end_))) {
    print STDERR "Found nonWordNet Word in Math: $possible_word\n" if $verbose;
    my $cplxtok = XML::LibXML::Element->new('XMTok');
    $cplxtok->appendText($possible_word);
    my $role = 'UNKNOWN';
    my $meaning = 'UNKNOWN';
    $cplxtok->setAttribute('role',$role);
    $cplxtok->setAttribute('meaning',$meaning);
    $cplxtok->setAttribute('font','italic');
    $root->insertBefore($cplxtok,$start_);
    my $sibling=$start_->nextSibling();
    while (!($sibling->isSameNode($end_))) {
      #print "sibling: ".$sibling->toString."\n";
      my $newsibling=$sibling->nextSibling;
      $root->removeChild($sibling);
      $sibling=$newsibling;
    }
    $root->removeChild($end_);
    $root->removeChild($start_);
    return 2;
  }}}
}


#2.2. Recursive driver for the detection of complex XMToks
#     Input: Any XM* element, recursive traversal
sub handle_complex_xmtoks {
  my ($root)=@_;
  my @nodes=$root->getChildNodes;
  return unless scalar(@nodes)>0;
  my $possible_word=undef;
  my $start_;
  my $end_;
  while (scalar(@nodes)>0) {
    my $currnode=shift @nodes;
    next if $currnode->nodeName eq "#text";#sometimes we end up in text nodes, we need to skip
    if ($currnode->nodeName eq "XMTok") {
      my $letter=$currnode->textContent;
      if ($letter=~/^[a-zA-Z]+$/) {
        $start_=$currnode if (!$possible_word);
        $possible_word.=$letter;
        $end_=$currnode; }
      else {
        #handle if recognized
        check_math_word($possible_word,$root,$start_,$end_) if $possible_word;
        #then reset
        $possible_word=undef; } }
    else {
      #handle if recognized
      check_math_word($possible_word,$root,$start_,$end_) if $possible_word;
      #reset
      $possible_word=undef;
      #analyze the different math node recursively
      handle_complex_xmtoks($currnode); }
  }
  if (defined $possible_word) {
    #handle if recognized
    check_math_word($possible_word,$root,$start_,$end_) if $possible_word;
    #then reset
    $possible_word=undef;
  }
}


#2.3. Top level driver for complex XMTok formation, uses handle_complex_xmtoks
#     INPUT: Document root
sub purify_tokens {
  print STDERR "--------------- Find Complex Tokens ---------------\n" if $verbose;
  my ($class,$dom) = @_;
  $dom=$class unless ($class eq 'LLaMaPUn::Preprocessor::Purify');
  my @xmaths = $dom->getElementsByTagName('XMath');
  foreach my $xmath(@xmaths) {
    handle_complex_xmtoks($xmath);
  }
  return $dom;
}

#########################################################
# 3. Purge XMToks void of semantics back to plain text  #
# Future: LLaMaPUn::Pre::MathToText                      #
#########################################################


#3.0. Boolean sub checking whether a given XML Node is an XMHint space
sub isHintSpace {
  my ($node)=@_;
  return 1 unless $node;
  if (($node->nodeName eq 'XMHint') && ($node->getAttribute('name') =~ /space/)) {
    return  1;
  }
  return 0;
}

#3.1. Detects complex Tokens which are void of semantics and exports them out of XMath
sub text_XMath_to_text {
  print STDERR "--------------- XMath to Text ---------------\n" if $verbose;
  my ($class,$dom) = @_;
  my $mathid=0;
  $dom=$class unless ($class eq 'LLaMaPUn::Preprocessor::Purify');
  my @xmaths = $dom->getElementsByTagName('XMath');
  foreach my $xmath(@xmaths) {
    next unless ($xmath && $xmath->childNodes);
    next if (($xmath->firstChild)->isSameNode($xmath->lastChild));
    my @mathnodes=$xmath->getChildrenByTagName('XMTok');
    foreach my $mnode(@mathnodes) {
      #First heuristic - recognizable word between spaces
      if (isHintSpace($mnode->nextSibling) && isHintSpace($mnode->previousSibling))
  {#We are between spaces
    my $possible_word=$mnode->textContent;
    #Is this a recognizable word?
    if ($nonWordNet{$possible_word}||check_wordnet_word($possible_word)) {
      #Prepare a text element
      print STDERR "Found NL word in Math: $possible_word\n" if $verbose;
      my $newtextnode = XML::LibXML::Element->new('text');
      #Were the XMhints at both sides?
      my $lspace="";
      my $rspace="";
      $lspace=" " if ($mnode->previousSibling);
      $rspace=" " if ($mnode->nextSibling);
      $newtextnode->appendText("$lspace$possible_word$rspace");
      #We know we are at the top-level XMath, so we need to cut around the $mnode (if needed)
      my $mathroot = $xmath->parentNode;
      my $proot = $mathroot->parentNode;
      if (($mnode->previousSibling) && ($mnode->nextSibling)) {
        $xmath->removeChild($mnode->previousSibling);
        $xmath->removeChild($mnode->nextSibling);
        #Move all nodes following $mnode to a new Math element to follow, then remove $mnode
        my $followmath = XML::LibXML::Element->new('Math');
        $mathid++;
        $followmath->setAttribute("xml:id","purify1.m$mathid");
        my $newxmathchild = XML::LibXML::Element->new('XMath');
        $followmath->addChild($newxmathchild);
        my $followxmath = $followmath->firstChild;
        my $rightnode = $mnode->nextSibling;
        while ($rightnode) {
    my $newrightnode=$rightnode->nextSibling;
    $followxmath->addChild($rightnode);
    $xmath->removeChild($rightnode);
    $rightnode=$newrightnode;
        }
        $xmath->removeChild($mnode);
        $proot->insertAfter($newtextnode,$mathroot);
        $proot->insertAfter($followmath,$newtextnode);
        #Done?
      }
      else {#It's in the beginning or end, hence deleting it is sufficient (and inserting on the right side)
        $xmath->removeChild($mnode->previousSibling) if $mnode->previousSibling;
        $xmath->removeChild($mnode->nextSibling) if $mnode->nextSibling;
        if ($mnode->nextSibling) {
    $proot->insertBefore($newtextnode,$mathroot);
        } else {
    $proot->insertAfter($newtextnode,$mathroot);
        }
        $xmath->removeChild($mnode);
      }
    }
  }
      else {}#Second heuristic?
    }
  }
  return $dom;
}

##############################################
# 4. Merge adjacent Math blocks              #
##############################################

#4.1. Main merging subroutine
sub merge_math {
  print STDERR "--------------- Merge Math ---------------\n" if $verbose;
  my ($class,$dom) = @_;
  $dom=$class unless ($class eq 'LLaMaPUn::Preprocessor::Purify');
  my $count=0;
  my @mathels=$dom->getElementsByTagName('Math');
  foreach my $mathel(@mathels) {
    #Unlink any text between two math nodes that has only 0-2 spaces inside.
    if (($mathel->previousSibling) && ($mathel->previousSibling->nodeName eq 'text')) {
      my $prev_text = $mathel->previousSibling;
      if ($prev_text->previousSibling && ($prev_text->previousSibling->nodeName eq 'Math') && ($prev_text->textContent =~ /^\s?\s?$/)) {
        $prev_text->unbindNode;
      }
    }
    # Merge adjacent math elements
    if (($mathel->previousSibling) && ($mathel->previousSibling->nodeName eq 'Math')) {
      $count++;
      my $prevmath = $mathel->previousSibling;
      my $prevxmath = $prevmath->firstChild;
      my @XMnodes = $mathel->firstChild->getChildNodes;
      foreach my $newnode(@XMnodes) {
         $newnode->unbindNode;
         $prevxmath->addChild($newnode);
      }
      my $root = $mathel->parentNode;
      $root->removeChild($mathel);
    }
  }
  $count++ if $count;
  print STDERR "Merged $count Math blocks!\n" if $verbose;
  #v2.0 Unwrap non-Math equations:
  my @eqels = $dom->getElementsByTagName('equation');
  foreach (@eqels) {
    unless ($_->hasChildNodes() && ($_->getElementsByTagName('Math'))) {
      my $eqel = $_;
      #Just claim the equation is a paragraph:
      my $p = XML::LibXML::Element->new('p');
      my @children = map($_->cloneNode(1),$eqel->childNodes);
      foreach (@children) {
  $p->addChild($_);
      }
      $eqel->removeChildNodes();
      $eqel->replaceNode($p);
    }    
  }
  return $dom;
}

##############################################
# 4. Misc                                    #
##############################################

#5. Outputs the purified $domument into a file
sub to_file {
  my ($dom,$filename) = @_;
  print STDERR "Writing file $filename ...\n" if $verbose;
  open(FOUT,">",$filename) || die("File $filename can't be accessed!\n");
  print FOUT $dom->toString(1);
  close FOUT;
  return;
}

1;

__END__

=pod 

=head1 NAME

C<LLaMaPUn::Preprocessor::Purify> - Purification preprocessing of LaTeXML documents.

=head1 SYNOPSIS

    use LLaMaPUn::Preprocessor::Purify;
    $dom = purify_noparse($filepath,text_to_math=>0|1,math_to_text=>0|1,complex_tokens=>0|1,merge_math=>0|1,verbose=>0|1);
    $dom = text_math_to_XMath($dom);
    $dom = purify_tokens($dom);
    $dom = text_XMath_to_text($dom);
    $dom = merge_math($dom);    

=head1 DESCRIPTION

 Four purification functionality modes, following the workflow:

   1) Moves math in plain text into XMath elements

   2) Heuristically recognizes complex tokens

   3) Finds natural language structures that are void of math 
  semantics in the XMath blocks and purges them to <text>

   4) Merges adjacent Math blocks to achieve larger math contexts

 The global driver of this procedure is the purify_noparse subroutine, while the individual steps are also publicly accessible.

 Future enhancements:
 
    * Purify punctuation erroneously left inside the very end of Math blocks.
    * Formalize trailing "Reference" sections to <bibliography>
    * Formalize leading section-less text to <abstract> when unambiguous. 
    * Achieve complete document scope - currently only purifying <ltx:text> elements in <ltx:p> paragraphs.

 Prerequisites:

    * Installed LaTeXML perl Packages (integrated for speedup of latexmlmath)
    * Installed WordNet perl Packages (integrated for speedup of WordNet lookup)

=head2 METHODS

=over 4

=item C<<  $dom = purify_noparse($filepath,text_to_math=>0|1,math_to_text=>0|1,complex_tokens=>0|1,merge_math=>0|1); >>

Main driver of the purification algorithm. Takes either a filepath or a XML::LibXML::Document object on input. 
The processing can be customized as follows: 

    text_to_math : If not explicitly set to 0, purifies math fragments residing in plain text mode (<ltx:text>).
    
    math_to_text : If not explicitly set to 0, purifies text fragments residing in math mode (<ltx:Math>).

    complex_tokens : If not explicitly set to 0, purifies complex tokens residing in math mode that have been missed by LaTeXML.

    merge_math : If not explicitly set to 0, merges adjacent math blocks (ltx:math).

=item C<<      $dom = text_math_to_XMath($dom); >>

Given a XML::LibXML::Document object on input, purifies math fragments residing in plain text mode (<ltx:text>).

=item C<<      $dom = purify_tokens($dom); >>

Given a XML::LibXML::Document object on input, purifies complex tokens residing in math mode that have been missed by LaTeXML.

=item C<<      $dom = text_XMath_to_text($dom); >>

Given a XML::LibXML::Document object on input, purifies text fragments residing in math mode (<ltx:Math>).

=item C<<      $dom = merge_math($dom);    >>

Given a XML::LibXML::Document object on input, merges adjacent math blocks (ltx:math).

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
