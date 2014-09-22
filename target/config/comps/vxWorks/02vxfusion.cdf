/* 02vxfusion.cdf - component description file for VxFusion */

/* Copyright (c) 1999, Wind River Systems, Inc. */

/* 
modification history
--------------------
01c,18jun99,drm  Changing show rtns parent init rtn to be usrToolsInit().
01b,27may99,drm  changed windmp to vxfusion
01a,24feb99,drm  created.
*/
   

Folder FOLDER_VXFUSION_SHOW_ROUTINES {

	NAME		VxFusion show routines
	SYNOPSIS	enable display of distributed objects
	CHILDREN	INCLUDE_VXFUSION_DIST_MSG_Q_SHOW \
			INCLUDE_VXFUSION_GRP_MSG_Q_SHOW \
			INCLUDE_VXFUSION_DIST_NAME_DB_SHOW \
			INCLUDE_VXFUSION_IF_SHOW
}


Component INCLUDE_VXFUSION_DIST_MSG_Q_SHOW {
	NAME		distributed message queue show routine
	INIT_RTN	msgQDistShowInit ();
	_INIT_ORDER  	usrToolsInit
	MODULES		msgQDistShow.o
	INCLUDE_WHEN 	INCLUDE_VXFUSION INCLUDE_MSG_Q_SHOW
}
	

Component INCLUDE_VXFUSION_GRP_MSG_Q_SHOW {
	NAME		group message queue show routine
	INIT_RTN	msgQDistGrpShowInit ();
	_INIT_ORDER  	usrToolsInit
	MODULES		msgQDistGrpShow.o
}


Component INCLUDE_VXFUSION_DIST_NAME_DB_SHOW {
	NAME		distributed name database show routine
	INIT_RTN	distNameShowInit ();
	_INIT_ORDER  	usrToolsInit
	MODULES		distNameShow.o
}


Component INCLUDE_VXFUSION_IF_SHOW {
	NAME		adapter interface show routine
	INIT_RTN	distIfShowInit ();
	_INIT_ORDER  	usrToolsInit
	MODULES		distIfShow.o
}


Folder FOLDER_VXFUSION {
	NAME 		VxFusion
	SYNOPSIS 	VxFusion Optional Component 
	_CHILDREN	FOLDER_ROOT
	CHILDREN	FOLDER_VXFUSION_SHOW_ROUTINES \
			INCLUDE_VXFUSION
}


Component INCLUDE_VXFUSION {
	NAME		VxFusion
	INIT_RTN	usrVxFusionInit (BOOT_LINE_ADRS);
	MODULES 	distGapLib.o distIfLib.o distIfUdp.o distIncoLib.o \
			distLib.o distNameLib.o distNetLib.o distNodeLib.o \
			distObjLib.o distStatLib.o distTBufLib.o \
			msgQDistGrpLib.o msgQDistLib.o 
	_INIT_ORDER  usrNetworkInit
	CONFIGLETTES usrVxFusion.c
}



