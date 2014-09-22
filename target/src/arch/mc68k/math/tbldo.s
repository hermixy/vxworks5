/* tbldo.s - Motorola 68040 FP jump table (EXC) */

/* Copyright 1991-1992 Wind River Systems, Inc. */
	.data
	.globl	_copyright_wind_river
	.long	_copyright_wind_river

/*
modification history
--------------------
01d,23aug92,jcf  changed bxxx to jxx.
01c,26may92,rrr  the tree shuffle
01b,10jan92,kdl  added modification history; general cleanup.
01a,15aug91,kdl  original version, from Motorola FPSP v2.0.
*/

/*
DESCRIPTION


	tbldosa 3.1 12/10/90

 Modified:
	8/16/90	chinds	The table was constructed to use only one level
			of indirection in __x_do_func for monoadic
			functions.  Dyadic functions require two
			levels, and the tables are still contained
			in __x_do_func.  The table is arranged for
			index with a 10-bit index, with the first
			7 bits the opcode, and the remaining 3
			the stag.  For dyadic functions, all
			valid addresses are to the generic entry
			point.


		Copyright (C) Motorola, Inc. 1990
			All Rights Reserved

	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF MOTOROLA
	The copyright notice above does not evidence any
	actual or intended publication of such source code.

TBLDO	idnt    2,1 Motorola 040 Floating Point Software Package

	section	8

NOMANUAL
*/

|	xref	__x_ld_pinf,__x_ld_pone,__x_ld_ppi2
|	xref	__x_t_dz2,__x_t_operr
|	xref	__x_serror,__x_sone,__x_szero,__x_sinf,__x_snzrinx
|	xref	__x_sopr_inf,__x_spi_2,__x_src_nan,__x_szr_inf

|	xref	__x_smovcr
|	xref	__x_pmod,__x_prem,__x_pscale
|	xref	__x_satanh,__x_satanhd
|	xref	__x_sacos,__x_sacosd,__x_sasin,__x_sasind,__x_satan,__x_satand
|	xref	__x_setox,__x_setoxd,__x_setoxm1,__x_setoxm1d,__x_setoxm1i
|	xref	__x_sgetexp,__x_sgetexpd,__x_sgetman,__x_sgetmand
|	xref	__x_sint,__x_sintd,__x_sintrz
|	xref  __x_ssincos,__x_ssincosd,__x_ssincosi,__x_ssincosnan,__x_ssincosz
|	xref	__x_scos,__x_scosd,__x_ssin,__x_ssind,__x_stan,__x_stand
|	xref	__x_scosh,__x_scoshd,__x_ssinh,__x_ssinhd,__x_stanh,__x_stanhd
|	xref	__x_sslog10,__x_sslog2,__x_sslogn,__x_sslognp1
|	xref	__x_sslog10d,__x_sslog2d,__x_sslognd,__x_slognp1d
|	xref	__x_stentox,__x_stentoxd,__x_stwotox,__x_stwotoxd


|	instruction			| opcode-stag Notes
	.globl	__x_tblpre
__x_tblpre:
	.long	__x_smovcr		| 0x00-0 fmovecr all
	.long	__x_smovcr		| 0x00-1 fmovecr all
	.long	__x_smovcr		| 0x00-2 fmovecr all
	.long	__x_smovcr		| 0x00-3 fmovecr all
	.long	__x_smovcr		| 0x00-4 fmovecr all
	.long	__x_smovcr		| 0x00-5 fmovecr all
	.long	__x_smovcr		| 0x00-6 fmovecr all
	.long	__x_smovcr		| 0x00-7 fmovecr all

	.long	__x_sint		| 0x01-0 fint norm
	.long	__x_szero		| 0x01-1 fint zero
	.long	__x_sinf		| 0x01-2 fint inf
	.long	__x_src_nan		| 0x01-3 fint nan
	.long	__x_sintd		| 0x01-4 fint denorm inx
	.long	__x_serror		| 0x01-5 fint ERROR
	.long	__x_serror		| 0x01-6 fint ERROR
	.long	__x_serror		| 0x01-7 fint ERROR

	.long	__x_ssinh		| 0x02-0 fsinh norm
	.long	__x_szero		| 0x02-1 fsinh zero
	.long	__x_sinf		| 0x02-2 fsinh inf
	.long	__x_src_nan		| 0x02-3 fsinh nan
	.long	__x_ssinhd		| 0x02-4 fsinh denorm
	.long	__x_serror		| 0x02-5 fsinh ERROR
	.long	__x_serror		| 0x02-6 fsinh ERROR
	.long	__x_serror		| 0x02-7 fsinh ERROR

	.long	__x_sintrz		| 0x03-0 fintrz norm
	.long	__x_szero		| 0x03-1 fintrz zero
	.long	__x_sinf		| 0x03-2 fintrz inf
	.long	__x_src_nan		| 0x03-3 fintrz nan
	.long	__x_snzrinx		| 0x03-4 fintrz denorm inx
	.long	__x_serror		| 0x03-5 fintrz ERROR
	.long	__x_serror		| 0x03-6 fintrz ERROR
	.long	__x_serror		| 0x03-7 fintrz ERROR
	.long	__x_serror		| 0x04-0 ERROR - illegal extension
	.long	__x_serror		| 0x04-1 ERROR - illegal extension
	.long	__x_serror		| 0x04-2 ERROR - illegal extension
	.long	__x_serror		| 0x04-3 ERROR - illegal extension
	.long	__x_serror		| 0x04-4 ERROR - illegal extension
	.long	__x_serror		| 0x04-5 ERROR - illegal extension
	.long	__x_serror		| 0x04-6 ERROR - illegal extension
	.long	__x_serror		| 0x04-7 ERROR - illegal extension

	.long	__x_serror		| 0x05-0 ERROR - illegal extension
	.long	__x_serror		| 0x05-1 ERROR - illegal extension
	.long	__x_serror		| 0x05-2 ERROR - illegal extension
	.long	__x_serror		| 0x05-3 ERROR - illegal extension
	.long	__x_serror		| 0x05-4 ERROR - illegal extension
	.long	__x_serror		| 0x05-5 ERROR - illegal extension
	.long	__x_serror		| 0x05-6 ERROR - illegal extension
	.long	__x_serror		| 0x05-7 ERROR - illegal extension

	.long	__x_sslognp1		| 0x06-0 flognp1 norm
	.long	__x_szero		| 0x06-1 flognp1 zero
	.long	__x_sopr_inf		| 0x06-2 flognp1 inf
	.long	__x_src_nan		| 0x06-3 flognp1 nan
	.long	__x_slognp1d		| 0x06-4 flognp1 denorm
	.long	__x_serror		| 0x06-5 flognp1 ERROR
	.long	__x_serror		| 0x06-6 flognp1 ERROR
	.long	__x_serror		| 0x06-7 flognp1 ERROR

	.long	__x_serror		| 0x07-0 ERROR - illegal extension
	.long	__x_serror		| 0x07-1 ERROR - illegal extension
	.long	__x_serror		| 0x07-2 ERROR - illegal extension
	.long	__x_serror		| 0x07-3 ERROR - illegal extension
	.long	__x_serror		| 0x07-4 ERROR - illegal extension
	.long	__x_serror		| 0x07-5 ERROR - illegal extension
	.long	__x_serror		| 0x07-6 ERROR - illegal extension
	.long	__x_serror		| 0x07-7 ERROR - illegal extension

	.long	__x_setoxm1		| 0x08-0 fetoxm1 norm
	.long	__x_szero		| 0x08-1 fetoxm1 zero
	.long	__x_setoxm1i		| 0x08-2 fetoxm1 inf
	.long	__x_src_nan		| 0x08-3 fetoxm1 nan
	.long	__x_setoxm1d		| 0x08-4 fetoxm1 denorm
	.long	__x_serror		| 0x08-5 fetoxm1 ERROR
	.long	__x_serror		| 0x08-6 fetoxm1 ERROR
	.long	__x_serror		| 0x08-7 fetoxm1 ERROR

	.long	__x_stanh		| 0x09-0 ftanh norm
	.long	__x_szero		| 0x09-1 ftanh zero
	.long	__x_sone		| 0x09-2 ftanh inf
	.long	__x_src_nan		| 0x09-3 ftanh nan
	.long	__x_stanhd		| 0x09-4 ftanh denorm
	.long	__x_serror		| 0x09-5 ftanh ERROR
	.long	__x_serror		| 0x09-6 ftanh ERROR
	.long	__x_serror		| 0x09-7 ftanh ERROR

	.long	__x_satan		| 0x0a-0 fatan norm
	.long	__x_szero		| 0x0a-1 fatan zero
	.long	__x_spi_2		| 0x0a-2 fatan inf
	.long	__x_src_nan		| 0x0a-3 fatan nan
	.long	__x_satand		| 0x0a-4 fatan denorm
	.long	__x_serror		| 0x0a-5 fatan ERROR
	.long	__x_serror		| 0x0a-6 fatan ERROR
	.long	__x_serror		| 0x0a-7 fatan ERROR

	.long	__x_serror		| 0x0b-0 ERROR - illegal extension
	.long	__x_serror		| 0x0b-1 ERROR - illegal extension
	.long	__x_serror		| 0x0b-2 ERROR - illegal extension
	.long	__x_serror		| 0x0b-3 ERROR - illegal extension
	.long	__x_serror		| 0x0b-4 ERROR - illegal extension
	.long	__x_serror		| 0x0b-5 ERROR - illegal extension
	.long	__x_serror		| 0x0b-6 ERROR - illegal extension
	.long	__x_serror		| 0x0b-7 ERROR - illegal extension

	.long	__x_sasin		| 0x0c-0 fasin norm
	.long	__x_szero		| 0x0c-1 fasin zero
	.long	__x_t_operr		| 0x0c-2 fasin inf
	.long	__x_src_nan		| 0x0c-3 fasin nan
	.long	__x_sasind		| 0x0c-4 fasin denorm
	.long	__x_serror		| 0x0c-5 fasin ERROR
	.long	__x_serror		| 0x0c-6 fasin ERROR
	.long	__x_serror		| 0x0c-7 fasin ERROR

	.long	__x_satanh		| 0x0d-0 fatanh norm
	.long	__x_szero		| 0x0d-1 fatanh zero
	.long	__x_t_operr		| 0x0d-2 fatanh inf
	.long	__x_src_nan		| 0x0d-3 fatanh nan
	.long	__x_satanhd		| 0x0d-4 fatanh denorm
	.long	__x_serror		| 0x0d-5 fatanh ERROR
	.long	__x_serror		| 0x0d-6 fatanh ERROR
	.long	__x_serror		| 0x0d-7 fatanh ERROR

	.long	__x_ssin		| 0x0e-0 fsin norm
	.long	__x_szero		| 0x0e-1 fsin zero
	.long	__x_t_operr		| 0x0e-2 fsin inf
	.long	__x_src_nan		| 0x0e-3 fsin nan
	.long	__x_ssind		| 0x0e-4 fsin denorm
	.long	__x_serror		| 0x0e-5 fsin ERROR
	.long	__x_serror		| 0x0e-6 fsin ERROR
	.long	__x_serror		| 0x0e-7 fsin ERROR

	.long	__x_stan		| 0x0f-0 ftan norm
	.long	__x_szero		| 0x0f-1 ftan zero
	.long	__x_t_operr		| 0x0f-2 ftan inf
	.long	__x_src_nan		| 0x0f-3 ftan nan
	.long	__x_stand		| 0x0f-4 ftan denorm
	.long	__x_serror		| 0x0f-5 ftan ERROR
	.long	__x_serror		| 0x0f-6 ftan ERROR
	.long	__x_serror		| 0x0f-7 ftan ERROR

	.long	__x_setox		| 0x10-0 fetox norm
	.long	__x_ld_pone		| 0x10-1 fetox zero
	.long	__x_szr_inf		| 0x10-2 fetox inf
	.long	__x_src_nan		| 0x10-3 fetox nan
	.long	__x_setoxd		| 0x10-4 fetox denorm
	.long	__x_serror		| 0x10-5 fetox ERROR
	.long	__x_serror		| 0x10-6 fetox ERROR
	.long	__x_serror		| 0x10-7 fetox ERROR

	.long	__x_stwotox		| 0x11-0 ftwotox norm
	.long	__x_ld_pone		| 0x11-1 ftwotox zero
	.long	__x_szr_inf		| 0x11-2 ftwotox inf
	.long	__x_src_nan		| 0x11-3 ftwotox nan
	.long	__x_stwotoxd		| 0x11-4 ftwotox denorm
	.long	__x_serror		| 0x11-5 ftwotox ERROR
	.long	__x_serror		| 0x11-6 ftwotox ERROR
	.long	__x_serror		| 0x11-7 ftwotox ERROR

	.long	__x_stentox		| 0x12-0 ftentox norm
	.long	__x_ld_pone		| 0x12-1 ftentox zero
	.long	__x_szr_inf		| 0x12-2 ftentox inf
	.long	__x_src_nan		| 0x12-3 ftentox nan
	.long	__x_stentoxd		| 0x12-4 ftentox denorm
	.long	__x_serror		| 0x12-5 ftentox ERROR
	.long	__x_serror		| 0x12-6 ftentox ERROR
	.long	__x_serror		| 0x12-7 ftentox ERROR

	.long	__x_serror		| 0x13-0 ERROR - illegal extension
	.long	__x_serror		| 0x13-1 ERROR - illegal extension
	.long	__x_serror		| 0x13-2 ERROR - illegal extension
	.long	__x_serror		| 0x13-3 ERROR - illegal extension
	.long	__x_serror		| 0x13-4 ERROR - illegal extension
	.long	__x_serror		| 0x13-5 ERROR - illegal extension
	.long	__x_serror		| 0x13-6 ERROR - illegal extension
	.long	__x_serror		| 0x13-7 ERROR - illegal extension

	.long	__x_sslogn		| 0x14-0 flogn norm
	.long	__x_t_dz2		| 0x14-1 flogn zero
	.long	__x_sopr_inf		| 0x14-2 flogn inf
	.long	__x_src_nan		| 0x14-3 flogn nan
	.long	__x_sslognd		| 0x14-4 flogn denorm
	.long	__x_serror		| 0x14-5 flogn ERROR
	.long	__x_serror		| 0x14-6 flogn ERROR
	.long	__x_serror		| 0x14-7 flogn ERROR

	.long	__x_sslog10		| 0x15-0 flog10 norm
	.long	__x_t_dz2		| 0x15-1 flog10 zero
	.long	__x_sopr_inf		| 0x15-2 flog10 inf
	.long	__x_src_nan		| 0x15-3 flog10 nan
	.long	__x_sslog10d		| 0x15-4 flog10 denorm
	.long	__x_serror		| 0x15-5 flog10 ERROR
	.long	__x_serror		| 0x15-6 flog10 ERROR
	.long	__x_serror		| 0x15-7 flog10 ERROR

	.long	__x_sslog2		| 0x16-0 flog2 norm
	.long	__x_t_dz2		| 0x16-1 flog2 zero
	.long	__x_sopr_inf		| 0x16-2 flog2 inf
	.long	__x_src_nan		| 0x16-3 flog2 nan
	.long	__x_sslog2d		| 0x16-4 flog2 denorm
	.long	__x_serror		| 0x16-5 flog2 ERROR
	.long	__x_serror		| 0x16-6 flog2 ERROR
	.long	__x_serror		| 0x16-7 flog2 ERROR

	.long	__x_serror		| 0x17-0 ERROR - illegal extension
	.long	__x_serror		| 0x17-1 ERROR - illegal extension
	.long	__x_serror		| 0x17-2 ERROR - illegal extension
	.long	__x_serror		| 0x17-3 ERROR - illegal extension
	.long	__x_serror		| 0x17-4 ERROR - illegal extension
	.long	__x_serror		| 0x17-5 ERROR - illegal extension
	.long	__x_serror		| 0x17-6 ERROR - illegal extension
	.long	__x_serror		| 0x17-7 ERROR - illegal extension

	.long	__x_serror		| 0x18-0 ERROR - illegal extension
	.long	__x_serror		| 0x18-1 ERROR - illegal extension
	.long	__x_serror		| 0x18-2 ERROR - illegal extension
	.long	__x_serror		| 0x18-3 ERROR - illegal extension
	.long	__x_serror		| 0x18-4 ERROR - illegal extension
	.long	__x_serror		| 0x18-5 ERROR - illegal extension
	.long	__x_serror		| 0x18-6 ERROR - illegal extension
	.long	__x_serror		| 0x18-7 ERROR - illegal extension

	.long	__x_scosh		| 0x19-0 fcosh norm
	.long	__x_ld_pone		| 0x19-1 fcosh zero
	.long	__x_ld_pinf		| 0x19-2 fcosh inf
	.long	__x_src_nan		| 0x19-3 fcosh nan
	.long	__x_scoshd		| 0x19-4 fcosh denorm
	.long	__x_serror		| 0x19-5 fcosh ERROR
	.long	__x_serror		| 0x19-6 fcosh ERROR
	.long	__x_serror		| 0x19-7 fcosh ERROR

	.long	__x_serror		| 0x1a-0 ERROR - illegal extension
	.long	__x_serror		| 0x1a-1 ERROR - illegal extension
	.long	__x_serror		| 0x1a-2 ERROR - illegal extension
	.long	__x_serror		| 0x1a-3 ERROR - illegal extension
	.long	__x_serror		| 0x1a-4 ERROR - illegal extension
	.long	__x_serror		| 0x1a-5 ERROR - illegal extension
	.long	__x_serror		| 0x1a-6 ERROR - illegal extension
	.long	__x_serror		| 0x1a-7 ERROR - illegal extension

	.long	__x_serror		| 0x1b-0 ERROR - illegal extension
	.long	__x_serror		| 0x1b-1 ERROR - illegal extension
	.long	__x_serror		| 0x1b-2 ERROR - illegal extension
	.long	__x_serror		| 0x1b-3 ERROR - illegal extension
	.long	__x_serror		| 0x1b-4 ERROR - illegal extension
	.long	__x_serror		| 0x1b-5 ERROR - illegal extension
	.long	__x_serror		| 0x1b-6 ERROR - illegal extension
	.long	__x_serror		| 0x1b-7 ERROR - illegal extension

	.long	__x_sacos		| 0x1c-0 facos norm
	.long	__x_ld_ppi2		| 0x1c-1 facos zero
	.long	__x_t_operr		| 0x1c-2 facos inf
	.long	__x_src_nan		| 0x1c-3 facos nan
	.long	__x_sacosd		| 0x1c-4 facos denorm
	.long	__x_serror		| 0x1c-5 facos ERROR
	.long	__x_serror		| 0x1c-6 facos ERROR
	.long	__x_serror		| 0x1c-7 facos ERROR

	.long	__x_scos		| 0x1d-0 fcos norm
	.long	__x_ld_pone		| 0x1d-1 fcos zero
	.long	__x_t_operr		| 0x1d-2 fcos inf
	.long	__x_src_nan		| 0x1d-3 fcos nan
	.long	__x_scosd		| 0x1d-4 fcos denorm
	.long	__x_serror		| 0x1d-5 fcos ERROR
	.long	__x_serror		| 0x1d-6 fcos ERROR
	.long	__x_serror		| 0x1d-7 fcos ERROR

	.long	__x_sgetexp		| 0x1e-0 fgetexp norm
	.long	__x_szero		| 0x1e-1 fgetexp zero
	.long	__x_t_operr		| 0x1e-2 fgetexp inf
	.long	__x_src_nan		| 0x1e-3 fgetexp nan
	.long	__x_sgetexpd		| 0x1e-4 fgetexp denorm
	.long	__x_serror		| 0x1e-5 fgetexp ERROR
	.long	__x_serror		| 0x1e-6 fgetexp ERROR
	.long	__x_serror		| 0x1e-7 fgetexp ERROR

	.long	__x_sgetman		| 0x1f-0 fgetman norm
	.long	__x_szero		| 0x1f-1 fgetman zero
	.long	__x_t_operr		| 0x1f-2 fgetman inf
	.long	__x_src_nan		| 0x1f-3 fgetman nan
	.long	__x_sgetmand		| 0x1f-4 fgetman denorm
	.long	__x_serror		| 0x1f-5 fgetman ERROR
	.long	__x_serror		| 0x1f-6 fgetman ERROR
	.long	__x_serror		| 0x1f-7 fgetman ERROR

	.long	__x_serror		| 0x20-0 ERROR - illegal extension
	.long	__x_serror		| 0x20-1 ERROR - illegal extension
	.long	__x_serror		| 0x20-2 ERROR - illegal extension
	.long	__x_serror		| 0x20-3 ERROR - illegal extension
	.long	__x_serror		| 0x20-4 ERROR - illegal extension
	.long	__x_serror		| 0x20-5 ERROR - illegal extension
	.long	__x_serror		| 0x20-6 ERROR - illegal extension
	.long	__x_serror		| 0x20-7 ERROR - illegal extension

	.long	__x_pmod		| 0x21-0 fmod all
	.long	__x_pmod		| 0x21-1 fmod all
	.long	__x_pmod		| 0x21-2 fmod all
	.long	__x_pmod		| 0x21-3 fmod all
	.long	__x_pmod		| 0x21-4 fmod all
	.long	__x_serror		| 0x21-5 fmod ERROR
	.long	__x_serror		| 0x21-6 fmod ERROR
	.long	__x_serror		| 0x21-7 fmod ERROR

	.long	__x_serror		| 0x22-0 ERROR - illegal extension
	.long	__x_serror		| 0x22-1 ERROR - illegal extension
	.long	__x_serror		| 0x22-2 ERROR - illegal extension
	.long	__x_serror		| 0x22-3 ERROR - illegal extension
	.long	__x_serror		| 0x22-4 ERROR - illegal extension
	.long	__x_serror		| 0x22-5 ERROR - illegal extension
	.long	__x_serror		| 0x22-6 ERROR - illegal extension
	.long	__x_serror		| 0x22-7 ERROR - illegal extension

	.long	__x_serror		| 0x23-0 ERROR - illegal extension
	.long	__x_serror		| 0x23-1 ERROR - illegal extension
	.long	__x_serror		| 0x23-2 ERROR - illegal extension
	.long	__x_serror		| 0x23-3 ERROR - illegal extension
	.long	__x_serror		| 0x23-4 ERROR - illegal extension
	.long	__x_serror		| 0x23-5 ERROR - illegal extension
	.long	__x_serror		| 0x23-6 ERROR - illegal extension
	.long	__x_serror		| 0x23-7 ERROR - illegal extension

	.long	__x_serror		| 0x24-0 ERROR - illegal extension
	.long	__x_serror		| 0x24-1 ERROR - illegal extension
	.long	__x_serror		| 0x24-2 ERROR - illegal extension
	.long	__x_serror		| 0x24-3 ERROR - illegal extension
	.long	__x_serror		| 0x24-4 ERROR - illegal extension
	.long	__x_serror		| 0x24-5 ERROR - illegal extension
	.long	__x_serror		| 0x24-6 ERROR - illegal extension
	.long	__x_serror		| 0x24-7 ERROR - illegal extension

	.long	__x_prem		| 0x25-0 frem all
	.long	__x_prem		| 0x25-1 frem all
	.long	__x_prem		| 0x25-2 frem all
	.long	__x_prem		| 0x25-3 frem all
	.long	__x_prem		| 0x25-4 frem all
	.long	__x_serror		| 0x25-5 frem ERROR
	.long	__x_serror		| 0x25-6 frem ERROR
	.long	__x_serror		| 0x25-7 frem ERROR

	.long	__x_pscale		| 0x26-0 fscale all
	.long	__x_pscale		| 0x26-1 fscale all
	.long	__x_pscale		| 0x26-2 fscale all
	.long	__x_pscale		| 0x26-3 fscale all
	.long	__x_pscale		| 0x26-4 fscale all
	.long	__x_serror		| 0x26-5 fscale ERROR
	.long	__x_serror		| 0x26-6 fscale ERROR
	.long	__x_serror		| 0x26-7 fscale ERROR

	.long	__x_serror		| 0x27-0 ERROR - illegal extension
	.long	__x_serror		| 0x27-1 ERROR - illegal extension
	.long	__x_serror		| 0x27-2 ERROR - illegal extension
	.long	__x_serror		| 0x27-3 ERROR - illegal extension
	.long	__x_serror		| 0x27-4 ERROR - illegal extension
	.long	__x_serror		| 0x27-5 ERROR - illegal extension
	.long	__x_serror		| 0x27-6 ERROR - illegal extension
	.long	__x_serror		| 0x27-7 ERROR - illegal extension

	.long	__x_serror		| 0x28-0 ERROR - illegal extension
	.long	__x_serror		| 0x28-1 ERROR - illegal extension
	.long	__x_serror		| 0x28-2 ERROR - illegal extension
	.long	__x_serror		| 0x28-3 ERROR - illegal extension
	.long	__x_serror		| 0x28-4 ERROR - illegal extension
	.long	__x_serror		| 0x28-5 ERROR - illegal extension
	.long	__x_serror		| 0x28-6 ERROR - illegal extension
	.long	__x_serror		| 0x28-7 ERROR - illegal extension

	.long	__x_serror		| 0x29-0 ERROR - illegal extension
	.long	__x_serror		| 0x29-1 ERROR - illegal extension
	.long	__x_serror		| 0x29-2 ERROR - illegal extension
	.long	__x_serror		| 0x29-3 ERROR - illegal extension
	.long	__x_serror		| 0x29-4 ERROR - illegal extension
	.long	__x_serror		| 0x29-5 ERROR - illegal extension
	.long	__x_serror		| 0x29-6 ERROR - illegal extension
	.long	__x_serror		| 0x29-7 ERROR - illegal extension

	.long	__x_serror		| 0x2a-0 ERROR - illegal extension
	.long	__x_serror		| 0x2a-1 ERROR - illegal extension
	.long	__x_serror		| 0x2a-2 ERROR - illegal extension
	.long	__x_serror		| 0x2a-3 ERROR - illegal extension
	.long	__x_serror		| 0x2a-4 ERROR - illegal extension
	.long	__x_serror		| 0x2a-5 ERROR - illegal extension
	.long	__x_serror		| 0x2a-6 ERROR - illegal extension
	.long	__x_serror		| 0x2a-7 ERROR - illegal extension

	.long	__x_serror		| 0x2b-0 ERROR - illegal extension
	.long	__x_serror		| 0x2b-1 ERROR - illegal extension
	.long	__x_serror		| 0x2b-2 ERROR - illegal extension
	.long	__x_serror		| 0x2b-3 ERROR - illegal extension
	.long	__x_serror		| 0x2b-4 ERROR - illegal extension
	.long	__x_serror		| 0x2b-5 ERROR - illegal extension
	.long	__x_serror		| 0x2b-6 ERROR - illegal extension
	.long	__x_serror		| 0x2b-7 ERROR - illegal extension

	.long	__x_serror		| 0x2c-0 ERROR - illegal extension
	.long	__x_serror		| 0x2c-1 ERROR - illegal extension
	.long	__x_serror		| 0x2c-2 ERROR - illegal extension
	.long	__x_serror		| 0x2c-3 ERROR - illegal extension
	.long	__x_serror		| 0x2c-4 ERROR - illegal extension
	.long	__x_serror		| 0x2c-5 ERROR - illegal extension
	.long	__x_serror		| 0x2c-6 ERROR - illegal extension
	.long	__x_serror		| 0x2c-7 ERROR - illegal extension

	.long	__x_serror		| 0x2d-0 ERROR - illegal extension
	.long	__x_serror		| 0x2d-1 ERROR - illegal extension
	.long	__x_serror		| 0x2d-2 ERROR - illegal extension
	.long	__x_serror		| 0x2d-3 ERROR - illegal extension
	.long	__x_serror		| 0x2d-4 ERROR - illegal extension
	.long	__x_serror		| 0x2d-5 ERROR - illegal extension
	.long	__x_serror		| 0x2d-6 ERROR - illegal extension
	.long	__x_serror		| 0x2d-7 ERROR - illegal extension

	.long	__x_serror		| 0x2e-0 ERROR - illegal extension
	.long	__x_serror		| 0x2e-1 ERROR - illegal extension
	.long	__x_serror		| 0x2e-2 ERROR - illegal extension
	.long	__x_serror		| 0x2e-3 ERROR - illegal extension
	.long	__x_serror		| 0x2e-4 ERROR - illegal extension
	.long	__x_serror		| 0x2e-5 ERROR - illegal extension
	.long	__x_serror		| 0x2e-6 ERROR - illegal extension
	.long	__x_serror		| 0x2e-7 ERROR - illegal extension

	.long	__x_serror		| 0x2f-0 ERROR - illegal extension
	.long	__x_serror		| 0x2f-1 ERROR - illegal extension
	.long	__x_serror		| 0x2f-2 ERROR - illegal extension
	.long	__x_serror		| 0x2f-3 ERROR - illegal extension
	.long	__x_serror		| 0x2f-4 ERROR - illegal extension
	.long	__x_serror		| 0x2f-5 ERROR - illegal extension
	.long	__x_serror		| 0x2f-6 ERROR - illegal extension
	.long	__x_serror		| 0x2f-7 ERROR - illegal extension

	.long	__x_ssincos		| 0x30-0 fsincos norm
	.long	__x_ssincosz		| 0x30-1 fsincos zero
	.long	__x_ssincosi		| 0x30-2 fsincos inf
	.long	__x_ssincosnan		| 0x30-3 fsincos nan
	.long	__x_ssincosd		| 0x30-4 fsincos denorm
	.long	__x_serror		| 0x30-5 fsincos ERROR
	.long	__x_serror		| 0x30-6 fsincos ERROR
	.long	__x_serror		| 0x30-7 fsincos ERROR

	.long	__x_ssincos		| 0x31-0 fsincos norm
	.long	__x_ssincosz		| 0x31-1 fsincos zero
	.long	__x_ssincosi		| 0x31-2 fsincos inf
	.long	__x_ssincosnan		| 0x31-3 fsincos nan
	.long	__x_ssincosd		| 0x31-4 fsincos denorm
	.long	__x_serror		| 0x31-5 fsincos ERROR
	.long	__x_serror		| 0x31-6 fsincos ERROR
	.long	__x_serror		| 0x31-7 fsincos ERROR

	.long	__x_ssincos		| 0x32-0 fsincos norm
	.long	__x_ssincosz		| 0x32-1 fsincos zero
	.long	__x_ssincosi		| 0x32-2 fsincos inf
	.long	__x_ssincosnan		| 0x32-3 fsincos nan
	.long	__x_ssincosd		| 0x32-4 fsincos denorm
	.long	__x_serror		| 0x32-5 fsincos ERROR
	.long	__x_serror		| 0x32-6 fsincos ERROR
	.long	__x_serror		| 0x32-7 fsincos ERROR

	.long	__x_ssincos		| 0x33-0 fsincos norm
	.long	__x_ssincosz		| 0x33-1 fsincos zero
	.long	__x_ssincosi		| 0x33-2 fsincos inf
	.long	__x_ssincosnan		| 0x33-3 fsincos nan
	.long	__x_ssincosd		| 0x33-4 fsincos denorm
	.long	__x_serror		| 0x33-5 fsincos ERROR
	.long	__x_serror		| 0x33-6 fsincos ERROR
	.long	__x_serror		| 0x33-7 fsincos ERROR

	.long	__x_ssincos		| 0x34-0 fsincos norm
	.long	__x_ssincosz		| 0x34-1 fsincos zero
	.long	__x_ssincosi		| 0x34-2 fsincos inf
	.long	__x_ssincosnan		| 0x34-3 fsincos nan
	.long	__x_ssincosd		| 0x34-4 fsincos denorm
	.long	__x_serror		| 0x34-5 fsincos ERROR
	.long	__x_serror		| 0x34-6 fsincos ERROR
	.long	__x_serror		| 0x34-7 fsincos ERROR

	.long	__x_ssincos		| 0x35-0 fsincos norm
	.long	__x_ssincosz		| 0x35-1 fsincos zero
	.long	__x_ssincosi		| 0x35-2 fsincos inf
	.long	__x_ssincosnan		| 0x35-3 fsincos nan
	.long	__x_ssincosd		| 0x35-4 fsincos denorm
	.long	__x_serror		| 0x35-5 fsincos ERROR
	.long	__x_serror		| 0x35-6 fsincos ERROR
	.long	__x_serror		| 0x35-7 fsincos ERROR

	.long	__x_ssincos		| 0x36-0 fsincos norm
	.long	__x_ssincosz		| 0x36-1 fsincos zero
	.long	__x_ssincosi		| 0x36-2 fsincos inf
	.long	__x_ssincosnan		| 0x36-3 fsincos nan
	.long	__x_ssincosd		| 0x36-4 fsincos denorm
	.long	__x_serror		| 0x36-5 fsincos ERROR
	.long	__x_serror		| 0x36-6 fsincos ERROR
	.long	__x_serror		| 0x36-7 fsincos ERROR

	.long	__x_ssincos		| 0x37-0 fsincos norm
	.long	__x_ssincosz		| 0x37-1 fsincos zero
	.long	__x_ssincosi		| 0x37-2 fsincos inf
	.long	__x_ssincosnan		| 0x37-3 fsincos nan
	.long	__x_ssincosd		| 0x37-4 fsincos denorm
	.long	__x_serror		| 0x37-5 fsincos ERROR
	.long	__x_serror		| 0x37-6 fsincos ERROR
	.long	__x_serror		| 0x37-7 fsincos ERROR

|	end
