#!/usr/bin/perl
$|=1;
while (<>)
{
    #`echo "first $_\n" >>/tmp/redi.log`;
     #if(/(\d+) http:\/\/(p.*\.hbtv\.ccgslb\.com\.cn)\/(.*)\/media_b125000_.*?(\s+\S.*)/){
     if(/(\d+) http:\/\/(p.*\.hbtv\.ccgslb\.com\.cn)\/(.*)\/media_b125000_.*\.abst(.*)/){
	my $num = $1;
        my $first = $2;
        my $second = $3;
        my $third = $4;

        my $redi_ret =  "$num http:///$first/$second/$third\n";
        print $redi_ret;
     #   `echo "2--------- $redi_ret\n" >>/tmp/redi.log`;

    }
    #elsif(/(\d+) (http:\/\/hdcdn\.etiantian\.com\/)[^\/]+\/[^\/]+\/(.*\.mp4).*/i)
    elsif(/(\d+) (http:\/\/hdcdn\.etiantian\.com\/.*\.mp4).*/i)
    {
		my $num = $1;
                my $path = $2;
                my $ret = "$num $path\n";

                print $ret;
      #  `echo "2--------- $ret" >>/tmp/redi.log`;
    }
    else{
        print $_;
      #  `echo "3--------- $_\n" >>/tmp/redi.log`;
    }
}
