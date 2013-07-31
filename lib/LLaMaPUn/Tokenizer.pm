# /=====================================================================\ #
# |  LaMaPuN                                                            | #
# | Tokenization Module                                                 | #
# |=====================================================================| #
# | Part of the LaMaPUn project: http://kwarc.info/projects/lamapun/    | #
# |  Research software, produced as part of work done by the            | #
# |  KWARC group at Jacobs University,                                  | #
# | Copyright (c) 2009 LaMaPUn group                                    | #
# | Released under the GNU Public License                               | #
# |---------------------------------------------------------------------| #
# | Deyan Ginev <d.ginev@jacobs-university.de>                  #_#     | #
# | http://kwarc.info/people/dginev                            (o o)    | #
# \=========================================================ooo==U==ooo=/ #

package LaMaPUn::Tokenizer;
use strict;
use Carp;
use LaMaPUn::Util;
use Encode;

our %Tokenizer = (sentence=>"LaMaPUn::Tokenizer::Sentence",
	      chunk=>"LaMaPUn::Tokenizer::Chunk",
	      word=>"LaMaPUn::Tokenizer::Word");

sub new {
  my($class,%options)=@_;
  #Default is sentence
  $options{token}="sentence" unless $options{token};
  $options{input}="text" unless $options{input};
  $options{mergeMath}=1 unless defined $options{mergeMath};
  $options{normalize}=1 unless defined $options{normalize};
  my $tokenizer=$Tokenizer{lc($options{token})}->new();
  bless ({TOKENIZER=>$tokenizer,
	 MERGEMATH=>$options{mergeMath},
	 INPUT=>lc($options{input}),
         NORMALIZE=>$options{normalize}}, $class);
}

sub tokenize_file {
  my ($self,$input,$output)=@_;
  my $tokens=[];
  if ($output) {#Case 1: write to file
    croak "File $input not accessible for read!\n" unless (open(DATA,"<$input"));
    croak "File $output not accessible for write!\n" unless (open(OUT,">$output"));
    while (<DATA>) {
      chop $_;    
      my $gottokens=$self->tokenize(\$_);
      push @$tokens, @$gottokens;
      print OUT map ($_."\n",@$gottokens);
    }
  }
  else { #Case 2: return ref to array of resulting tokens
    croak "File $input not accessible for read!\n" unless (open(DATA,"<$input"));
    while (<DATA>) {
      chop $_;
      push @$tokens, @{$self->tokenize(\$_)};
    }
  }
  $tokens;
}

sub tokenize {
  my ($self,$tokenRefO,%options)=@_;
  my $tokenRef=$tokenRefO;
  $tokenRef=\$tokenRefO unless (ref $tokenRefO); #Handle "raw" input data as ref
  my $tokenizer=$self->{TOKENIZER};
  my $type=$self->{INPUT};
  if (($type eq "text") && ($tokenizer->isa($Tokenizer{chunk}))) {
     my $sent_tokenizer=$Tokenizer{sentence}->new();
     $$tokenRef=join("\n",@{$sent_tokenizer->tokenize($tokenRef)});
  }
  $tokenRef = $tokenizer->tokenize($tokenRef,%options);
  $tokenRef = $tokenizer->mergeMath($tokenRef) if $self->{MERGEMATH};
  $tokenRef = $tokenizer->normalize($tokenRef) if $self->{NORMALIZE};
  $tokenRef;
}


#============================================================================== 
# Sentence Tokenization
package LaMaPUn::Tokenizer::Sentence;
use base qw(LaMaPUn::Tokenizer);
use strict;
use Encode;
use Lingua::EN::Sentence qw( get_sentences add_acronyms get_acronyms );
add_acronyms('fig','eq','sec','i.e','e.g','P-a.s','cf','Thm','Def','Conj','resp');

sub new {
  my ($class)=@_;
  my $self={};
  bless ($self,$class);
}

sub tokenize {
  my $w=LaMaPUn::Tokenizer::Word->w;
  if (${$_[1]}=~/$w/) {
    get_sentences(${$_[1]}); #split by sentences, given a reference to a
                             #text scalar. output is sentence array ref
  }
  else {
    my @ret= (${$_[1]});
    \@ret;  #if only noise chars - assume this is some technical input which is to be returned as-is
  }
}

sub mergeMath {
  my $inref=$_[1]; #reference to an array of sentences/chunks
  my $outref=[];
  foreach my $sentence(@$inref) {
    $sentence =~ s/\(?MathExprEquation((\d|\w|-)*)\)?\s+\(?MathExprEquation((\d|\w|-)*)\)?/MathExprEquation$1/g;
    push @$outref, $sentence;
  }
  undef $inref;
  $outref;
}

#######maw
sub getAbbrevs {
    my @tmpabbrevs = map { quotemeta($_) } (get_acronyms());
    my $abbrevs=join("|", @tmpabbrevs);
    return $abbrevs;
}
#######

sub normalize {
  my $inref=$_[1];
  my $outref=[];
  my $mathidtoken = LaMaPUn::Preprocessor->mathidtoken;
#  my $abbrevs=getAbbrevs();#maw
  my $w=LaMaPUn::Tokenizer::Word->w;
  foreach my $sentence(@$inref) {
    next if $sentence =~ /^$/;
    $sentence =~ s/ +/ /go;# copied from final "maw"
    $sentence =~ s/^ +| +$//go;#start with space normalization (we're coming from the XML world after all)
    #Also fix the temporary hack for newline
    $sentence =~ s/\(?MathExpr((\d|\w|-)*)\)?\s+([A-Z])/MathExpr$1newlinecrap$3/mg;
    $sentence =~ s/(\w)(\(?$mathidtoken\)?)/$1 $2/mg;
    $sentence =~ s/(\(?$mathidtoken\)?)($w)/$1 $6/mg;
    $sentence =~ s/(\w)ourcitation/$1 ourcitation/mg;
    $sentence =~ s/ourcitation(\w)/ourcitation $1/mg;
    #Temproray hacky handling of dashes, please improve!
    $sentence =~ s/\x{2014}|\x{2013}|-/thisisadash/g; #Move all dashes to ascii (and use a replacement to make our life easy)
 #   $sentence =~ s/(\x{2014}|\x{2013})/thisisutfdash/g;

#    $sentence =~ s/([a-zA-Z])(\W)/$1 $2/g;#dg
#    $sentence =~ s/(?:(?!$abbrevs)|(?![A-Z]{1}))(\W)/$1 $2/g;#maw

    #downgrade Unicode to ASCII for ease of use
    #DG Question: Do we lose semantics in the long run with this?
    $sentence =~ s/(\x{201C}|\x{201D}|\x{0022})/"/g;
    $sentence =~ s/(\x{0027}|\x{0060}|\x{2019}|\x{2018})/'/g; 

    $sentence =~ s/([a-zA-Z])(\'s)/$1 $2/g;#maw : saxon gen
#    $sentence =~ s/(\W)([a-zA-Z])/$1 $2/g;
    $sentence =~ s/ (\w+)([\:\;\,\"\'])/ $1 $2 /g;#maw
    $sentence =~ s/([\:\;\,\"\'])(\w+)/ $1 $2 /g;#maw
    $sentence =~ s/(\') (s)/$1$2/g;#maw : fix saxon gen again
    $sentence =~ s/([()])(.)/$1 $2/g;
    $sentence =~ s/(.)([()])/$1 $2/g;
    $sentence =~ s/(,)(\D)/$1 $2/g;
    $sentence =~ s/(\D)(,)/$1 $2/g;
    $sentence =~ s/ ((\d\.)*)([,])/ $1 $3/g;#maw number 
    $sentence =~ s/ (\w+[\.\!\?]*)(\")| (\w+)(\'\')/ $1 $2/g;
    $sentence =~ s/(MathExpr((\d|\w|-)*))([,.?!])/$1 $4/g;
    $sentence =~ s/\(\s*ourreference\s*\)/ourreference/;
    $sentence =~ s/thisisadash/-/g;
    $sentence =~ s/-(\s+)\./-./; # Unspace -. Weird fix, probably needed only for morse code
    #End of hacky dashes

#    $sentence =~ s/ ([\.\?\!]) /$1 newlinecrap/g;#dg
    $sentence =~ s/( [\.\?\!])(?= +)(?!( +\))) / $1 newlinecrap/g;#maw 
    $sentence =~ s/([\.\?\!] +\)) / $1 newlinecrap/g;#maw
    $sentence =~ s/ ([\.\?\!])( *\r*\n)/$1 newlinecrap/g;#maw
    $sentence =~ s/([\.\?\!]) (\))/ $1 $2/g;#maw

    $sentence =~ s/\x{220E}|\x{25AE}|\x{2023}|\x{25A0}|\x{25A1}|\x{25AA}/ourQED newlinecrap /g;

    $sentence = join(" ",split(/\s+/,$sentence));
#    $sentence =~ s/newlinecrap +/\n/go;#dg
    $sentence =~ s/ ((\w|\d|,|;|\'|\")*?)([\.\?\!]) (newlinecrap)/ $1 $3 $4/g;#maw+morse fix
    $sentence =~ s/ ((\w|\d|,|;|\'|\")*?)([\.\?\!])$/ $1 $3/g;#maw+ morse fix
    $sentence =~ s/newlinecrap */\n/go;#maw

    $sentence =~ s/ +/ /go;#maw 
    $sentence =~ s/^ +| +$//go;#maw
    $sentence =~ s/\.\.(\s+)\./.../g; #Fix ... case
    
    push @$outref, split(/\n/m,$sentence);#dg
  }
  undef $inref;
  $outref;
}

#============================================================================== 
# Chunk Tokenization
package LaMaPUn::Tokenizer::Chunk;
use base qw(LaMaPUn::Tokenizer);
use strict;

sub new {
  my ($class)=@_;
  my $self={};
  bless ($self,$class);
}

sub tokenize {
  my $inref=$_[1];
  my $outref=[];
  $$inref =~ s/\x{2014}|\x{2013}/-/g; #Move all dashes to ascii
  $$inref =~ s/(\x{201C}|\x{201D}|\x{0022})/"/g;
  $$inref =~ s/(\x{0027}|\x{0060}|\x{2019}|\x{2018})/'/g; 
  $$inref=~s/(\w|\s|\))\.$/$1/mg;
  $$inref=~s/\s*([\,\;\:\'\"\`]|\‘|\’|\,|\“|\”)\s*/\n/mg;
  $$inref=~s/\s+-\s+/\n/mg;
  $$inref=~s/^\W$//mg;
  push @$outref, split(/\n/m,$$inref);
  undef $inref;
  $outref;
}

sub mergeMath {
  LaMaPUn::Tokenizer::Sentence->mergeMath($_[1]);
}

sub normalize {
  LaMaPUn::Tokenizer::Sentence->normalize($_[1]);
}

#============================================================================== 
# Word Tokenization
package LaMaPUn::Tokenizer::Word;
use base qw(LaMaPUn::Tokenizer);
use strict;
use encoding 'utf8';

#TODO: Test if dates work. Written on 26.10.2010
#All special Unicode alphabetical latin symbols:
our $special=qr/[šžŸÀÂÃÄÅÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖÙÚÛÜÝàáâãäåæçèéêëìíîïðñòóôõöøùúûüýÿĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİĴĵĶķĹĺĻļĽľŁłŃńŅņŇňŌōŎŏŐőŔŕŖŗŘřŚśŜŝŞşŠšŢţŤťŦŧŨũŪūŬŭŮůŰűŲųŴŵŶŷŸŹźŻżŽžƵƶǍǎǏǐǑǒǓǔǕǖǗǘǙǚǛǜǞǟǠǡǥǦǧǨǩǪǫǬǭǰǴǵǸǹǺǻǼǽǾǿȀȁȂȃȄȅȆȇȈȉȊȋȌȍȎȏȐȑȒȓȔȕȖȗȘșȚțȞȟȦȧȨȩȪȫȬȭȮȯȰȱȲȳḀḁḂḃḄḅḆḇḈḉḊḋḌḍḎḏḐḑḒḓḔḕḖḗḘḙḚḛḜḝḞḟḠḡḢḣḤḥḦḧḨḩḪḫḬḭḮḯḰḱḲḳḴḵḶḷḸḹḺḻḼḽḾḿṀṁṂṃṄṅṆṇṈṉṊṋṌṍṎṏṐṑṒṓṔṕṖṗṘṙṚṛṜṝṞṟṠṡṢṣṤṥṦṧṨṩṪṫṬṭṮṯṰṱṲṳṴṵṶṷṸṹṺṻṼṽṾṿẀẁẂẃẄẅẆẇẈẉẊẋẌẍẎẏẐẑẒẓẔẕẖẗẘẙẠạẤấẦầẪẫẬậẮắẰằẴẵẶặẸẹẼẽẾếỀềỄễỆệỊịỌọỐốỒồỖỗỘộỚớỜờỠỡỢợỤụỨứỪừỮữỰựỲỳỴỵỸỹÅ]/;
#An alphabetical char:
our $w=qr/([a-zA-Z]|$special)/;
our $wpos=qr/([a-zA-Z]|$special|\/)/;
#A word/number:
our $wordtokenstrict = qr/$w+((\-|\—)($w)+)*/;
our $wordnumtoken = qr/($w+((\-|\—)($w|\d)+)*|(\d+(\.\d+)?|\d*\.\d+)([eE][-+]?\d+)?)/;
our $wordnumpostoken = qr/($wpos+((\-|\—)($wpos|\d)+)*|(\d+(\.\d+)?|\d*\.\d+)([eE][-+]?\d+)?)/;
our $wordtoken=$wordnumtoken;

#External getters for the word token regexps
sub wordtoken { $wordtoken; }
sub wordtokenstrict { $wordtokenstrict; }
sub wordnumtoken { $wordnumtoken; }
sub wordnumpostoken { $wordnumpostoken; }
sub w {$w;}

sub new {
  my ($class)=@_;
  my $self={};
  bless ($self,$class);
}


# Tokenization adjusted by initial work at:
# (c) 2003-2008 Vlado Keselj http://www.cs.dal.ca/~vlado
# Text::Ngrams - A Perl module for N-grams processing
sub tokenize {
  my ($class,$inref,%options)=@_;
  my $strict=1 if $options{strict};
  if ($strict) {
    $wordtoken=$wordtokenstrict;
  } else {
    if ($options{postags}) {
      $wordtoken = $wordnumpostoken;
    }
    else {
      $wordtoken=$wordnumtoken; 
    }
  }
  my $outref=[];

  while ($$inref=~s/($wordtoken)//) {
    push @$outref, $1;
  }
  undef $inref;
  $outref;
}

sub mergeMath {
  my $inref=$_[1]; #reference to an array words
  my $outref=[];
  my $prev="";
  foreach my $word(@$inref) {
    if (!($prev=~ /MathExprEquation/ && $word =~ /MathExprEquation/)) {
      push @$outref, $word;
      $prev=$word;
    } 
  }
  undef $inref;
  $outref;
}

sub normalize {
# Normalization has no purpose in word tokenization
#  LaMaPUn::Tokenizer::Sentence->normalize($_[1]);
 $_[1];
}

1;

__END__

=pod 

=head1 NAME

C<LaMaPUn::Tokenizer> - Tokenization module for LaMaPUn normalized documents (plain text)

=head1 SYNOPSIS

    use LaMaPUn::Tokenizer;
    my $tokenizer=LaMaPUn::Tokenizer->new(input=>"text",
                                          token=>"sentence", 
                                          mergeMath=>1, 
                                          normalize=>1);
    my $tokensRef = $tokenizer->tokenize_file($inputfile,$outputfile);
    my $tokensRef = $tokenizer->tokenize($some_text);

=head1 DESCRIPTION

=head2 METHODS

=over 4

=item C<< my $preprocessor = LaMaPUn::Preprocessor->new(%options); >>

Creates a new LaMaPUn Tokenizer object for normalized text documents

  input : Admissible "text", "sentence" and "chunk". 
          While "text" can refer to any plain text input, 
          "sentence" and "chunk" requires a single 
          sentence/chunk per input line.

  token : Admissible "sentence", "chunk" or "word", where 
          "chunk" denotes a punctuation-free chunk of text.

  mergeMath : Default is "1". Merges adjacent math blocks.

  normalize : Default is "1". Splits implicit chunks, such as
              "(MathExpr) New sentence..."

=item C<< my $tokensRef = $tokenizer->tokenize_file($inputfile,$outputfile); >>

Reads the text file $inputfile as input and returns a reference to the resulting
array of tokens. If the optional $outputfile is supplied, one token per line
is outputed at the target file.

=item C<< my $tokensRef = $tokenizer->tokenize($some_text); >>

Tokenizes the scalar (or reference) variable on input, returning a reference
to the array result.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
