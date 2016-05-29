#!/usr/bin/env perl
use Socket;
use Thread;
$prefix = "/usr/local/detectorig/route_result/link_route_data";
$port = 1028; #监听端口
$max_begin_sleep = 1; #开始wget之前随机等待的最长时间
$timeout = 5; #wget超时
$interval = 1; #wget失败后，间隔多久重试一次

%fetch = ();
open( file_handle, "/usr/local/squid/etc/domain.conf" );
while( $line = <file_handle> ){
    %ip=();
    next unless $line =~ /^[a-zA-Z0-9]/;
    @all = split(/\s+/, $line);
    next unless @all[22];
    foreach $elem( split( /\|/, @all[9] ) ) {
        $ip{$elem} = 1 if $elem !~ /no/i
    }
    foreach $elem( split( /\|/, @all[12] ) ) {
        $ip{$elem} = 1;
    }
    foreach $elem( split( /\|/, @all[22] ) ) {
        $key = ( split( /:/, $elem ) )[0];
        $ip{$key} = 0;
    }
    while( ($key, $value) = each( %ip ) ){
        $fetch{$key} = 1 if $value == 1;
    }
}
close( file_handle );

srand( time );
@threads = ();
$count = 0;
while( ( $ip, $value ) = each( %fetch ) ) {
    next unless $value == 1;
    $threads[$count] = Thread->new(\&download, $ip );
    $count++;
}
foreach $thread(@threads) {
    $thread->join();
}

sub download{
    sleep rand $max_begin_sleep;
    $ip_num = unpack( 'L', inet_aton( @_ ) );
    $local_file = sprintf( "%s.%s", $prefix, $ip_num );
    $local_file_temp = sprintf( "%s.tmp", $local_file );
    $cmd = sprintf( "wget --timeout=%u -qO %s http://%s:%u/", $timeout, $local_file_temp, @_, $port );
    print $cmd,"\n";
    $ret = system $cmd;
    if( $ret != 0 ){
        sleep $interval;
        print $cmd, "\n";
        $ret = system $cmd;
    }
    print "mv ", $local_file_temp, " ", $local_file, "\n";
    rename( $local_file_temp, $local_file );
}
