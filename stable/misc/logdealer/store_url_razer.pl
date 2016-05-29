#!/usr/bin/perl
$|=1;
while (<>)
{
        `echo "first $_\n" >>/tmp/redi.log`;
        if(/http:\/\/cfbeta\.razersynapse\.com\/(.*)\?/){
        my $first = $1;
        my $redi_ret = "0 http://cfbeta.razersynapse.com/$first\n";
        print $redi_ret;
        `echo "2--------- $_\n" >>/tmp/redi.log`;
        }
        else{
                print $_;
        `echo "3--------- $_\n" >>/tmp/redi.log`;
        }
}

