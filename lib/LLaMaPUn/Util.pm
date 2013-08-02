# /=====================================================================\ #
# |  LLaMaPUn                                                            | #
# | Utilities Module                                                    | #
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

package LLaMaPUn::Util;
use strict;
use XML::LibXML;
use File::Spec;
use Encode;
use Carp;
use Exporter;
our @ISA = qw(Exporter);
our @EXPORT = qw( &niceCurrentTime &parse_file &splitFilepath &load_text_file &locate_external &locate_resource);

#%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#Utilities:

sub niceCurrentTime {
   my  @months = qw(Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec);
   my  @weekDays = qw(Sun Mon Tue Wed Thu Fri Sat Sun);
   my ($second, $minute, $hour, $dayOfMonth, $month, $yearOffset, $dayOfWeek, $dayOfYear, $daylightSavings) = gmtime();
   my $year = 1900 + $yearOffset;
   my $theGMTime = "$hour:$minute:$second, $months[$month] $dayOfMonth";
   $theGMTime;
}

sub parse_file {
  my ($self,$filepath,%options)=@_;
  $filepath=$self unless (ref $self)||($self =~/LLaMaPUn::Util/);
  $filepath = File::Spec->rel2abs($filepath) 
    if ($filepath && !(File::Spec->file_name_is_absolute($filepath)));
  my $parser=XML::LibXML->new();
  if ($options{warn}) {
    return undef if `xmllint --noout $filepath 2>&1`;
  }
  else {
    croak "Malformed XML in $filepath" if `xmllint --noout $filepath 2>&1`;
  }
  my $doc=$parser->parse_file($filepath);
  $doc;
}

sub load_text_file {
  my ($self,$filepath,%options)=@_;
  $filepath=$self unless (ref $self)||($self =~/LLaMaPUn::Util/);
  $filepath = File::Spec->rel2abs($filepath) 
    if ($filepath && !(File::Spec->file_name_is_absolute($filepath)));
  my $init="";
  my $data=\$init;
  if ($options{warn}) {
    unless (open(DATA,"<$filepath")) {
      carp "File $filepath not accessible for read!\n";
    }
    else {
      $$data=join("",<DATA>);
      close(DATA);
    }
  }
  else {
    unless (open(DATA,"<$filepath")) {
      croak "File $filepath not accessible for read!\n" 
    }
    else {
      $$data=join("",<DATA>);
      close(DATA); 
    }
  }
  $data;
}

sub splitFilepath {
  my ($self,$filepath) = @_;
  $filepath=$self unless (ref $self)||($self =~/LLaMaPUn::Util/);
  $filepath = File::Spec->rel2abs($filepath) 
    if ($filepath && !(File::Spec->file_name_is_absolute($filepath)));

  my @fp=split(//,$filepath);
  my $start="";
  $start=shift @fp if ($fp[0] eq '/');
  my @pieces=split(/\//,join('',@fp));
  my $fname = pop @pieces;
  my $path=$start.join("/",@pieces)."/";
  my ($ext,@extt);
  ($fname,@extt)=split(/\./,$fname);
  $ext=join(".",@extt);
  ($path,$fname,$ext);  
}

sub locate_external {
  my ($self,$external)=@_;
  $external=$self unless (ref $self)||($self =~/LLaMaPUn::Util/);
  $external=~s/^\///g; 
  #New idea: find External dirs using @PATHS!
  my $found=undef;
  foreach my $path(@INC) {
    if (-d "$path/LLaMaPUn/External/$external/") {
      $found="$path/LLaMaPUn/External/$external/";
      last;
    }
  }
  $found = File::Spec->rel2abs($found) 
    if ($found && !(File::Spec->file_name_is_absolute($found)));
  $found=~s/\/$//g;
  $found;
}

sub locate_resource {
  my ($self,$resource)=@_;
  $resource=$self unless (ref $self)||($self =~/LLaMaPUn::Util/);
  $resource=~s/^\///g; 
  #New idea: find Resource dirs using @PATHS!
  my $found=undef;
  foreach my $path(@INC) {
    if (-e "$path/LLaMaPUn/Resources/$resource" || -d "$path/LLaMaPUn/Resources/$resource") {
      $found="$path/LLaMaPUn/Resources/$resource";
      last;
    }
  }
  $found = File::Spec->rel2abs($found) 
    if ($found && !(File::Spec->file_name_is_absolute($found)));
  $found;
}

1;

__END__

=pod 

=head1 NAME

C<LLaMaPUn::Util> - Miscellanious utilities used by the LLaMaPUn module family

=head1 SYNOPSIS

    use LLaMaPUn::Util;
    $time=niceCurrentTime();
    $doc=parse_file($filepath,warn=>1);
    ($name,$path)=splitFilepath($filepath);
    $dirpath=locate_external("External Name");
    $filepath=locate_resource("Resource Name");
    $filecontent = load_text_file($filepath);
    
=head1 DESCRIPTION

Miscellanious utilities used by the LLaMaPUn module family. These include:

- File system and XML API hooks
- Time and (TODO)Debug methods
- (TODO) Multi-thread programming API hooks


=head2 METHODS

=over 4

=item C<< $time=niceCurrentTime(); >>

Returns a visually nice version of the current GMT time

=item C<< $doc=parse_file($filepath,%options); >>

Hook to XML::LibXML. Additionally performs a very important sanity check
regarding the well-formedness of the file using xmllint.

- warn : If warn is set to 1, (default is 0) the return value will be undef.
         If warn is left as 0, Perl will die on malformed input.

=item C<< $filecontent = load_text_file($filepath); >>

Loads the entire body of a text file into memory and returns a pointer to it.

=item C<< ($path,$name,$ext)=splitFilepath($filepath); >>

Given a filepath (absolute or relative) return the separated 
path, name and extension for that path.

=item C<< $dirpath=locate_external("External Name"); >>

Locate the external application given on argument and return its directory path.

=item C<< $filepath=locate_resource("Resource Name"); >>

Locates an external resource given on argument and returns its filepath.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
