# Makefile - makefile for vw/src/libc/stdio
#
# modification history
# --------------------
# 01b,12oct01,tam  added repackaging support
# 01a,18jun96,yp   created from 01d of MakeSkel
#
# DESCRIPTION
# This file contains the makefile rules for building the vx library
#
#*/

TGT_DIR=$(WIND_BASE)/target

LIB_BASE_NAME   = os

DOC_FILES=	ansiStdio.c

## ansiStdio.c includes the following files
# 	clearerr.c fclose.c fdopen.c feof.c ferror.c fflush.c 
#	fgetc.c fgetpos.c fgets.c fileno.c fopen.c fprintf.c 
#	fputc.c fputs.c fread.c freopen.c fscanf.c fseek.c 
#	fsetpos.c ftell.c fvwrite.c fwrite.c getc.c getchar.c 
#	gets.c getw.c perror.c putc.c putchar.c puts.c putw.c 
#	rewind.c scanf.c setbuf.c setbuffer.c setvbuf.c stdioLib.c 
#	stdioShow.c tmpfile.c tmpnam.c ungetc.c vfprintf.c


OBJS=	clearerr.o fclose.o fdopen.o feof.o ferror.o fflush.o fgetc.o \
        fgetpos.o fgets.o fileno.o flags.o fopen.o fprintf.o \
        fputc.o fputs.o fread.o freopen.o fscanf.o fseek.o fsetpos.o \
        ftell.o fvwrite.o fwrite.o getc.o getchar.o gets.o getw.o \
	makebuf.o perror.o putc.o putchar.o putw.o puts.o refill.o \
        rewind.o rget.o scanf.o setbuf.o setbuffer.o setvbuf.o \
	stdio.o stdioLib.o stdioShow.o tmpnam.o tmpfile.o ungetc.o \
	wbuf.o wsetup.o vfprintf.o 

include $(TGT_DIR)/h/make/rules.library

