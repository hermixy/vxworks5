# Makefile - makefile for vw/src/rpc
#
# modification history
# --------------------
# 01c,23oct01,tam  updated for re-packaging
# 01b,02aug99,dbt  added support for standalone agent.
# 01a,18jun96,yp   created from 01a of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = rpc

OBJS=	auth_none.o auth_unix.o authunix_pro.o clnt_generic.o clnt_perror.o \
	clnt_raw.o clnt_simple.o clnt_tcp.o clnt_udp.o get_myaddr.o \
	pmap_clnt.o pmap_getmaps.o pmap_getport.o pmap_prot.o pmap_prot2.o \
	pmap_rmt.o portmap.o rpc_callmsg.o rpc_common.o rpc_prot.o svc.o \
	svc_auth.o svc_auth_uni.o svc_raw.o svc_simple.o svc_tcp.o svc_udp.o \
	xdr.o xdr_array.o xdr_float.o xdr_mem.o xdr_rec.o xdr_ref.o

LIBNAMEWDBST=lib$(CPU)$(TOOL)wdbst.a
LIBDIRNAMEWDBST=obj$(CPU)$(TOOL)wdbst

OBJS_WDBST = xdr.o xdr_mem.o xdr_bytes.o xdr_float.o

include $(TGT_DIR)/h/make/rules.library

