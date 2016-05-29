#!/usr/bin/perl
$|=1;
while (<>)
{
    `echo "first $_\n" >>/tmp/redi.log`;
	if(/http:\/\/(.*)\/(.*)\?(.*)/) {

        my $first = $1; 
        my $second = $2; 
        my $third = $3; 

        my $redi_ret =  "0 http://$first/$second\n";
        print $redi_ret;
        `echo "2--------- $redi_ret\n" >>/tmp/redi.log`;

    }   
    else{
        print $_; 
        `echo "3--------- $_\n" >>/tmp/redi.log`;
    }   
}
