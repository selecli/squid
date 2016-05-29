# -*- mode: perl -*- 
# ============================================================================

# $Id: usm.t,v 1.1.1.1 2006/05/30 16:49:29 alex Exp $

# Test of the SNMPv3 User-based Security Model. 

# Copyright (c) 2001-2005 David M. Town <dtown@cpan.org>.
# All rights reserved.

# This program is free software; you may redistribute it and/or modify it
# under the same terms as Perl itself.

# ============================================================================

use strict;
use Test;

BEGIN
{
   $|  = 1;
   $^W = 1;
   plan tests => 14
}

use Net::SNMP::Message qw(SEQUENCE OCTET_STRING FALSE);

#
# Load the Net::SNMP::Security::USM module
#

eval 'use Net::SNMP::Security::USM';

my $skip = ($@ =~ /locate (\S+\.pm)/) ? $@ : FALSE;

#
# 1. Create the Net::SNMP::Security::USM object
#

my ($u, $e); 

eval 
{ 
   ($u, $e) = Net::SNMP::Security::USM->new(
      -username     => 'dtown',
      -authpassword => 'maplesyrup',
      -privpassword => 'maplesyrup',
      -privprotocol => 'des'
   );

   # "Perform" discovery...
   $u->_engine_id_discovery(pack('x11H2', '02')) if defined($u);

   # ...and synchronization
   $u->_synchronize(10, time()) if defined($u); 
};

skip($skip, ($@ || $e), '', 'Failed to create Net::SNMP::Security::USM object');

#
# 2. Check the localized authKey
#

eval 
{ 
   $e = unpack('H*', $u->auth_key); 
};

skip(
   $skip,
   ($@ || $e), 
   '526f5eed9fcce26f8964c2930787d82b', # RFC 3414 - A.3.1 
   'Invalid authKey calculated'
);

#
# 3. Check the localized privKey
#

eval 
{ 
   $e = unpack('H*', $u->priv_key); 
};

skip(
   $skip,
   ($@ || $e), 
   '526f5eed9fcce26f8964c2930787d82b', 
   'Invalid privKey calculated'
);

#
# 4. Create and initalize a Message
#

my $m;

eval 
{
   ($m, $e) = Net::SNMP::Message->new;
   $m->prepare(SEQUENCE, pack('H*', 'deadbeef') x 8) if defined($m);
   $e = $m->error if defined($m);
};

skip($skip, ($@ || $e), '', 'Failed to create Net::SNMP::Message object');

#
# 5. Calculate the HMAC
#

my $h;

eval 
{ 
   $h = unpack('H*', $u->_auth_hmac($m)); 
};

skip($skip, $@, '', 'Calculate the HMAC failed');

#
# 6. Encrypt/descrypt the Message
#

eval 
{
   my $salt;
   my $len = $m->length;
   my $buff = $m->clear; 
   $m->append($u->_encrypt_data($m, $salt, $buff));
   $u->_decrypt_data($m, $salt, $m->process(OCTET_STRING));
   $e = $u->error;
   # Remove padding if necessary
   substr(${$m->reference}, $len, -$len, '') if ($len -= $m->length); 
};

skip($skip, ($@ || $e), '', 'Privacy failed');

#
# 7. Check the HMAC
#

my $h2;

eval 
{ 
   $h2 = unpack('H*', $u->_auth_hmac($m)); 
};

skip($skip, ($@ || $h2), $h, 'Authentication failed');

#
# 8. Create the Net::SNMP::Security::USM object
#

eval 
{ 
   ($u, $e) = Net::SNMP::Security::USM->new(
      -username     => 'dtown',
      -authpassword => 'maplesyrup',
      -authprotocol => 'sha1',
      -privpassword => 'maplesyrup',
      -privprotocol => 'des'
   );

   # "Perform" discovery...
   $u->_engine_id_discovery(pack('x11H2', '02')) if defined($u);

   # ...and synchronization
   $u->_synchronize(10, time()) if defined($u);
};

skip($skip, ($@ || $e), '', 'Failed to create Net::SNMP::Security::USM object');

#
# 9. Check the localized authKey
#

eval 
{ 
   $e = unpack('H*', $u->auth_key); 
};

skip(
   $skip,
   ($@ || $e), 
   '6695febc9288e36282235fc7151f128497b38f3f', # RFC 3414 - A.3.2 
   'Invalid authKey calculated'
);

#
# 10. Check the localized privKey
#

eval 
{ 
   $e = unpack('H*', $u->priv_key); 
};

skip(
   $skip,
   ($@ || $e), 
   '6695febc9288e36282235fc7151f1284', 
   'Invalid privKey calculated'
);

#
# 11. Create and initalize a Message
#

eval 
{
   ($m, $e) = Net::SNMP::Message->new;
   $m->prepare(SEQUENCE, pack('H*', 'deadbeef') x 8) if defined($m);
   $e = $m->error if defined($m);
};

skip($skip, ($@ || $e), '', 'Failed to create Net::SNMP::Message object');

#
# 12. Calculate the HMAC
#

eval 
{ 
   $h = unpack('H*', $u->_auth_hmac($m)); 
};

skip($skip, $@, '', 'Calculate the HMAC failed');

#
# 13. Encrypt/descrypt the Message
#

eval 
{
   my $salt;
   my $len = $m->length;
   my $buff = $m->clear;
   $m->append($u->_encrypt_data($m, $salt, $buff));
   $u->_decrypt_data($m, $salt, $m->process(OCTET_STRING));
   $e = $u->error;
   # Remove padding if necessary
   substr(${$m->reference}, $len, -$len, '') if ($len -= $m->length);
};

skip($skip, ($@ || $e), '', 'Privacy failed');

#
# 14. Check the HMAC
#

eval 
{ 
   $h2 = unpack('H*', $u->_auth_hmac($m)); 
};

skip($skip, ($@ || $h2), $h, 'Authentication failed');

# ============================================================================
