make
cp preloadmgr /usr/local/squid/bin/ -f 
cp mkdir.sh /usr/local/squid/bin/ -f 
make -C ../preload;cp ../preload/preload /usr/local/squid/bin/ -f 
make -C ../objectserver;cp ../objectserver/objsvr /usr/local/squid/bin/ -f 
make -C ../flexiget;cp ../flexiget/flexiget /usr/local/squid/bin/ -f 

