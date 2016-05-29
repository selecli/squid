#! /usr/local/bin/perl 

eval '(exit $?0)' && eval 'exec /usr/local/bin/perl $0 ${1+"$@"}'
&& eval 'exec /usr/local/bin/perl $0 $argv:q'
if 0;

# ============================================================================

# $Id: table.pl,v 1.1.1.1 2006/05/30 16:49:29 alex Exp $

# Copyright (c) 2000-2002 David M. Town <dtown@cpan.org>
# All rights reserved.

# This program is free software; you may redistribute it and/or modify it
# under the same terms as Perl itself.

# ============================================================================

use strict;

use Net::SNMP qw(snmp_dispatcher oid_lex_sort);

# Create the SNMP session 
my ($session, $error) = Net::SNMP->session(
   -hostname  => $ARGV[0] || 'localhost',
   -community => $ARGV[1] || 'public',
   -port      => $ARGV[2] || 161,
   -version   => 'snmpv2c'
);

# Was the session created?
if (!defined($session)) {
   printf("ERROR: %s.\n", $error);
   exit 1;
}

# iso.org.dod.internet.mgmt.interfaces.ifTable
my $ifTable = '1.3.6.1.2.1.2.2';

printf("\n== SNMPv2c blocking get_table(): %s ==\n\n", $ifTable);

my $result;

if (defined($result = $session->get_table(-baseoid => $ifTable))) {
   foreach (oid_lex_sort(keys(%{$result}))) {
      printf("%s => %s\n", $_, $result->{$_});
   }
   print "\n";
} else {
   printf("ERROR: %s.\n\n", $session->error());
}

$session->close;


###
## Now a non-blocking example
###

printf("\n== SNMPv2c non-blocking get_table(): %s ==\n\n", $ifTable); 

# Blocking and non-blocking objects cannot exist at the
# same time.  We must clear the reference to the blocking
# object or the creation of the non-blocking object will
# fail.

$session = undef;

# Create the non-blocking SNMP session
($session, $error) = Net::SNMP->session(
   -hostname    => $ARGV[0] || 'localhost',
   -community   => $ARGV[1] || 'public',
   -port        => $ARGV[2] || 161,
   -nonblocking => 1,
   -version     => 'snmpv2c'
);

# Was the session created?
if (!defined($session)) {
   printf("ERROR: %s.\n", $error);
   exit 1;
}

if (!defined($session->get_table(-baseoid  => $ifTable,
                                 -callback => \&print_results_cb)))
{
   printf("ERROR: %s.\n", $session->error());
}

# Start the event loop
snmp_dispatcher();

print "\n";

exit 0;

sub print_results_cb
{
   my ($session) = @_;

   if (!defined($session->var_bind_list())) {
      printf("ERROR: %s.\n", $session->error());
   } else {
      foreach (oid_lex_sort(keys(%{$session->var_bind_list()}))) {
         printf("%s => %s\n", $_, $session->var_bind_list()->{$_});
      } 
   }
}

# ============================================================================

