/*
Copyright 1998-2002 Wind River Systems, Inc.


modification history
--------------------
01g,27mar02,jkf  SPR#74716, moved INCLUDE_DISK_UTIL to 00vxWorks.cdf
01f,07mar02,jkf  fixed SPR#73968, removed INCLUDE_FTPD_ANONYMOUS_ACCESS
01e,03oct01,jkf  removed ramDiskDevCreate declaration.
01d,27sep01,jkf  veloce changes, added new headers
01c,10oct99,jkf  changed for cbioLib public API.
01b,15oct98,lrn  added RAWFS section override
01a,07oct98,lrn  written


DESCRIPTION
  This file contains descriptions for DosFs 2.0 component release

*/


// DosFs 2.0 module description
Folder FOLDER_DOSFS2
	{
	NAME		dosFs File System Components (dosFs2)
	SYNOPSIS	DOS File System, and related components
	_CHILDREN	FOLDER_IO_SYSTEM
	CHILDREN	INCLUDE_DOSFS_MAIN \
			INCLUDE_DOSFS_FAT \
			SELECT_DOSFS_DIR \
			INCLUDE_DOSFS_FMT \
			INCLUDE_DOSFS_CHKDSK \
			INCLUDE_CBIO \
			INCLUDE_DISK_CACHE \
			INCLUDE_DISK_PART \
			INCLUDE_TAR
	// Defaults are minimal at this stage
	DEFAULTS	\
			INCLUDE_DOSFS_MAIN \
			INCLUDE_DOSFS_FAT \
			INCLUDE_DOSFS_DIR_VFAT \
			INCLUDE_CBIO
	}

Selection SELECT_DOSFS_DIR
	{
	NAME		DOS File System Directory Handlers
	COUNT		1-
	CHILDREN	INCLUDE_DOSFS_DIR_VFAT INCLUDE_DOSFS_DIR_FIXED
	DEFAULTS	INCLUDE_DOSFS_DIR_VFAT
	}

Component INCLUDE_DOSFS_MAIN
	{
	NAME		dosfs File System Main Module (dosFs2)
	MODULES		dosFsLib.o
	INIT_RTN	dosFsLibInit(0);
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_CBIO INCLUDE_DOSFS_FAT SELECT_DOSFS_DIR
	}

Component INCLUDE_DOSFS_FAT
	{
	NAME		DOS File System FAT12/16/32 Handler
	MODULES		dosFsFat.o
	INIT_RTN	dosFsFatInit();
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_DOSFS_MAIN
	}

Component INCLUDE_DOSFS_DIR_VFAT
	{
	NAME		DOS File System VFAT Directory Handler 
	SYNOPSIS	VFAT Variable-length file names support, Win95/NT compatible
	MODULES		dosVDirLib.o
	INIT_RTN	dosVDirLibInit();
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_DOSFS_MAIN
	}

Component INCLUDE_DOSFS_DIR_FIXED
	{
	NAME		DOS File System Old Directory Format Handler 
	SYNOPSIS	Strict 8.3 and VxLongs propriatery long names
	MODULES		dosDirOldLib.o
	INIT_RTN	dosDirOldLibInit();
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_DOSFS_MAIN
	}

Component INCLUDE_DOSFS_FMT
	{
	NAME		DOS File System Volume Formatter Module
	SYNOPSIS	High level formatting of DOS volumes
	MODULES		dosFsFmtLib.o
	INIT_RTN	dosFsFmtLibInit();
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_DOSFS_MAIN
	}

Component INCLUDE_DOSFS_CHKDSK
	{
	NAME		DOS File System Consistency Checker
	SYNOPSIS	Consistency checking set on per-device basis
	MODULES		dosChkLib.o
	INIT_RTN	dosChkLibInit();
	HDR_FILES	dosFsLib.h
	REQUIRES	INCLUDE_DOSFS_MAIN
	}

Component INCLUDE_CBIO
	{
	NAME		CBIO (Cached Block I/O) Support, cbioLib
	MODULES		cbioLib.o
	INIT_RTN	cbioLibInit();
	HDR_FILES	cbioLib.h
	}

Component INCLUDE_DISK_CACHE
	{
	NAME		CBIO Disk Cache Handler
	SYNOPSIS	CBIO Disk Cache size is set on per-device basis
	MODULES		dcacheCbio.o
	HDR_FILES	dcacheCbio.h
	LINK_SYMS	dcacheDevCreate
	REQUIRES	INCLUDE_CBIO
	}

Component INCLUDE_DISK_PART
	{
	NAME		CBIO Disk Partition Handler
	SYNOPSIS	Supports disk paritition tables
	MODULES		dpartCbio.o usrFdiskPartLib.o
	HDR_FILES	dpartCbio.h usrFdiskPartLib.h
	LINK_SYMS	dpartDevCreate usrFdiskPartRead
	REQUIRES	INCLUDE_CBIO
	}

Component INCLUDE_TAR
	{
	NAME		File System Backup and Archival
	SYNOPSIS	UNIX-compatible TAR facility
	MODULES		tarLib.o
	LINK_SYMS	tarHelp
	HDR_FILES	tarLib.h
	}

InitGroup usrDosFsInit
	{
	INIT_RTN	usrDosFsInit ();
	SYNOPSIS	DOS File System components
	_INIT_ORDER	usrIosExtraInit

	INIT_BEFORE 	INCLUDE_SCSI \
			INCLUDE_FD \
			INCLUDE_IDE \
			INCLUDE_ATA \
			INCLUDE_PCMCIA \
			INCLUDE_TFFS

	INIT_ORDER	\
			INCLUDE_CBIO \
			INCLUDE_DOSFS_MAIN \
			INCLUDE_DOSFS_FAT \
			INCLUDE_DOSFS_DIR_VFAT \
			INCLUDE_DOSFS_DIR_FIXED \
			INCLUDE_DOSFS_CHKDSK \
			INCLUDE_DOSFS_FMT \
			INCLUDE_RAM_DISK
	}

// Backward compatible configuration
Component INCLUDE_DOSFS {
	NAME		DOS filesystem backward-compatibility
	SYNOPSIS	Old dosFs API module, depreciated
	MODULES		usrDosFsOld.o
	LINK_SYMS	dosFsInit
	HDR_FILES	dosFsLib.h
	INIT_BEFORE 	INCLUDE_SCSI \
			INCLUDE_FD \
			INCLUDE_IDE \
			INCLUDE_ATA \
			INCLUDE_PCMCIA \
			INCLUDE_TFFS 
       }

// Ram disk

Component INCLUDE_RAM_DISK
        {
        NAME            CBIO RAM Disk with DOS File System
        MODULES         ramDiskCbio.o
        CFG_PARAMS      RAM_DISK_SIZE RAM_DISK_DEV_NAME RAM_DISK_MAX_FILES RAM_DISK_MEM_ADRS
        INIT_RTN        { CBIO_DEV_ID cbio ; \
            cbio=ramDiskDevCreate(RAM_DISK_MEM_ADRS,512,17,RAM_DISK_SIZE/512,0);\
            if(cbio!=NULL){ \
                dosFsDevCreate(RAM_DISK_DEV_NAME,cbio,RAM_DISK_MAX_FILES,NONE);\
                dosFsVolFormat(cbio,DOS_OPT_BLANK | DOS_OPT_QUIET, NULL);\
            }}
        INIT_AFTER      FOLDER_DOSFS2
        _CHILDREN       FOLDER_PERIPHERALS
        REQUIRES        INCLUDE_DOSFS_MAIN
        HDR_FILES       dosFsLib.h ramDiskCbio.h
        }

Parameter RAM_DISK_DEV_NAME
        {
        NAME            RAM Disk logical device name
        TYPE            string
        DEFAULT         "/ram0"
        }
Parameter RAM_DISK_SIZE
        {
        NAME            Size of RAM allocated for RAM Disk
        TYPE            int
        DEFAULT         0x10000
        }

Parameter RAM_DISK_MAX_FILES
        {
        NAME            Maximum open files on RAM disk
        TYPE            int
        DEFAULT         20
        }


Parameter RAM_DISK_MEM_ADRS
        {
        NAME            Default pool address, 0 = malloc
        TYPE            int  
        DEFAULT         0
        }
//
// CBIO Compliant RAW File system
//
Component INCLUDE_RAWFS {
	NAME		CBIO API RAW Filesystem
	SYNOPSIS	Raw file system with 64-bit arithmetic, use CBIO API at lower layer
	MODULES		rawFsLib.o
	INIT_RTN	rawFsInit (NUM_RAWFS_FILES);
	CFG_PARAMS	NUM_RAWFS_FILES
        REQUIRES        INCLUDE_CBIO
	HDR_FILES	rawFsLib.h cbioLib.h 
}
