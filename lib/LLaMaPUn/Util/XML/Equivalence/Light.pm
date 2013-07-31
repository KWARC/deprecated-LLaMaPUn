# /=====================================================================\ #
# |  LLaMaPUn Utilities                                                  | #
# | Generate Equivalence Classes of XML Trees (Light)                   | #
# |=====================================================================| #
# | Part of the LLaMaPUn project: http://kwarc.info/projects/LLaMaPUn/    | #
# |  Research software, produced as part of work done by the            | #
# |  KWARC group at Jacobs University,                                  | #
# | Copyright (c) 2010 LLaMaPUn group                                    | #
# | Released under the GNU Public License                               | #
# |---------------------------------------------------------------------| #
# | Deyan Ginev <d.ginev@jacobs-university.de>                  #_#     | #
# | http://kwarc.info/people/dginev                            (o o)    | #
# \=========================================================ooo==U==ooo=/ #

package LLaMaPUn::Util::XML::Equivalence::Light;
use strict;
use Data::Compare;
#use Data::Dumper;
use vars qw($VERSION);
$VERSION = "0.0.1";

#########################################################################
## Class-level variables

our $eq_attr = 'equivclass';
our $base_attr = 'baseclass';

#########################################################################
## Methods

sub new {
  my($class,%options)=@_;
  my $default = $options{default} // 'include';
  my $inmarkers = {attr=>{}, node=>{}};
  my $exmarkers = {attr=>{}, node=>{}};
  $exmarkers->{attr}->{$base_attr} = 1; #exclude result attributes
  $exmarkers->{attr}->{$eq_attr} = 1; # exclude result attributes
  process_marker($_,$exmarkers) foreach @{$options{ignore}};
  process_marker($_,$inmarkers) foreach @{$options{process}};
  bless {default=>$default,
         ignore=>$exmarkers,
         process=>$inmarkers,
         base_map=>{},
         class_map=>{}}, $class;
}

sub classify {
  my ($self,$tree) = @_;
  my @leaves = $tree->findnodes( '//*[not(*)]' );
  #1. Classify Leaves
  $self->_classify($_) foreach (@leaves);
  #2. Recursively classify parents, until root is reached
  my @p = parents(@leaves);
  while (@p) {
    $self->_classify($_) foreach (@p);
    @p = parents(@p);
  }
}

sub equal {
  my ($self,$tree1,$tree2) = @_;
  return unless defined $tree1 && defined $tree2 
    && $tree1->isa('XML::LibXML::Element') && $tree2->isa('XML::LibXML::Element');
  my $attr1 = $tree1->getAttribute($eq_attr);
  my $attr2 = $tree2->getAttribute($eq_attr);
  $attr1 eq $attr2;
}

sub diff_head {
  my ($self,$tree1,$tree2) = @_;
  return unless defined $tree1 && defined $tree2 
    && $tree1->isa('XML::LibXML::Element') && $tree2->isa('XML::LibXML::Element');
  my $attr1 = $tree1->getAttribute($base_attr);
  my $attr2 = $tree2->getAttribute($base_attr);
  $attr1 ne $attr2;
}

sub diff_children {
  my ($self,$tree1,$tree2) = @_;
  return unless defined $tree1 && defined $tree2 
    && $tree1->isa('XML::LibXML::Element') && $tree2->isa('XML::LibXML::Element');

  my @c1 = map { $_->getAttribute($eq_attr) }
    grep {defined $_ && $_->isa('XML::LibXML::Element')} $tree1->childNodes;
  my @c2 = map { $_->getAttribute($eq_attr) }
    grep {defined $_ && $_->isa('XML::LibXML::Element')} $tree2->childNodes;
  my $diff = 0;
  my @diffpositions;
  return -1 if $#c1 != $#c2;
  for (0..$#c1) {
    if ($c1[$_] ne $c2[$_]) {
      $diff++;
      push @diffpositions, $_ if (wantarray);
    }
  }
  #-- undefined context
  return if (! defined wantarray);
  #-- list context
   return ($diff,@diffpositions) if (wantarray);
  #-- neither undefined nor list context, return scalar
  return $diff;
}

sub autoclean {
  my ($self,$tree) = @_;
  foreach ($tree->findnodes('//*[@'.$base_attr.' or @'.$eq_attr.']')) {
    $_->removeAttribute($base_attr);
    $_->removeAttribute($eq_attr);
  }
}

#########################################################################
## Internal utility functions

sub process_marker {
  my ($s,$hashref) = @_;
  my ($key,$val) = split(/:/,$s);
  $hashref->{$key} = {} unless defined $hashref->{$key};
  $hashref->{$key}->{$val}=1;
}

sub _classify {
  my ($self,$el) = @_;
  return unless defined $el && (!$el->isa('XML::LibXML::Document')); #only defined non-document nodes
  return if defined $el->getAttribute($eq_attr); # only yet-to-be-classified elements
  my $base_class = $self->base_class($el); # establish base class
  my $complex_class = $self->complex_class($el,$base_class); # establish real equivalence class
  $el->setAttribute($base_attr,$base_class); # set base class
  $el->setAttribute($eq_attr,$complex_class); # set equivalence class
}

sub complex_class {
  my ($self,$el,$base_class) = @_;
  # 1. Establish class structure, given base class and children:
  my $child_classes = join(',', map {$_->hasAttributes ? $_->getAttribute($eq_attr)||'0' : '0'} grep defined, $el->childNodes);
  my $struct = $base_class.":".$child_classes;

  # 2. Retrieve or establish equivalence class:
  my $class_map = $self->{'class_map'};
  if (my $class = $class_map->{$struct}) {
    return $class;
  } else {
    $class = scalar(keys %$class_map)+1;
    $class_map->{$struct} = $class;
    return $class;
  }
}

sub base_class {
  my ($self,$el) = @_;
  # 1. Establish base structure, given equivalence markers:
  my $process = $self->{process};
  my $ignore = $self->{ignore};
  my $default = $self->{default};

  # 1.1. Pepare structure hash for given element
  my $struct = {node=>{},attr=>{}};
  # 1.1.1. Mode 1: Include default
  if ($default eq 'include') {
    #Node level
    $struct->{node}->{name} = $el->localName unless $ignore->{node}->{name};
    $struct->{node}->{prefix} = $el->prefix unless $ignore->{node}->{namespace};
    $struct->{node}->{text} = nodeValue_direct($el) unless $ignore->{node}->{text};
    #Attribute level
    foreach ($el->attributes) {
      next unless $_->isa('XML::LibXML::Attr');
      my $name = $_->localName;
      my $val = $_->value;
      next if $ignore->{attr}->{$name};
      $struct->{attr}->{$name} = $val;
    }
  }
  # 1.1.2. Mode 2: Exclude default
  elsif ($default eq 'exclude') {
    #Node level
    $struct->{node}->{name} = $el->localName if $process->{node}->{name};
    $struct->{node}->{prefix} = $el->prefix if $process->{node}->{namespace};
    $struct->{node}->{text} = nodeValue_direct($el) if $process->{node}->{text};
    #Attribute level
    foreach ($el->attributes) {
      next unless $_->isa('XML::LibXML::Attr');
      my $name = $_->localName;
      my $val = $_->value;
      next unless $process->{attr}->{$name};
      $struct->{attr}->{$name} = $val;
    }
  }

  # 2. Retrieve or establish base class:
  my $base_map = $self->{'base_map'};
  my $base_class;
  foreach (keys %$base_map) {
    #2.1. Check if element fits current base class
    if (Compare($base_map->{$_}, $struct))
    #2.2. If yes, terminate and return class label (= hash key)
    { $base_class = $_ ; last; }
    #2.3. If not, continue
  }
  if (!$base_class) {
    #Not found, new class:
    $base_class = scalar(keys %$base_map) + 1;
    $base_map->{$base_class} = $struct;
  }
  $base_class;
}

sub parents {
  grep {defined $_} map {$_->parentNode} grep {defined $_ && $_->isa('XML::LibXML::Node')} @_;
}

sub nodeValue_direct { #Returns the text content of text nodes that are immediate children
 my ($el) = @_;
 join('', map {trim($_->data)} grep {$_->isa('XML::LibXML::Text')} $el->childNodes);}
sub trim {
  my ($string) = @_;
  $string =~ /^\s*(.*)?\s*$/m; #Multi-line removal of leading and trailing spaces.
  return $1; }
##############################
1;
# TODO: Enhance equal and autoclean as additional class-level methods, not only object-specific
__END__

=pod

=head1 NAME

C<LLaMaPUn::Util::XML::Equivalence::Light> - Compute the equivalence classes for all subtrees of a given XML tree.

=head1 SYNOPSIS

    use LLaMaPUn::Util::XML::Equivalence;
    my $eq = LLaMaPUn::Util::XML::Equivalence->new(ignore=>['attr:name', 'element:text', 'element:name'],
                                                  process=>['attr:mode'], default=>'include|exclude');
    $eq->classify($tree1);
    $eq->classify($tree2);
    my $trees_equal = $eq->equal($tree1,$tree2);
    my $head_node_different = $eq->diff_head($tree1,$tree2);
    my ($children_different,@which_ones) = $eq->diff_children($tree1,$tree2);
    $eq->autoclean($tree1);
    $eq->autoclean($tree2);

=head1 DESCRIPTION

Computes the equivalence classes for all subtrees of a given XML tree, 
based on a fully customizable set of markers to determine equality.

 Processing comes in two flavours:
 - LLaMaPUn::Util::XML::Equivalence::Light
   A light-weight pass that only finds the equivalence classes
   (suitable for shallow algorithms, such as Share)

 - LLaMaPUn::Util::XML::Equivalence
   A full-blown analytics extraction providing extra information on level, difference cliques and others.
   (needed for complex algorithms, such as Pack)

=head2 ALGORITHM

=over 4

=item Equivalence Class Generation and Traversal

 Build up equivalence classes of all subtrees in the given XML tree.
 - Datastructure: node, list of children (list is empty for leaves)
 - Algorithm:
   1.1. Traverse all leaves and cache found classes (xml:id <-> class hash).
   1.2. Traverse subtrees on level N (for N=Depth-1 downto 1)
    - Given subtree root eq. class
    - Given ordered list of children eq. classes
    - Construct touple class, consisting of (class_root,list_of<class_child>).

 Complexity, given b (branching factor) and d (depth):
    - Tree traversal (typical): b^(d+1) - 1
    - Lookup in existing classes: ?
    - Overall: O(?)

=item O(1) Comparison of Subtrees

Trivial, via the comparison of the $eq_attr attribute of the two subtrees

=back

=head1 METHODS

=over 4

=item C<< my $eq = LLaMaPUn::Util::XML::Equivalence->new(ignore=>qw(attr:name node:text node:name),
                                                      process=>qw(attr:mode), default=>'include|exclude');>>

Creates a new LLaMaPUn::Util::XML::Equivalence object, specifying the equivalence criteria.
Options:
 - ignore: An array of markers NOT to be considered for establishing equivalence
 - process: Inverse to ignore. An array of markers to be considered for establishing equivalence.
 - default: Default mode of marker treatment; 'exclude' ignores any markers not specified by
            the 'process' option, while 'include' considers all markers not excluded by the 'ignore' option
 - light: Opt for leight-weight processing, only computes equivalence classes without extra information
          such as levels, difference cliques, etc.

Note on conventions: One can specify different XML objects as markers via a key:value syntax.
                     See list of supported keys below.
 - C<attr:foo> - specifies the attribute foo
 - C<node:name> - specifies the local name of the XML element
 - C<node:namespace> - specifies the namespace of the XML element
                       TODO: Currently not handling attribute namespaces
 - C<node:text> - specifies the text content of the XML element.
                     IMPORTANT: Not the conventional textContent() but only content of text nodes
                                that are _direct_children_ of the processed node.

=item C<< $eq->classify($xmltree); >>

Generates Equivalence classes given an XML tree.
Invades the given $tree (a XML::LibXML::Node object) with $eq_attr and $base_attr attributes.

=item C<< my $trees_equal = $eq->equal($xmltree1,$xmltree2); >>

Checks if two trees are equal, based on the $eq_attr attribute.
Note: Assumption of a previous execution of $eq->classify() on both $xmltree1 and $xmltree2.

=item C<< my $head_node_different = $eq->diff_head($xmltree1,$xmltree2); >>

Checks if two trees have equivalent head nodes (i.e. roots for these subtrees)
Note: Assumption of a previous execution of $eq->classify() on both $xmltree1 and $xmltree2.

=item C<< my ($children_different,@which_ones) = $eq->diff_children($xmltree1,$xmltree2); >>

Checks if two trees have equivalent children subtrees. Returns:
  * -1 if the number of children is different in the two trees
  * 0 if all children subtrees are equivalent
  * n, where n>0, if there are n non-equivalent children subtrees

In array context, the module also returns a second output value,
   namely an array of the indices of differing children.

Note: Assumption of a previous execution of $eq->classify() on both $xmltree1 and $xmltree2.

=item C<< $eq->autoclean($tree1); >>

Autocleans the given tree from the $eq_attr and $base_attr attributes.

=back

=head1 INTERNAL ROUTINES

=over 4

=item C<< nodeValue_direct($xmlnode) >>

Returns the text content of all direct text children of the given node.

=item C<< trim($string) >>

Trims the leading and trailing whitespace from a (potentially multi-line) string.

=item C<< process_marker($marker,$hash) >>

Processess a "key:value" marker setting $hash->{$key} = $value.

=item C<< _classify($xmlnode) >>

Internal classification routine, processing a single node, given the so far discovered equivalence classes.

=item C<< base_class($xmlnode) >>

Determine the 'base class' of the current $xmlnode, which entails classifying the node by its tag-related 
properties and attributes.

=item C<< complex_class($xmlnode,$base_class) >>

Determine the 'complex class' of the current $xmlnode, building on the base_class and the complex
classes of the children.

=item C<< parents(@xmlnodes) >>

Return the list of (defined) parents of the given @xmlnodes.

=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
