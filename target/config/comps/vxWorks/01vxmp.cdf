/*
Copyright 1998-2002 Wind River Systems, Inc.


modification history
--------------------
01b,11mar02,mas  added default for SM_OBJ_MEM_ADRS (SPR 73371)
01a,12nov01,mas  written based on 00vxWorks.cdf: 02v,05nov01,gls


DESCRIPTION
  This file contains the description for the optional VxWorks product VxMP.
*/


Component INCLUDE_SM_OBJ {
	NAME		shared memory objects
	CONFIGLETTES	usrSmObj.c
	MODULES		smObjLib.o
	INIT_RTN	usrSmObjInit (BOOT_LINE_ADRS);
	_INIT_ORDER	usrKernelExtraInit
	INIT_AFTER	INCLUDE_SM_COMMON
	_CHILDREN	FOLDER_KERNEL
	REQUIRES	INCLUDE_SM_COMMON
	CFG_PARAMS	SM_OBJ_MEM_ADRS		\
			SM_OBJ_MAX_TASK		\
			SM_OBJ_MAX_SEM		\
			SM_OBJ_MAX_MSG_Q	\
			SM_OBJ_MAX_MEM_PART	\
			SM_OBJ_MAX_NAME		\
			SM_OBJ_MAX_TRIES
	HDR_FILES	sysLib.h stdio.h stdlib.h private/funcBindP.h \
			smLib.h smObjLib.h smPktLib.h cacheLib.h \
			vxLib.h string.h usrConfig.h
}


Parameter SM_OBJ_MEM_ADRS {
	NAME		shared memory object base address, NONE = allocate
	DEFAULT		NONE
}

Parameter SM_OBJ_MAX_TASK {
	NAME		max # of tasks using smObj
	TYPE		uint
	DEFAULT		40
}

Parameter SM_OBJ_MAX_SEM {
	NAME		max # of shared semaphores
	TYPE		uint
	DEFAULT		60
}

Parameter SM_OBJ_MAX_MSG_Q {
	NAME		max # of shared message queues
	TYPE		uint
	DEFAULT		10
}

Parameter SM_OBJ_MAX_MEM_PART {
	NAME		max # of shared memory partitions
	TYPE		uint
	DEFAULT		4
}

Parameter SM_OBJ_MAX_NAME {
	NAME		max # of shared objects names
	TYPE		uint
	DEFAULT		100
}

Parameter SM_OBJ_MAX_TRIES {
	NAME		max # of tries to obtain lock
	TYPE		uint
	DEFAULT		5000
}

