use strict;
use warnings;

use Test::More tests => 2;

my $eval_return = eval {
  use LLaMaPUn;
  use LLaMaPUn::Util;
  use LLaMaPUn::Preprocessor;
  use LLaMaPUn::Preprocessor::Purify;
  use LLaMaPUn::Preprocessor::MarkTokens;
  use LLaMaPUn::Tokenizer;
  1;
};

ok($eval_return && !$@, 'LLaMaPUn Modules Loaded successfully.');

my $preprocessor = LLaMaPUn::Preprocessor->new(replacemath=>"position");
ok($preprocessor, 'Can initialize a LLaMaPUn::Preprocessor object');
