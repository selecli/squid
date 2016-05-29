# -*- mode: perl -*- 
# ============================================================================

# $Id: mp.t,v 1.1.1.1 2006/05/30 16:49:29 alex Exp $

# Test of the Message Processing Model. 

# Copyright (c) 2001-2004 David M. Town <dtown@cpan.org>.
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
   plan tests => 7
}

use Net::SNMP::MessageProcessing;
use Net::SNMP::PDU qw( OCTET_STRING SNMP_VERSION_2C );
use Net::SNMP::Security;
use Net::SNMP::Transport;

#
# 1. Get the Message Processing instance 
#

my $m;

eval 
{ 
   $m = Net::SNMP::MessageProcessing->instance; 
}; 

ok($@, '', 'Failed to get Message Processing instance');

#
# 2. Create a Security object
#

my ($s, $e);

eval 
{ 
   ($s, $e) = Net::SNMP::Security->new(-version => SNMP_VERSION_2C); 
};

ok(($@ || $e), '', 'Failed to create Security object');

#
# 3. Create a Transport Layer object
#

my $t;

eval 
{ 
   ($t, $e) = Net::SNMP::Transport->new; 
};

ok(($@ || $e), '', 'Failed to create Transport Layer object'); 

#
# 4. Create a PDU object
#

my $p;

eval 
{ 
   ($p, $e) = Net::SNMP::PDU->new(
      -version   => SNMP_VERSION_2C,
      -transport => $t,
      -security  => $s
   ); 
};

ok(($@ || $e), '', 'Failed to create PDU object');

#
# 5. Prepare the PDU
#

eval 
{ 
   $p->prepare_set_request(['1.3.6.1.2.1.1.4.0', OCTET_STRING, 'dtown']); 
   $e = $p->error;
};

ok(($@ || $e), '', 'Failed to prepare set-request');

#
# 6. Prepare the Message
#

eval 
{
   $p = $m->prepare_outgoing_msg($p);
   $e = $m->error;
};

ok(($@ || $e), '', 'Failed to prepare Message');

#
# 7. Process the message (should get error)
#

eval 
{
   $m->prepare_data_elements($p);
   $e = $m->error;
};

ok(($@ || $e), qr/Expected/, 'Failed to process Message');

# ============================================================================
