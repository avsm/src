################################################################################
#
#            !!!!!   Do NOT edit this file directly!   !!!!!
#
#            Edit mktests.PL and/or parts/inc/limits instead.
#
################################################################################

BEGIN {
  if ($ENV{'PERL_CORE'}) {
    chdir 't' if -d 't';
    @INC = ('../lib', '../ext/Devel/PPPort/t') if -d '../lib' && -d '../ext';
    require Config; import Config;
    use vars '%Config';
    if (" $Config{'extensions'} " !~ m[ Devel/PPPort ]) {
      print "1..0 # Skip -- Perl configured without Devel::PPPort module\n";
      exit 0;
    }
  }
  else {
    unshift @INC, 't';
  }

  eval "use Test";
  if ($@) {
    require 'testutil.pl';
    print "1..4\n";
  }
  else {
    plan(tests => 4);
  }
}

use Devel::PPPort;
use strict;
$^W = 1;

ok(&Devel::PPPort::iv_size());
ok(&Devel::PPPort::uv_size());
ok(&Devel::PPPort::iv_type());
ok(&Devel::PPPort::uv_type());

