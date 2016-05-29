#include ../../Make.properties

#--
#--- common makefile section --
#--
SHELL = /bin/sh
PERL = /usr/bin/perl
CHMOD = chmod
CHOWN = chown
CP = cp
CP_F = cp -f
CP_RF = cp -rf
CD = cd
MV = mv
NOOP = $(SHELL) -c true
NOECHO = @
RM_F = rm -f
RM_RF = rm -rf
TEST_F = test -f
TOUCH = touch
MKDIR = mkdir

#--
#--- monitor's property ---
#--
PERLDIR = ./Net-SNMP/5.2.0
MON = ./mon
PERLMAKEFILE = Makefile.PL
MAKE = make -s
TEST = test
INSTALL = install
INSTALLDIR = /monitor
SOURCEFILE = ./mon/*
SNMPCONF = /etc/snmp/snmpd.conf
SNMPCONF_BAK = /etc/snmp/snmpd.conf_bak
SNMPCONF_NEW = /monitor/share/snmpd.conf
SERVICE = /sbin/service
SNMPD = snmpd
RESTART = restart
UNINSTALL = uninstall

all:
	#///////////////////////////////////////////////
	@echo Compiling source file...
	#///////////////////////////////////////////////
	@$(CD) $(PERLDIR)/ &&  $(PERL) $(PERLMAKEFILE)
	@$(CD) $(PERLDIR)/ &&  $(MAKE) 
	#@$(CD) $(PERLDIR)/ &&  $(MAKE) $(TEST)
	@$(CD) $(PERLDIR)/ &&  $(MAKE) $(INSTALL)	
	@$(CD) $(MON)/src && $(MAKE)

install:
#copy monitor programs
	#///////////////////////////////////////////////
        #      (2)   install monitor 
        #///////////////////////////////////////////////
	-test -d $(INSTALLDIR)/||$(MKDIR) $(INSTALLDIR) 
	-$(CP_RF) $(SOURCEFILE) $(INSTALLDIR)
	-$(CHOWN) root:root $(INSTALLDIR)/bin/*
	-$(CHOWN) squid:root -R $(INSTALLDIR)/pool
	-$(CHMOD) +x $(INSTALLDIR)/bin/*

#config snmp server
        #///////////////////////////////////////////////
        #      (3)   config snmp server
        #///////////////////////////////////////////////
	-test -d /etc/snmp || mkdir -p /etc/snmp
	-test -f $(SNMPCONF)&& mv -f $(SNMPCONF) $(SNMPCONF_BAK) 
	-$(CP_F)  $(SNMPCONF_NEW) $(SNMPCONF)

clean:
	@$(CD) $(MON)/src && $(MAKE) cleanobj
	
uninstall:
	#************** remove monitor programs *************
	-$(RM_RF) $(INSTALLDIR)
	#**************** restore snmp config ************
	-test -f $(SNMPCONF_BAK) && mv -f $(SNMPCONF_BAK) $(SNMPCONF)
	@$(SERVICE) $(SNMPD) $(RESTART)
	@$(CD) $(MON)/src && $(MAKE) cleanall
