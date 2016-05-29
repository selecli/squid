#!/bin/bash

INSTALL_DIR="/usr/local/detectorig"
DETECTORIG_CONF="/usr/local/squid/etc/detectorig.conf"
DOMAIN_CONF="/usr/local/squid/etc/domain.conf"

if [ ! -f $DOMAIN_CONF ] ; then
	cp -f etc/domain.conf $DOMAIN_CONF
	chmod 644 $DOMAIN_CONF
	chown squid.squid $DOMAIN_CONF
fi

if [ ! -f $DETECTORIG_CONF ] ; then
	cp -f etc/detectorig.conf $DETECTORIG_CONF
	chmod 644 $DETECTORIG_CONF
	chown squid.squid $DETECTORIG_CONF
fi

if [ ! -d $INSTALL_DIR ]; then
	mkdir -p $INSTALL_DIR
	chmod 755 $INSTALL_DIR 
fi

if [ ! -d $INSTALL_DIR/sbin ]; then
	mkdir -p $INSTALL_DIR/sbin
	chmod 755 $INSTALL_DIR/sbin
fi

if [ ! -d $INSTALL_DIR/bin ]; then
	mkdir -p $INSTALL_DIR/bin
	chmod 755 $INSTALL_DIR/bin
fi

if [ ! -d $INSTALL_DIR/route_result ]; then
	mkdir -p $INSTALL_DIR/route_result
	chmod 755 $INSTALL_DIR/route_result
fi

if [ ! -d $INSTALL_DIR/module ]; then
	mkdir -p $INSTALL_DIR/module
	chmod 755 $INSTALL_DIR/module
fi

cp -f bin/* $INSTALL_DIR/bin
cp -f sbin/* $INSTALL_DIR/sbin
cp -f module/* $INSTALL_DIR/module
chmod +x $INSTALL_DIR/bin/*
chmod +x $INSTALL_DIR/sbin/*
chmod +x $INSTALL_DIR/module/*
