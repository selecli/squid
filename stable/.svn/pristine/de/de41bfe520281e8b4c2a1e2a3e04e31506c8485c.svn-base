include ../../Make.properties

MOD_NAME = mod_cpis_body_modify.so

all:clean $(OBJS) 
	$(MOD_LINK) mod_cpis_body_modify.o  -o $(MOD_NAME)
	
install:
	cp -rf *.so ../$(MOD_DIR)

clean:
	rm -rf $(OBJS) *.so

uninstall:
	rm -rf /usr/local/squid/sbin/$(MOD_NAME)	

%.o:	%.c;
#	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -Wall  -D_REENTRANT -Wall -c -fPIC -g -O -I ../../squid/src -I ../../squid/include  $< -o $@

	$(MOD_GCC) $< -o $@

