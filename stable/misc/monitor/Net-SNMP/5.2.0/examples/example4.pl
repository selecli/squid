#! /usr/local/bin/perl

# ============================================================================

# $Id: example4.pl,v 1.1.1.1 2006/05/30 16:49:29 alex Exp $

# Copyright (c) 2000-2002 David M. Town <david.town@marconi.com>.
# All rights reserved.

# This program is free software; you may redistribute it and/or modify it
# under the same terms as Perl itself.

# ============================================================================

use strict;

use Net::SNMP qw(snmp_dispatcher ticks_to_time);

# List of hosts to poll

my @HOSTS = qw(1.1.1.1 1.1.1.2 localhost);

# Poll interval (in seconds).  This value should be greater 
# than the number of retries plus one, times the timeout value.

my $INTERVAL  = 60;

# Maximum number of polls, including the initial poll.

my $MAX_POLLS = 10;

my $sysUpTime = '1.3.6.1.2.1.1.3.0';

# Create a session for each host and queue the first get-request.

foreach my $host (@HOSTS) {

   my ($session, $error) = Net::SNMP->session(
      -hostname    => $host,
      -nonblocking => 0x1,   # Create non-blocking objects
      -translate   => [
         -timeticks => 0x0   # Turn off so sysUpTime is numeric
      ]  
   );
   if (!defined($session)) {
      printf("ERROR: %s.\n", $error);
      exit 1;
   }

   # Queue the get-request, passing references to variables that
   # will be used to store the last sysUpTime and the number of
   # polls that this session has performed.

   my ($last_uptime, $num_polls) = (0, 0);

   $session->get_request(
       -varbindlist => [$sysUpTime],
       -callback    => [
          \&validate_sysUpTime_cb, \$last_uptime, \$num_polls
       ]
   );

}

# Define a reference point for all of the polls
my $EPOC = time();

# Enter the event loop
snmp_dispatcher();

exit 0;

sub validate_sysUpTime_cb
{
   my ($session, $last_uptime, $num_polls) = @_;


   if (!defined($session->var_bind_list)) {

      printf("%-15s  ERROR: %s\n", $session->hostname, $session->error);

   } else {


      # Validate the sysUpTime

      my $uptime = $session->var_bind_list->{$sysUpTime};

      if ($uptime < ${$last_uptime}) {
         printf("%-15s  WARNING: %s is less than %s\n",
            $session->hostname, 
            ticks_to_time($uptime), 
            ticks_to_time(${$last_uptime})
         );
      } else {
         printf("%-15s  Ok (%s)\n", 
            $session->hostname, ticks_to_time($uptime)
         );
      }

      # Store the new sysUpTime
      ${$last_uptime} = $uptime;

   }

   # Queue the next message if we have not reached $MAX_POLLS.  
   # Since we do not provide a -callback argument, the same 
   # callback and it's original arguments will be used.

   if (++${$num_polls} < $MAX_POLLS) {
      my $delay = (($INTERVAL * ${$num_polls}) + $EPOC) - time();
      $session->get_request(
         -delay       => ($delay >= 0) ? $delay : 0,
         -varbindlist => [$sysUpTime]
      );
   }

   $session->error_status;
}
