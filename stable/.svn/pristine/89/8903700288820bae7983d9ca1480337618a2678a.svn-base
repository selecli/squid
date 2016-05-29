include ../../Make.properties
#FLAG = -g -Wall
SHELL=/bin/sh
INSTALL = /usr/bin/install
ARCH:=$(shell arch)
ifeq ($(ARCH),x86_64)
LDADD = -L./ -lssl -lMyspaceAuthMusic -IMyspaceAuthMusic.h -ldl -lVerify_64 -lstdc++ -lmid_encdec64 
FLAG = -Wall -O2 -DMYSPACE_CHECK -DDUOWAN -DQQ_GREEN_MP3 -DSDO -DIQILU -DBAID -DOUPENG -DNNPAN -DPPTV -DNOKIA -DOOYALA
FLAG_REGEX = -O2
else
LDADD= -L. -lVerify -ldl -lssl -L./ -I. -Xlinker -rpath=./ -lVerify_32 -lmid_encdec32
FLAG = -Wall -O2 -DQQ_MUSIC -DDUOWAN -DQQ_GREEN_MP3 -DSDO -DIQILU -DBAID -DOUPENG -DNNPAN -DPPTV -DNOKIA -DOOYALA
FLAG_REGEX = -O2
endif

DET_PATH = "/usr/local/squid/bin/"
INSTALL = /bin/cp

all: redirect redirect_md5 redirect_three_to_four redirect_produce redirect_SHA256

redirect: redirect.o redirect_main.o md5.o redirect_conf.o oupeng.o ninety_nine.o \
	three_to_four.o regex.o qq_music.o cookie_md5.o qq_topstream.o mafengwo.o pptv.o \
	tudou.o qq_green_mp3.o lighttpd.o rfc1738.o sdo.o iqilu.o sdo_music.o 163music.o baidu_xcode.o bokecc.o wanglong.o \
	microsoft_hmac.o base64_urlencode.o hmac_sha256.o hmac_sha1.o msn.o SHA256.o nokia.o cwg.o ooyala.o longyuan.o
	gcc -o redirect redirect.o redirect_main.o md5.o oupeng.o ninety_nine.o \
	redirect_conf.o three_to_four.o regex.o qq_music.o cookie_md5.o mafengwo.o pptv.o \
	microsoft_hmac.o base64_urlencode.o hmac_sha256.o hmac_sha1.o msn.o SHA256.o \
	qq_topstream.o tudou.o qq_green_mp3.o lighttpd.o rfc1738.o sdo.o iqilu.o \
	sdo_music.o 163music.o baidu_xcode.o bokecc.o wanglong.o nokia.o cwg.o ooyala.o longyuan.o $(LDADD) $(FLAG)

nokia.o:nokia.c
	gcc -c nokia.c $(FLAG)

ooyala.o: ooyala.c
	gcc -c ooyala.c $(FLAG)

longyuan.o: longyuan.c
	gcc -c longyuan.c $(FLAG)

mafengwo.o:mafengwo.c
	gcc -c mafengwo.c $(FLAG)

msn.o: msn.c
	gcc -c msn.c $(FLAG)

SHA256.o: SHA256.c 
	gcc -c SHA256.c $(FLAG)

SHA256_main.o: SHA256_main.c 
	gcc -c SHA256_main.c $(FLAG)

redirect_SHA256: SHA256.o SHA256_main.o 
	gcc -o redirect_SHA256 SHA256.o SHA256_main.o $(FLAG)

hmac_sha256.o: hmac_sha256.c 
	gcc -c hmac_sha256.c $(FLAG)

hmac_sha1.o: hmac_sha1.c 
	gcc -c hmac_sha1.c $(FLAG)
        
base64_urlencode.o: base64_urlencode.c 
	gcc -c base64_urlencode.c $(FALG)
        
microsoft_hmac.o: microsoft_hmac.c 
	gcc -c microsoft_hmac.c $(FLAG)

sdo_music.o:sdo_music.c
	gcc -c sdo_music.c $(FLAG)

oupeng.o:oupeng.c
	gcc -c oupeng.c base64.c $(FLAG)

ninety_nine.o:
	gcc -c ninety_nine.c $(FLAG)

wanglong.o:wanglong.c
	gcc -c wanglong.c $(FLAG)
	
iqilu.o:iqilu.c
	gcc -c iqilu.c $(FLAG)

163music.o:163music.c
	gcc -c 163music.c $(FLAG)

bokecc.o:bokecc.c
	gcc -c bokecc.c $(FLAG)

cwg.o:cwg.c
	gcc -c cwg.c $(FLAG)

sdo.o:sdo.c
	gcc -c sdo.c $(FLAG)

rfc1738.o:rfc1738.c
	gcc -c rfc1738.c $(FLAG)

lighttpd.o:lighttpd.c
	gcc -c lighttpd.c $(FLAG)

baidu_xcode.o:baidu_xcode.c
	gcc -c baidu_xcode.c $(FLAG)

qq_green_mp3.o:qq_green_mp3.c
	gcc -c qq_green_mp3.c $(FLAG)

tudou.o: tudou.c
	gcc -c tudou.c $(FLAG)

qq_topstream.o: qq_topstream.c
	gcc -c qq_topstream.c $(FLAG)

cookie_md5.o: cookie_md5.c cookie_md5.h
	gcc -c cookie_md5.c $(FLAG)

qq_music.o: qq_music.c
	gcc -c qq_music.c $(FLAG)

three_to_four.o: three_to_four.c three_to_four.h
	gcc -c three_to_four.c $(FLAG)

redirect_conf.o: redirect_conf.c redirect_conf.h regex.h
	gcc -c redirect_conf.c $(FLAG)

redirect.o: redirect.c md5.h redirect_conf.h three_to_four.h regex.h
	gcc -c redirect.c $(FLAG)

pptv.o:pptv.c
	gcc -c pptv.c $(FLAG)

redirect_main.o: redirect_main.c
	gcc -c redirect_main.c $(FLAG)

regex.o: regex.c
	gcc -c regex.c $(FLAG_REGEX)

md5.o: md5.c
	gcc -c  md5.c $(FLAG)

redirect_md5: md5.o md5_main.o
	gcc -o redirect_md5 md5.o md5_main.o $(FLAG)

md5_main.o: md5_main.c
	gcc -c md5_main.c $(FLAG)

redirect_three_to_four: three_to_four.o three_to_four_main.o
	gcc -o redirect_three_to_four three_to_four.o three_to_four_main.o $(FLAG)

three_to_four_main.o: three_to_four_main.c
	gcc -c three_to_four_main.c $(FLAG)

redirect_produce: redirect_produce.o md5.o three_to_four.o
	gcc -o redirect_produce redirect_produce.o md5.o three_to_four.o $(FLAG)

redirect_produce.o: redirect_produce.c
	gcc -c redirect_produce.c $(FLAG)

clean:
	@rm -f *.o redirect_produce redirect redirect_three_to_four redirect_md5

install-tar:
#	@$(INSTALL) redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so $(DET_PATH)
	@cp -f redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl storeurl_symbio.pl storeurl_99pan.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so ../$(BIN_DIR)
#	@cp -f redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so ../$(BIN_DIR)
	@echo "redirect install success!"

install-rpm:
#	@$(INSTALL) redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so $(DET_PATH)
	@cp -f redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl storeurl_symbio.pl storeurl_99pan.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so ../$(BIN_DIR)
#	@cp -f redirect redirect_md5 redirect_three_to_four redirect_produce redirect302.pl locationadd.pl libVerifyTS.so libVerifyTS_64.so ../$(BIN_DIR)
	@echo "redirect install success!"

uninstall:
	@rm -f /usr/local/squid/bin/redirect
	@rm -f /usr/local/squid/bin/redirect_md5
	@rm -f /usr/local/squid/bin/redirect_three_to_four
	@rm -f /usr/local/squid/bin/redirect_produce
	@rm -f /usr/local/squid/bin/redirect302.pl
	@rm -f /usr/local/squid/bin/storeurl_symbio.pl
	@rm -f /usr/local/squid/bin/storeurl_99pan.pl
	@rm -f /usr/local/squid/bin/locationadd.pl
	@rm -f /usr/local/squid/etc/redirect.conf
	@rm -f /usr/local/squid/etc/redirect.conf.default
	@rm -f /usr/local/squid/share/doc/redirect.txt
	@rm -f /usr/local/squid/bin/libVerifyTS.so
	@rm -f /usr/local/squid/bin/libVerifyTS_64.so
	@echo "redirect uninstall success!"
