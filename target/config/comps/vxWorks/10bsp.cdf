/*
Copyright 1998-2002 Wind River Systems, Inc.


modification history
--------------------
01c,09jul02,jkf  SPR#78374, assume only one dos volume on ctrl=0 drive=0,
                 ATA_DEV_NAMES_0 is DOSFS_NAMES_ATA_PRIMARY_MASTER, etc
01b,09nov01,jac  Added ATA_DEV_NAMES_2 and ATA_DEV_NAMES_3 to define devices
                 on secondary ATA controller.  Added support for defining
                 cdromFs as file system for an ATA target device.
01a,14oct98,lrn  written


DESCRIPTION
  This file contains additions and modifications for BSP components
to work with the DosFs 2.0 component.
Modified configlettes in this component are all located in
target/config/comps/src/dosfs2/ directory in order to avoid modifying
the original Tornado 2.0 configlettes.

XXX - to do
*/

//
// Floppy Disk Section
//
Parameter FD_DEV_NAME {
	NAME		Floppy disk logical device name
	TYPE		string
	DEFAULT		"/fd"	// 0 or 1 will be appended
}
Parameter FD_CACHE_SIZE {
	NAME		Disk cache for Floppy Disk
	SYNOPSIS	Recommended 128K to compensate for slow mechanics
	TYPE		int
	DEFAULT		(128*1024)
}
Parameter FD_0_TYPE {
	NAME		Floppy Disk 0 drive type 
	SYNOPSIS	Use 0 for 3.5 and 1 for 5.25 inch drives
	TYPE		int
	DEFAULT		0
}
Parameter FD_1_TYPE {
	NAME		Floppy Disk 1 drive type 
	SYNOPSIS	Use 0 for 3.5 and 1 for 5.25 inch drives,\
			Use NONE of none installed.
	TYPE		int
	DEFAULT		NONE
}
	
Component INCLUDE_FD {
	NAME		floppy drive (NEC 765)
	SYNOPSIS	NEC 765 floppy drive component
	MODULES		nec765Fd.o
	CONFIGLETTES	dosfs2/usrFd.c
	CFG_PARAMS	FD_INT_VEC FD_INT_LVL \
			FD_CACHE_SIZE FD_DEV_NAME \
			FD_0_TYPE FD_1_TYPE
	HDR_FILES	drv/fdisk/nec765Fd.h
	REQUIRES	INCLUDE_DOSFS_MAIN INCLUDE_DISK_CACHE
	INIT_RTN	fdDrv (FD_INT_VEC, FD_INT_LVL);\
			usrFdConfig(0, FD_0_TYPE, FD_DEV_NAME "0");\
			usrFdConfig(1, FD_1_TYPE, FD_DEV_NAME "1");
}

//
// ATA Hard drive component
//

Parameter DOSFS_NAMES_ATA_PRIMARY_MASTER {
        NAME            ATA Controller 0, Hard disk 0 dos volume names, used in dosFsDevCreate
        SYNOPSIS        Comma separated list for each partition: "/ata0a,/ata0b", place optional filesystem type in parens: "/cd(cdrom)"
        TYPE            string
        DEFAULT         "/ata0a"
}
Parameter DOSFS_NAMES_ATA_PRIMARY_SLAVE {
        NAME            ATA Controller 0, Hard disk 1 logical names
        SYNOPSIS        Comma separated list for each partition: "/ata1a,/ata1b", place optional filesystem type in parens: "/cd(cdrom)"
        TYPE            string
        DEFAULT         ""
}
Parameter DOSFS_NAMES_ATA_SECONDARY_MASTER {
        NAME            ATA Controller 1, Hard disk 0 logical names
        SYNOPSIS        Comma separated list for each partition: "/ata2a,/ata2b", place optional filesystem type in parens: "/cd(cdrom)"
        TYPE            string
        DEFAULT         ""
}
Parameter DOSFS_NAMES_ATA_SECONDARY_SLAVE {
        NAME            ATA Controller 1, Hard disk 1 logical names
        SYNOPSIS        Comma separated list for each partition: "/ata3a,/ata3b", place optional filesystem type in parens: "/cd(cdrom)"
        TYPE            string
        DEFAULT         ""
}

Parameter ATA_CACHE_SIZE {
	NAME		Size of disk cache for Hard Disk
	TYPE		int
	DEFAULT		(128*1024)
}

Component INCLUDE_ATA {
	NAME		ATA hard drive
	SYNOPSIS	ATA hard drive component
	MODULES		ataDrv.o
	CONFIGLETTES	dosfs2/usrAta.c
	CFG_PARAMS	ATA_CACHE_SIZE NUM_DOSFS_FILES \
			DOSFS_NAMES_ATA_SECONDARY_SLAVE \
                        DOSFS_NAMES_ATA_SECONDARY_MASTER \
                        DOSFS_NAMES_ATA_PRIMARY_SLAVE DOSFS_NAMES_ATA_PRIMARY_MASTER
	REQUIRES	INCLUDE_DISK_CACHE INCLUDE_DISK_PART
	HDR_FILES	drv/hdisk/ataDrv.h drv/pcmcia/pccardLib.h
	INIT_RTN	usrAtaInit ();\
			if (strcmp (DOSFS_NAMES_ATA_PRIMARY_MASTER, "" )) \
			    usrAtaConfig (0, 0, DOSFS_NAMES_ATA_PRIMARY_MASTER); \
			if (strcmp (DOSFS_NAMES_ATA_PRIMARY_SLAVE, "" )) \
			    usrAtaConfig (0, 1, DOSFS_NAMES_ATA_PRIMARY_SLAVE); \
			if (strcmp (DOSFS_NAMES_ATA_SECONDARY_MASTER, "" )) \
			    usrAtaConfig (1, 0, DOSFS_NAMES_ATA_SECONDARY_MASTER); \
			if (strcmp (DOSFS_NAMES_ATA_SECONDARY_SLAVE, "" )) \
			    usrAtaConfig (1, 1, DOSFS_NAMES_ATA_SECONDARY_SLAVE);
}

Component INCLUDE_ATA_SHOW {
	NAME            ATA hard drive information display/show 
	MODULES		ataShow.o
	INIT_RTN	ataShowInit();
	REQUIRES	INCLUDE_ATA
	_CHILDREN	FOLDER_HD
}

