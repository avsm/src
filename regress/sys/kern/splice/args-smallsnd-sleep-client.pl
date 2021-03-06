# test with smaller relay send buffer delay before client

use strict;
use warnings;

our %args = (
    client => {
	func => sub { sleep 3; write_char(@_); },
	len => 2**17,
    },
    relay => {
	sndbuf => 2**12,
    },
    len => 131072,
    md5 => "31e5ad3d0d2aeb1ad8aaa847dfa665c2",
);

1;
