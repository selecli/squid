#! /usr/local/bin/perl 

eval '(exit $?0)' && eval 'exec /usr/local/bin/perl $0 ${1+"$@"}'
&& eval 'exec /usr/local/bin/perl $0 $argv:q'
if 0;

# ============================================================================

# $Id: snmpset.pl,v 1.1.1.1 2006/05/30 16:49:29 alex Exp $

# Copyright (c) 2000-2005 David M. Town <dtown@cpan.org>
# All rights reserved.

# This program is free software; you may redistribute it and/or modify it
# under the same terms as Perl itself.

# ============================================================================

use Net::SNMP v5.1.0 qw(:asn1 snmp_type_ntop DEBUG_ALL);
use Getopt::Std;

use strict;
use vars qw($SCRIPT $VERSION %OPTS);

$SCRIPT  = 'snmpset';
$VERSION = '2.3.0';

# Validate the command line options
if (!getopts('a:A:c:dD:E:m:n:p:r:t:u:v:x:X:', \%OPTS)) {
   _usage();
}

# Do we have enough information?
if (@ARGV < 4) {
   _usage();
}

# Create the SNMP session
my ($s, $e) = Net::SNMP->session(
   -hostname  => shift,
   exists($OPTS{a}) ? (-authprotocol =>  $OPTS{a}) : (),
   exists($OPTS{A}) ? (-authpassword =>  $OPTS{A}) : (),
   exists($OPTS{c}) ? (-community    =>  $OPTS{c}) : (),
   exists($OPTS{D}) ? (-domain       =>  $OPTS{D}) : (),
   exists($OPTS{d}) ? (-debug        => DEBUG_ALL) : (),
   exists($OPTS{m}) ? (-maxmsgsize   =>  $OPTS{m}) : (),
   exists($OPTS{p}) ? (-port         =>  $OPTS{p}) : (),
   exists($OPTS{P}) ? (-protocol     =>  $OPTS{P}) : (),
   exists($OPTS{r}) ? (-retries      =>  $OPTS{r}) : (),
   exists($OPTS{t}) ? (-timeout      =>  $OPTS{t}) : (),
   exists($OPTS{u}) ? (-username     =>  $OPTS{u}) : (),
   exists($OPTS{v}) ? (-version      =>  $OPTS{v}) : (),
   exists($OPTS{x}) ? (-privprotocol =>  $OPTS{x}) : (),
   exists($OPTS{X}) ? (-privpassword =>  $OPTS{X}) : ()
);

# Was the session created?
if (!defined($s)) {
   _exit($e);
}

# Convert the ASN.1 types to the respresentation expected by Net::SNMP
if (_convert_asn1_types(\@ARGV)) {
   _usage();
}

my @args = (
   exists($OPTS{E}) ? (-contextengineid => $OPTS{E}) : (),
   exists($OPTS{n}) ? (-contextname     => $OPTS{n}) : (),
   -varbindlist    => \@ARGV
);

# Send the SNMP message
if (!defined($s->set_request(@args))) {
   _exit($s->error());
}

# Print the results
foreach ($s->var_bind_names()) {
   printf(
      "%s = %s: %s\n", $_,
      snmp_type_ntop($s->var_bind_types()->{$_}),
      $s->var_bind_list()->{$_},
   );
}

# Close the session
$s->close();
 
exit 0;


# [private] ------------------------------------------------------------------


sub _convert_asn1_types
{
   my ($argv) = @_;

   my %asn1_types = (
      'a' => IPADDRESS,
      'c' => COUNTER32,
      'C' => COUNTER64,
      'g' => GAUGE32,
      'h' => OCTET_STRING,
      'i' => INTEGER32,
      'o' => OBJECT_IDENTIFIER,
      'p' => OPAQUE,
      's' => OCTET_STRING,
      't' => TIMETICKS,
   );

   if ((ref($argv) ne 'ARRAY') || (scalar(@{$argv}) % 3)) {
      return 1;
   }

   for (my $i = 0; $i < scalar(@{$argv}); $i += 3) {
      if (exists($asn1_types{$argv->[$i+1]})) {
         if ($argv->[$i+1] eq 'h') {
            if ($argv->[$i+2] =~ /^(?i:0x)?([0-9a-fA-F]+)$/) {
               $argv->[$i+2] = pack('H*', length($1) % 2 ? '0'.$1 : $1);
            } else {
               _exit("Expected hex string for type 'h'");
            }
         } 
         $argv->[$i+1] = $asn1_types{$argv->[$i+1]};
      } else {
         _exit('Unknown ASN.1 type [%s]', $argv->[$i+1]);
      }
   }

   0; 
}

sub _exit
{
   printf join('', sprintf('%s: ', $SCRIPT), shift(@_), ".\n"), @_;
   exit 1;
}

sub _usage
{
   print << "USAGE";
$SCRIPT v$VERSION
Usage: $SCRIPT [options] <hostname> <oid> <type> <value> [...]
Options: -v 1|2c|3      SNMP version
         -d             Enable debugging
   SNMPv1/SNMPv2c:
         -c <community> Community name
   SNMPv3:
         -u <username>  Username (required)
         -E <engineid>  Context Engine ID
         -n <name>      Context Name
         -a <authproto> Authentication protocol <md5|sha>
         -A <password>  Authentication password
         -x <privproto> Privacy protocol <des|3des|aes>
         -X <password>  Privacy password
   Transport Layer:
         -D <domain>    Domain <udp|udp6|tcp|tcp6>
         -m <octets>    Maximum message size
         -p <port>      Destination port
         -r <attempts>  Number of retries
         -t <secs>      Timeout period
Valid type values:
          a - IpAddress           i - INTEGER
          c - Counter             o - OBJECT IDENTIFIER
          C - Counter64           p - Opaque
          g - Gauge/Unsigned32    s - OCTET STRING          
          h - OCTET STRING (hex)  t - TimeTicks
USAGE
   exit 1;
}

# ============================================================================

