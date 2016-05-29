#!/usr/bin/perl
$|=1;
while (<>)
{
    `echo "first $_\n" >>/tmp/redi.log`;
    if(/http:\/\/cnmusic(.*)\.cdnmyspace\.cn\/(.*)\/(.*)\.mp3\?/){

        my $first = $1; 
        my $second = $2; 
        my $third = $3; 

        my $redi_ret =  "0 http://cnmusic$first.cdnmyspace.cn/$second/$third.mp3\n";
        print $redi_ret;
        `echo "2--------- $_\n" >>/tmp/redi.log`;

    }   
    else{
        print $_; 
        `echo "3--------- $_\n" >>/tmp/redi.log`;
    }   
}
