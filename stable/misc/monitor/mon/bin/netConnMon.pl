#!/usr/bin/perl

#************************************
#Desc:   Read system connection count
#Author: xinghua.wang@chinacache.com
#Date :  2008-07-09
#************************************

use strict;
use warnings;

our $VERSION = 1.4;

my $option = shift;

if ( defined $option ){
    if ( $option eq '-v' ){
        print "netConnMon Version: $VERSION\n";
    }
    else{
        print qq(Use "netConnMon -v" for version number.\n);
        print qq(Use "netConnMon -h" for help.\n);
    }

    exit;
}

open FH, '/proc/net/snmp' or die "open file failed: $!";

while( <FH> ){
    # find tcp statis
    last if ( /^Tcp:/ );
}

my @field = split;
my ($index) = grep {$field[$_] eq 'CurrEstab'} 0..$#field;
$_ = <FH>;
my @value = split;
my $netConnCount = $value[ $index ];

open FH, '>../pool/netConnMon.pool' or die "open file failed: $!";
print FH "netConnCount: $netConnCount\n";
print FH "netConnUpdateTime: ", time, "\n";
close FH;
