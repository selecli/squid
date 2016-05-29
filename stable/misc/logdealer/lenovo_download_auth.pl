#!/usr/bin/perl -w

#
# author: xin.yao
# date: 2012-07-26
# developed for lenovo token auth
# -----------------------------------------------------------
# log file rotate: 
# rotate size: MAX_LOGFILE_SIZE
# lenovo_token_auth.log --> lenovo_token_auth.log.bak
#

use strict;
use warnings;
use File::Copy;
use LWP::UserAgent;

my $sign;
my $token;
my $params;
my $pos;
my $sign_pos;
my $token_pos;
my $res_sign;
my $res_filename;
my $request_url;
my $port = 80;
my $auth_domain = "app.lenovomm.com";
my $download_domain = "app.lenovomm.com";
my $auth_url = "http://$auth_domain:$port/verifyserver/verify.do";
my $download_url = "http://$download_domain:$port/dlserver/fileman/DownLoadServlet";
my $logfile = "/var/log/chinacache/lenovo_token_auth.log";
my $MAX_LOGFILE_SIZE = 20480000;

$| = 1;

my $ret = open LOG, ">> $logfile";
if ($ret)
{
	print LOG "--------------------- start lenovo token auth perl script ---------------------\n";
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
	if (($pos = index($request_url, '?')) == -1)
	{
		print "401\n";
		write_log("401,", "Error: url lack '?', request_url=[$request_url]");
		next;
	}
	if (($token_pos = index($request_url, "?t=")) == -1 && ($token_pos = index($request_url, "&t=")) == -1)
	{
		print "401\n";
		write_log("401,", "Error: url lack param [t=], request_url=[$request_url]");
		next;
	}
	if (($sign_pos = rindex($request_url, "?s=") ) == -1 && ($sign_pos = rindex($request_url, "&s=") ) == -1)
	{
		print "401\n";
		write_log("401,", "Error: url lack param [s=], request_url=[$request_url]");
		next;
	}
	$params = substr($request_url, $token_pos + 3);
	$pos = index($params, '&');
	if ($pos == -1)
	{
		$token = substr($params, 0);
	}
	else
	{
		$token = substr($params, 0, $pos);
	}
	$params = substr($request_url, $sign_pos + 3);
	$pos = index($params, '&');
	if ($pos == -1)
	{
		$sign = substr($params, 0);
	}
	else
	{
		$sign = substr($params, 0, $pos);
	}
	if (!defined($token) || $token eq "")
	{
		print "401\n";
		write_log("401,", "Error: url lack param's value for [t=], request_url=[$request_url]");
		next;
	}
	if (!defined($sign) || $sign eq "")
	{
		print "401\n";
		write_log("401,", "Error: url lack param's value for [s=], request_url=[$request_url]");
		next;
	}
#################### here: auth start ####################
	my $method = "GET";
	my $ua = "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1; SV1; .NET CLR 2.0.50727; CIBA)";
	my $authUrl = "$auth_url?t=$token&s=$sign";
	write_log("auth url:", $authUrl);
	my $lwp = new LWP::UserAgent(agent => $ua);
	my $request = HTTP::Request->new($method => $authUrl);
	$request->header(Accept => "*/*");
	$request->header(Host => "$auth_domain");
	my $response = $lwp->request($request);
	write_log("auth response code:", $response->status_line);
	if (!$response->is_success())
	{
		print "400\n";
		write_log("400,", "Warning: auth failed, auth_url=[$authUrl]");
		next;
	}
	my $res_cont = $response->content();
	write_log("auth response content:", $res_cont);
	if (($pos = index($res_cont, "sign=")) == -1)
	{
		print "401\n";
		write_log("401,", "Error: auth response lack [sign=], auth_url=[$authUrl], response=[$res_cont]");
		next;
	}
	$params = substr($res_cont, $pos + 5);
	$pos = index($params, ",");
	if (-1 == $pos)
	{
		$res_sign = substr($params, 0);
	}
	else
	{
		$res_sign = substr($params, 0, $pos);
	}
	if ($res_sign eq "false")
	{
		print "401\n";
		write_log("401,", "Warning: auth response [sign=false], auth_url=[$authUrl], response=[$res_cont]");
		next;
	}
	if (($pos = index($res_cont, "filename=")) == -1)
	{
		print "401\n";
		write_log("401,", "Error: auth response lack [filename=], auth_url=[$authUrl], response=[$res_cont]");
		next;
	}
	$params = substr($res_cont, $pos + 9);
	$pos = index($params, ",");
	if ($pos == -1)
	{
		$res_filename = substr($params, 0);
	}
	else
	{
		$res_filename = substr($params, 0, $pos);
	}
	print "200 $download_url?f=$res_filename\n";
	write_log("200,", "download_url=[$download_url?f=$res_filename]");
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

