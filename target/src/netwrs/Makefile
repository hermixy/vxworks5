# Makefile - makefile for vw/src/netwrs
#
# modification history
# --------------------
# 02a,28jun02,rae  Remove one more Router Stack specific file
# 01z,20jun02,rae  Remove Router Stack specific files
# 01y,17dec01,vvv  removed rarpLib.c from doc build
# 01x,14nov01,tcr  add INCLUDE_WVNET
# 01w,24oct01,rae  add back etherLib.o
# 01v,15oct01,rae  merge from truestack ver 02f, base 01q
# 01v,08aug01,mem  New runtime arrangement support.
# 01u,05jun01,rae  added dhcpcCommonLib to DOC_FILES
# 01t,07feb01,spm  merged from version 01s of tor2_0_x branch (base 01q):
#                  renamed telnetLib.o during general overhaul (SPR #28675)
# 01s,05may99,pul  removed ADDED_CFLAGS 
# 01r,29apr99,pul  Upgraded NPT phase3 code to tor2.0.0
# 01q,21oct98,n_s  removed mbufLib.c, mbufSockLib.c, and bsdSockLib.c 
#                  from DOC_FILES
# 01p,06oct97,spm  added man pages for DHCP server and relay agent
# 01o,25sep97,gnn  changed ipEtherProto.* to ipProto.*
# 01n,27aug97,spm  removed dhcpcCommonLib.c from DOC_FILES (no user routines)
# 01m,26aug97,spm  renamed dhcpShow to dhcpcShow
# 01l,12aug97,gnn  changes necessitated by MUX/END update.
# 01k,11aug97,vin  added netBufLib.o
# 01j,15jul97,spm  added files for SNTP client and server
# 01i,02jul97,vin  added routeSockLib.o
# 01h,28apr97,gnn  removed the now defunct nullProto.o 
# 01g,17apr97,vin  added ipFilterLib.o
# 01f,17apr97,vin  added mCastRouteLib.o, igmpShow.o icmpShow.o tcpShow.o
#		   and udpShow.o		
# 01e,14apr97,spm  renamed dhcpShowLib.c to dhcpShow.c for tech pubs tools
# 01d,07apr97,spm  added files for DHCP server and relay agent
# 01c,07apr97,gnn  removed endTestProto.o to a new location in drv/end/test
# 01b,28feb97,vin  sorted and merged bsd44 to tor1_0_1
# 01a,18jun96,yp   created from 01i of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME= 	net

DOC_FILES=	arpLib.c bootpLib.c dhcpcCommonLib.c \
                dhcpcShow.c dhcpcLib.c dhcpcBootLib.c dhcpsLib.c dhcprLib.c \
                ftpLib.c ftpdLib.c hostLib.c \
		ifLib.c inetLib.c  mountLib.c netDrv.c netLib.c \
		netShow.c nfsdLib.c nfsDrv.c \
		nfsLib.c proxyArpLib.c proxyLib.c remLib.c rlogLib.c \
		routeLib.c routeEntryLib.c routeMessageLib.c rpcLib.c \
		sockLib.c sntpcLib.c sntpsLib.c \
                telnetdLib.c tftpLib.c tftpdLib.c \
		m2Lib.c m2SysLib.c m2IfLib.c m2IpLib.c m2IcmpLib.c m2UdpLib.c \
		m2TcpLib.c m2Igmp.c zbufLib.c zbufSockLib.c \
		rdiscLib.c pingLib.c \
		etherMultiLib.c muxLib.c muxTkLib.c ipProto.c igmpShow.c icmpShow.c \
		tcpShow.c udpShow.c ipFilterLib.c netBufLib.c ifIndexLib.c wvNetLib.c

OBJS=	arpLib.o bootpLib.o ipProto.o bsdSockLib.o \
        dhcpcShow.o dhcpcBootLib.o dhcpcCommonLib.o dhcpcLib.o \
        rarpLib.o rdiscLib.o dhcprLib.o dhcpsLib.o \
        etherLib.o etherMultiLib.o ftpLib.o ftpdLib.o \
        hostLib.o icmpLib.o ifLib.o igmpLib.o inetLib.o ipLib.o m2IcmpLib.o \
	m2IfLib.o m2IpLib.o m2Lib.o m2SysLib.o m2TcpLib.o m2UdpLib.o \
	mbufLib.o mbufSockLib.o mountLib.o muxLib.o \
	muxTkLib.o netDrv.o netLib.o \
	netShow.o nfsDrv.o nfsHash.o nfsLib.o nfsdLib.o pingLib.o proxyArpLib.o \
	proxyLib.o remLib.o rlogLib.o routeLib.o \
        routeCommonLib.o  routeUtilLib.o \
        rpcLib.o sockLib.o \
	sntpcLib.o sntpsLib.o tcpLib.o telnetdLib.o tftpLib.o tftpdLib.o \
        udpLib.o xdr_bool_t.o xdr_nfs.o xdr_nfsserv.o zbufLib.o zbufSockLib.o \
	mCastRouteLib.o igmpShow.o icmpShow.o tcpShow.o udpShow.o \
	ipFilterLib.o routeSockLib.o netBufLib.o ifIndexLib.o \
	sslLib.o wvNetLib.o nfsHash.o

EXTRA_DEFINE= -DINCLUDE_WVNET

LIBNAMEWDBST=lib$(CPU)$(TOOL)wdbst.a
LIBDIRNAMEWDBST=obj$(CPU)$(TOOL)wdbst

OBJS_WDBST = inetLib.o netBufLib.o muxLib.o muxTkLib.o etherMultiLib.o

include $(TGT_DIR)/h/make/rules.library

