#======================================================================
# Makefile Maker for LLaMaPUn
# d.ginev@jacobs-university.de
#======================================================================
use ExtUtils::MakeMaker;
use strict;
use warnings;

use FindBin;

#======================================================================
# Use "perl Makefile.PL <options>"
#======================================================================
our @EXCLUSIONS=();
our $MORE_MACROS = {};
our $MORE_MAKERULES=''; 

WriteMakefile(
  NAME => 'LLaMaPUn',
  AUTHOR => 'Deyan Ginev <d.ginev@jacobs-university.de>',
  ABSTRACT => "LLaMaPUn is Language and Mathematics Processing and Understanding",
  VERSION_FROM => 'lib/LLaMaPUn.pm',
  PREREQ_PM => { 
    "Lingua::EN::Sentence" => 0,
    'XML::LibXML'=>1.62,
    # 'WordNet::QueryData'=>0, # (Optional, so that we're easy to deploy on custom systems)
    'Scalar::Util'=>0,
    'LaTeXML'=>'0.8.0',
    'File::Slurp'=>0
  },
  (eval { ExtUtils::MakeMaker->VERSION(6.46) } ? (META_MERGE => {
    'meta-spec' => { version => 2 },
      resources => {
        repository => {
          type => 'git',
          url => 'https://github.com/KWARC/LLaMaPUn.git',
          web => 'https://github.com/KWARC/LLaMaPUn',
        }}})
  : ()),
);

