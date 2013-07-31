# /=====================================================================\ #
# |  LLaMaPUn                                                           | #
# | Pack/Unpack XML Trees following the                                 | #
# |       Compact Disjunctive Logical Forms(CDLF) approach              | #
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

package LLaMaPUn::Util::XML::SharedPacked;
use strict;
use LLaMaPUn::Util::XML::Equivalence;

use vars qw($VERSION);
$VERSION = "0.0.1";

#########################################################################
## Class-level variables
our $_namespace = 'http://www.kwarc.info/projects/LLaMaPUn/cdlf/ns#';
our $_prefix = 'cdlf';
our $_disj_el = 'set';
#########################################################################
## Methods

sub new {
  my($class,%options)=@_;
  #Check for an equivalence object, spawn one if none:
  my $equiv = $options{equivalence} // LLaMaPUn::Util::XML::Equivalence->new;
  my $autoclean = $options{autoclean} // 1;
  my $share = $options{share} // 1;
  bless {equivalence => $equiv, autoclean=>$autoclean, share=>$share}, $class;
}

sub compact {
  my ($self,$tree) = @_;
  #1. Embed subtree equivalence classes
  my $eq = $self->{equivalence};
  $eq->classify($tree);
  #2. Compacting loop
  my $state_changed;
  do {
    $state_changed=0;
    my @subtree_set = $tree->childNodes;
    #make into a set/hash and perform clustering analysis on every step?
    # keep in mind need to update equivalences when we add new disjunction elements to reach a normal form (CDNF)
    if(my ($c1,$c2,$recipe) = prescribe_next_pair(@subtree_set)) {
      $c1->unbindNode;
      $c2->unbindNode;
      $tree->appendChild(disjoin($c1,$c2));
      $state_changed=1;
    }
  } while ($state_changed);
  #3. Share classes
  $self->share($tree) if $self->{share};
  #4. Cleanup, if required
  $eq->autoclean($tree) if $self->{autoclean};
}

sub expand {
  my ($self,$tree) = @_;
  #1. Expand $_disj_el sets
  #TODO: Create me
  #2. Unshare classes
  $self->share($tree) if $self->{share};
  #4. Cleanup, if required
  $self->{equivalence}->autoclean($tree) if $self->{autoclean};
}

sub share {
  my ($self,$tree) = @_;
  #TODO: Create me
}

sub unshare {
  my ($self,$tree) = @_;
  #TODO: Create me
}

#########################################################################
## Internal utility functions

sub prescribe_next_pair {
  my @set = @_;
  #TODO: Create me
  
  undef;
}

sub disjoin {
  my ($c1,$c2,$recipe) = @_;
  #TODO: Create me
}


1;
#TODO: Enhance compact and expand as additional class-level methods and not only object-specific
__END__

=pod

=head1 NAME

C<LLaMaPUn::Util::XML::SharedPacked> - Pack/Unpack XML Trees following the
                                Compact Disjunctive Logical Forms(CDLF) approach

=head1 SYNOPSIS

    use LLaMaPUn::Util::XML::Equivalence;
    my $eq = LLaMaPUn::Util::XML::Equivalence->new(ignore=>['attr:name', 'element:text', 'element:name'],
                                              process=>['attr:mode'], default=>'include|exclude');

    use LLaMaPUn::Util::XML::SharedPacked;
    my $packer = LLaMaPUn::Util::XML::SharedPacked->new(equivalence=>$eq,autoclean=>1);
    $packer->compact($tree);
    $packer->epxand($tree);

=head1 DESCRIPTION

=head2 Packed Forests through Top-down topological clique discovery

We call a clique a fully expanded forest packable into a single disjunctive tree. 
The algorithm relies on the topological intuition that each clique has
 the following graph-theoretic properties:
1. Each clique has a uniform branch factor B
2. Hence, each tree in the forest has a root with B children (or degree = B).
3. Let C1, ..., C_B be the sets of possible children for branches 1 ... B.
4. The clique forest must consist of a total of C1 x C2 x .... x C_B trees
5. Each leaf in the forest, at branch i, must have degree C1 x C2 x ... x C_i-1 x C_i+1 x... x C_B.

Hence, the challenges remaining are to locate the best cuts of the given forest into a minimal number of cliques and to rewrite them into a packed representation.

=head2 Tree Compactification Algorithm

The Algorithm assumes a tree with a "fictive" root that holds a set of child subtrees
between which order is irrelevant.
An example of such a setup is a <parses> root element, containing a set of grammar parses as its children.

The goal of the algorithm is to compact the set of children into as packed and as few as possible subtrees.
The order inside the root's children IS important and preserved throughout the processing.
Again, the example with grammatical parses is appropriate - the order of the parse derivation
   corresponds to syntactic or semantic structure.

An important observation is that there are two types of differences that we can pack. There are various ways to interpret them:
 * "Atomic" and "compositional"
 * In XML, "tags" and "subtrees"
 * In parse trees, "categories" and "derivations"

Essentially, we recognize that +@(1,2) and -@(1,2) have an "atomic" difference that is structurally simpler than a "compositional" difference as in -@(1) and -@(1,2). As the atomic packing leaves the trees structurally unchanged, it is a natural first stage for the process, followed by packing subtrees.

1. Atomic top level - while packing can be performed:
 1.1. Find most dense cliques of atomic differences
 1.2. Pack cliques (disjoin diff tags)
2. Compositional top level - while packing can be performed:
 2.1. Find most dense cliques of compositional differences
 2.2. Pack cliques (disjoin diff subtrees)

-- OLD:
 1. Top level loop - repeat while changes occur
  1.1. Disjoin siblings with same children but different roots. (Unbind one if identical.)
  1.2. Disjoin different child of siblings with all but one identical children and identical root elements.
  1.3. Recache and reclassify newly formed disjunctions

 - Normalization TODO notes:  allow optional rearranging of siblings via
    an alphabetical sort on element local name OR values of given attribute OR text content.

=head1 METHODS

=over 4

=item C<< my $packer = LLaMaPUn::Util::XML::SharedPacked->new(equivalence=>$eq,autoclean=>1); >>

Creates a new SharedPacked object with the support for the following options:
 - equivalence: Provide an LLaMaPUn::Util::XML::Equivalence object $eq.
                If omitted, an equivalence object will be spawned with
                no 'ignore' or 'process' specified and the 'include' default.
 - autoclean:   If set to 1 removes the equivalence class attributes internally
                needed for compacting $tree, 0 preserves them. Default is 1.

=item C<< $packer->compact($tree); >>

Compacts a given $tree into a shared-packed form.

=item C<< $packer->expand($tree); >>

Expands a SharedPacked form of a $tree back to a flattened state.

=back

=head1 INTERNAL ROUTINES

=over 4

=item C<< disjoin(@nodelist) >>



=back

=head1 AUTHOR

Deyan Ginev <d.ginev@jacobs-university.de>

=head1 COPYRIGHT

 Research software, produced as part of work done by 
 the KWARC group at Jacobs University Bremen.
 Released under the GNU Public License

=cut
