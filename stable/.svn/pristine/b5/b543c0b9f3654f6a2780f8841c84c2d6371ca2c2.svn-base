include ../../Make.properties
LD = -ldl -lglib-2.0 
ifeq ($(PLATFORM),x86_64)
MOD_NAME = mod_zone_ip2location.so.64
LDFLAGS = -I /usr/include/glib-2.0/ -I /usr/lib64/glib-2.0/include -I /usr/local/include -L /usr/local/lib64 -L /lib64 -lIP2Location
else
MOD_NAME = mod_zone_ip2location.so.32
LDFLAGS = -I /usr/include/glib-2.0/ -I /usr/lib/glib-2.0/include -I /usr/local/include -L /usr/local/lib -L /lib -lIP2Location
endif

all:
	cp -f $(MOD_NAME) mod_zone_ip2location.so 

install:
	cp -rf *.so ../$(MOD_DIR)

clean:
	rm -rf $(OBJS) *.so

uninstall:
	rm -rf /usr/local/squid/sbin/$(MOD_NAME)	

%.o:	%.c
	$(MOD_GCC) $(LDFLAGS) $< -o $@
