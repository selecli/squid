include ../../Make.properties

MOD_NAME = mod_zone.so
#MOD_GCC_M = sed -i 's/\->version, \".*\");/\->version, \"$(FC_VERSION).$(SVN_NUM).$(PLATFORM)\");/g' mod_*; gcc $(CFLAGS) -Wall -c -O $(MOD_INCLUDE)
LD = -ldl -lglib-2.0 

ifeq ($(PLATFORM),x86_64)
LDFLAGS = -I /usr/include/glib-2.0/ -I /usr/lib64/glib-2.0/include
else
LDFLAGS = -I /usr/include/glib-2.0/ -I /usr/lib/glib-2.0/include
endif

all:clean $(OBJS) 
	$(MOD_LINK) $(LDFLAGS) $(LD) *.o  -o $(MOD_NAME)

install:
	cp -rf *.so ../$(MOD_DIR)

clean:
	rm -rf $(OBJS) *.so

uninstall:
	rm -rf /usr/local/squid/sbin/$(MOD_NAME)	

%.o:	%.c
	$(MOD_GCC) $(LDFLAGS) $< -o $@
