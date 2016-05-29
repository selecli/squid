#!/usr/bin/perl -w

#
# author: xin.yao
# date: 2012-07-30
# developed for symbio token auth
# ----------------------------------------------------------
# log file rotate:
# rotate size: MAX_LOGFILE_SIZE
# symbio_token_auth.log --> symbio_token_auth.log.bak
#

use strict;
use warnings;
use File::Copy;
use LWP::UserAgent;

my $uri;
my $pos;
my $token;
my $params;
my $token_pos;
my $request_url;
my $port_auth = 80;
my $port_download = 8080;
my $auth_domain = "app.gli.gsm.pku.edu.cn";
my $download_domain = "download.gli.gsm.pku.edu.cn";
my $auth_url = "http://$auth_domain:$port_auth/ChkToken.aspx";
my $download_url = "http://$download_domain:$port_download";
my $logfile = "/var/log/chinacache/symbio_token_auth.log";
my $MAX_LOGFILE_SIZE = 20480000;

$| = 1;

#system("chown squid:squid $logfile");
my $ret = open LOG, ">> $logfile";
if ($ret)
{
	print LOG "--------------------- start symabio token auth perl script ---------------------\n";
	close LOG;
}

while (<STDIN>)
{
#################### here: parse url start ####################
	$request_url = $_;
	chomp($request_url);
	write_log("original url:", $request_url);
	my $begin = index($request_url, 'http://');
	if ($begin == -1) 
	{   
		print "401\n";
		write_log("401,", "Error: url format is wrong, request_url=[$request_url]");
		next;
	}   
	if ($begin > 0)
	{
		$request_url = substr($request_url, $begin);
	}
	if (!defined($request_url) || $request_url eq "")
	{
		print "401\n";
		write_log("401,", "Error: url is NULL");
		next;
	}
	my $space = index($request_url, ' ');
	if ($space != -1 && $space > 0)
	{
		$request_url = substr($request_url, 0, $space);
	}
	if (-1 == ($token_pos = index($request_url, "?token=")) && -1 == ($token_pos = index($request_url, "&token=")))
	{
		print "401\n";
		write_log("401,", "Error: url lack param [token=], request_url=[$request_url]");
		next;
	}
	$params = substr($request_url, $token_pos + 7);
	$pos = index($params, '&');
	if ($pos == -1)
	{
		$token = substr($params, 0);
	}
	else
	{
		$token = substr($params, 0, $pos);
	}
	if (!defined($token) || $token eq "")
	{
		print "401\n";
		write_log("401,", "Error: url lack param's value for [token=], request_url=[$request_url]");
		next;
	}
	$pos = index($request_url, '?');
	if (-1 == $pos)
	{
		print "401\n";
		write_log("401,", "url lack '?', request_url=[$request_url]");
		next;
	}
# 7 --> length of "http://";
	my $tmp = substr($request_url, 7);
	my $uri_begin = index($tmp, '/');
	if (-1 == $uri_begin)
	{
		$uri = "";
	}
	else
	{
		$uri = substr($tmp, $uri_begin, $pos - $uri_begin - 7);
	}
	write_log("uri:", $uri);
	my $method = "GET";
	my $ua = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; CIBA)";
	my $authUrl = "$auth_url?token=$token";
	write_log("auth url:", $authUrl);
	my $lwp = new LWP::UserAgent(agent => $ua);
	my $request = HTTP::Request->new($method => $authUrl);
	$request->header(Accept => "*/*");
	$request->header(Host => "$auth_domain");
	my $response = $lwp->request($request);
	write_log("auth response code:", $response->status_line);
	if (!$response->is_success())
	{   
		print "401\n";
		write_log("401,", "Warning: auth failed, auth_url=[$authUrl]");
		next;
	}   
	my $res_cont = $response->content();
	write_log("auth response content:", $res_cont);
	if ($res_cont ne "1")
	{
		print "401\n";
		write_log("401,", "Warning: auth response [0], auth_url=[$authUrl], response=[$res_cont]");
		next;
	}
	print "200 $download_url$uri?token=$token\n";
	write_log("200,", "download_url=[$download_url$uri?token=$token]");
}

sub write_log
{
	my @filestat = stat($logfile);
	if ($filestat[7] > $MAX_LOGFILE_SIZE)
	{
		rename($logfile, $logfile.".bak");
	}
	my $success = open LOG, ">> $logfile";
	if (!$success)
	{
		return;
	}
	(my $sec, my $min, my $hour, my $mday, my $mon, my $year) = localtime(time);
	my $yyyymmdd = sprintf("%04d.%02d.%02d", $year + 1900, $mon +1, $mday);
	my $hhmmss = sprintf("%02d:%02d:%02d", $hour, $min, $sec);
	print LOG "$yyyymmdd $hhmmss| $_[0] $_[1]\n";
	close LOG;
}

