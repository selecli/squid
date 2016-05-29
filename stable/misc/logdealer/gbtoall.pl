#!/usr/bin/perl
use strict;
use Encode qw(decode encode);
use Encode::HanConvert qw(gb_to_big5 gb_to_trad simp_to_trad gb_to_big5 gb_to_simp);
use bytes;

binmode(STDOUT, ':bytes');

my $filename = $ARGV[0];
if($filename eq "-v")
{
	print "gbtoall V1.2\n";
	exit 0;
}
open FILE,"<",$filename or die "can not open $filename";

while(<FILE>)
{
	chomp;
	#my $decoded=decode("gb2312",$_);
	my $gbsimp=$_;

	my $u8simp = gb_to_simp($gbsimp);
	my $u8trad = simp_to_trad($u8simp);
	#my $decoded=decode("utf-8", $u8trad);
	my $gbtrad=encode("gb2312",$u8trad);

	my $b5trad=gb_to_big5($gbsimp);
	print "$gbsimp\n";
	print "$gbtrad\n";
	print "$u8simp\n";
	print "$u8trad\n";
	print "$b5trad\n";
	#print "$gbsimp\n"

}
