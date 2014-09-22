/* tarLib.c - UNIX tar compatible library */

/* Copyright 1993-2002 Wind River Systems, Inc. */
/* Copyright (c) 1993, 1994 RST Software Industries Ltd. */

/*
modification history
--------------------
01i,20sep01,jkf  SPR#69031, common code for both AE & 5.x.
01h,26dec99,jkf  T3 KHEAP_ALLOC
03g,11nov99,jkf  need to fill all eight bytes with 0x20 for the checksum 
                 calc correct for MKS toolkit 6.2 generated tar file.
03f,31jul99,jkf  T2 merge, tidiness & spelling.
03e,30jul98,lrn  partial doc cleanup
03b,24jun98,lrn  fixed bug causing 0-size files to be extracted as dirs,
		 improved tarHelp to list parameters
03a,07jun98,lrn  derived from RST usrTapeLib.c, cleaned all tape related stuff
02f,15jan95,rst  adding TarToc functionality
02d,28mar94,rst  added Tar utilities.
*/

/*
DESCRIPTION

This library implements functions for archiving, extracting and listing
of UNIX-compatible "tar" file archives.
It can be used to archive and extract entire file hierarchies
to/from archive files on local or remote disks, or directly to/from
magnetic tapes.

SEE ALSO: dosFsLib

CURRENT LIMITATIONS
This Tar utility does not handle MS-DOS file attributes,
when used in conjunction with the MS-DOS file system.
The maximum subdirectory depth supported by this library is 16,
while the total maximum path name that can be handled by 
tar is limited at 100 characters.
*/

#include "vxWorks.h"
#include "stdlib.h"
#include "ioLib.h"
#include "errnoLib.h"
#include "string.h"
#include "stdio.h"
#include "private/dosFsVerP.h"
#include "dosFsLib.h"
#include "dirent.h"
#include "stat.h"
#include "tarLib.h"

/* data types */
/*
 * VxWorks Tar Utility, part of usrTapeLib (for now).
 * UNIX tar(1) tape format - header definition
 */

#define TBLOCK		512	/* TAR Tape block size, part of TAR format */
#define NAMSIZ		100	/* size of file name in TAR tape header */
#define	MAXLEVEL	16	/* max. # of subdirectory levels, arbitrary */
#define	NULLDEV		"/null"	/* data sink */

typedef union hblock
    {
    char dummy[TBLOCK];
    struct header
	{
	char name[NAMSIZ];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char linkflag;
	char linkname[NAMSIZ];
    	}
    dbuf;
    } MT_HBLOCK ;

/* Control structure used internally for reentrancy etc. */
typedef struct
    {
    int 	fd ;			/* current tape file descriptor */
    int		bFactor;		/* current blocking factor */
    int		bValid;			/* number of valid blocks in buffer */
    int		recurseCnt;		/* recusrion counter */
    MT_HBLOCK *	pBuf;			/* working buffer */
    MT_HBLOCK *	pBnext;			/* ptr to next block to return */
    } MT_TAR_SOFT ;

/* globals */

char *TAPE = "/archive0.tar" ;	/* default archive name */

/* locals */
LOCAL char	bZero[ TBLOCK ] ; /* zeroed block */

/*******************************************************************************
*
* tarHelp - print help information about TAR archive functions.
*
* NOMANUAL
*/
void tarHelp
    (
    void
    )
    {
    int i ;
    const char * help_msg[] = {
    "Help information on TAR archival functions:\n",
    "\n",
    " Archive any directory: \n",
    "    tarArchive( archFile, bfactor, verbose, dirName )\n",
    " Extract from TAR archive into current directory:\n",
    "    tarExtract( archFile, bfactor, verbose )\n",
    " List contents of TAR archive\n",
    "    tarToc( archFile, bfactor)\n",
    "      if <dirName> is NULL, archive current directory\n",
    "\n", NULL 
    };
    for(i = 0; help_msg[i] != NULL ; i++ )
	printf(help_msg[i]);

    printf(" if <archFile> is NULL,\n"
	"current default archive file \"%s\" will be used,\n", TAPE);
    printf(" change the TAPE variable to set a different name.\n\n");

    }

/*******************************************************************************
*
* mtChecksum - calculate checksums for tar header blocks
*
* RETURNS: the checksum value
*/

LOCAL int mtChecksum
    (
    void *	pBuf,
    unsigned	size
    )
    {
    register int sum = 0 ;
    register unsigned char *p = pBuf ;

    while( size > 0 )
	{
	sum += *p++;
	size -= sizeof(*p);
	}

    return(sum);
    }

/*******************************************************************************
*
* tarRdBlks - read <nBlocks> blocks from tape
*
* RETURNS: number of blocks actually got, or ERROR.
*/

LOCAL int tarRdBlks
    (
    MT_TAR_SOFT *pCtrl,		/* control structure */
    MT_HBLOCK **ppBlk,		/* where to return buffer address */
    unsigned int nBlocks	/* how many blocks to get */
    )
    {
    register int rc ;

    if (pCtrl->bValid <= 0)
	{
	/* buffer empty, read more blocks from tape */

	rc = read(pCtrl->fd, pCtrl->pBuf->dummy, pCtrl->bFactor * TBLOCK);

	if ( rc == ERROR )
	    return ERROR ;
	else if( rc == 0 )
	    return 0 ;
	else if( (rc % TBLOCK) != 0 )
	    {
	    printErr("tarRdBlks: tape block not multiple of %d\n", TBLOCK);
	    return ERROR;
	    }
	

	pCtrl->bValid = rc / TBLOCK ;
	pCtrl->pBnext = pCtrl->pBuf ;
	}

    rc = min((ULONG)pCtrl->bValid, nBlocks) ;
    *ppBlk = pCtrl->pBnext ;
    pCtrl->bValid -= rc ;
    pCtrl->pBnext += rc ;

    return( rc ) ;
    }

/*******************************************************************************
*
* mtAccess - check existence of path for a new file or directory <name>
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS mtAccess
    (
    const char *name
    )
    {
    char tmpName [ NAMSIZ ] ;
    struct stat st ;
    int i ;
    static char slash = '/' ;
    register char *pSlash ;

    strncpy( tmpName, name, NAMSIZ ) ;
    pSlash = tmpName ;

    for(i=0; i<MAXLEVEL; i++)
	{
	if ( (pSlash = strchr(pSlash, slash)) == NULL )
	    return OK;

	*pSlash = '\0' ;

	if ( stat( tmpName, &st ) == OK )
	    {
	    if( S_ISDIR(st.st_mode ) == 0)
		{
		printErr("Path %s is not a directory\n", tmpName);
		return ERROR;
		}
	    }
	else
	    {
	    mkdir( tmpName );
	    }

	*pSlash = slash ;	/* restore slash position */
	pSlash++;
	}
    printErr("Path too long %s\n", name );
    return ERROR;
    }

/*******************************************************************************
*
* tarExtractFile - extract one file or directory from tar tape
*
* Called from tarExtract for every file/dir found on tape.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS tarExtractFile
    (
    MT_TAR_SOFT	*pCtrl,		/* control structure */
    MT_HBLOCK	*pBlk,		/* header block */
    BOOL	verbose		/* verbose */
    )
    {
    register int rc, fd ;
    int		sum = -1 ;		/* checksum */
    int		size = 0 ;		/* file size in bytes */
    int		nblks = 0;		/* file size in TBLOCKs */
    int		mode ;
    char *	fn ;			/* file/dir name */


    /* Check the checksum of this header */

    rc = sscanf( pBlk->dbuf.chksum, "%o", &sum ) ;	/* extract checksum */

    bfill( pBlk->dbuf.chksum, 8, ' ');		/* fill blanks */

    if( mtChecksum( pBlk->dummy, TBLOCK ) != sum )
	{
	printErr("bad checksum %d != %d\n", mtChecksum(
		pBlk->dummy, TBLOCK), sum );
	return ERROR;
	}

    /* Parse all fields of header that we need, and store them safely */
    sscanf( pBlk->dbuf.mode, "%o", &mode );
    sscanf( pBlk->dbuf.size, "%12o", &size );
    fn = pBlk->dbuf.name ;

    /* Handle directory */
    if( (size == 0) && ( fn[ strlen(fn) - 1 ] == '/' ) )
	{
	if( strcmp(fn, "./") == 0)
	    return OK;

	if ( fn[ strlen(fn) - 1 ] == '/' )	/* cut the slash */
	    fn[ strlen(fn) - 1 ] = '\0' ;

	/* Must make sure that parent exists for this new directory */
	if ( mtAccess(fn) == ERROR )
	    {
	    return ERROR;
	    }

	fd = open (fn, O_RDWR | O_CREAT, FSTAT_DIR | (mode & 0777) );
	if (( fd == ERROR ) && (errnoGet() == S_dosFsLib_FILE_EXISTS))
	    {
	    printErr("Warning: directory %s already exists\n", fn );
	    return OK;
	    }
	else if (fd == ERROR)
	    {
	    printErr("failed to create directory %s, %s, continuing\n",
		     fn, strerror(errnoGet()));
	    return OK;
	    }

	if(verbose)
	    printErr("created directory %s.\n", fn );

	return (close (fd));
	}

    /* non-empty file has a trailing slash, we better treat it as file and */
    if ( fn[ strlen(fn) - 1 ] == '/' )	/* cut a trailing slash */
	fn[ strlen(fn) - 1 ] = '\0' ;

    /* Filter out links etc */

    if ((pBlk->dbuf.linkflag != '\0') &&
    	(pBlk->dbuf.linkflag != '0') &&
	(pBlk->dbuf.linkflag != ' '))
	{
	printErr("we do not support links, %s skipped\n", fn );
	return OK;
	}

    /* Handle Regular File - calculate number of blocks */
    if( size > 0 )
	nblks = ( size / TBLOCK ) +  ((size % TBLOCK)? 1 : 0 ) ;

    /* Must make sure that directory exists for this new file */
    if ( mtAccess(fn) == ERROR )
	{
	return ERROR;
	}

    /* Create file */
    fd = open( fn, O_RDWR | O_CREAT, mode & 0777 ) ;
    if( fd == ERROR )
	{
	printErr("failed to create file %s, %s, skipping!\n",
		fn, strerror(errnoGet()));
	fd = open( NULLDEV, O_RDWR, 0) ;
	}

    if(verbose)
	printErr("extracting file %s, size %d bytes, %d blocks\n",
		fn, size, nblks );

    /* Loop until entire file extracted */
    while (size > 0)
	{
	MT_HBLOCK *pBuf;
	register int wc ;

	rc = tarRdBlks( pCtrl, &pBuf, nblks) ;

	if ( rc < 0 )
	    {
	    printErr("error reading tape\n");
	    close(fd);
	    return ERROR;
	    }

	wc = write( fd, pBuf->dummy, min( rc*TBLOCK, size ) ) ;

	if( wc == ERROR )
	    {
	    printErr("error writing file\n");
	    break;
	    }

	size -= rc*TBLOCK ;
	nblks -= rc ;
	}


    /* Close the newly created file */
    return( close(fd) ) ;

    }

/*******************************************************************************
*
* tarExtract - extract all files from a tar formatted tape
*
* This is a UNIX-tar compatible utility that extracts entire
* file hierarchies from tar-formatted archive.
* The files are extracted with their original names and modes.
* In some cases a file cannot be created on disk, for example if
* the name is too long for regular DOS file name conventions,
* in such cases entire files are skipped, and this program will
* continue with the next file. Directories are created in order
* to be able to create all files on tape.
*
* The <tape> argument may be any tape device name or file name
* that contains a tar formatted archive. If <tape> is equal "-",
* standard input is used. If <tape> is NULL (or unspecified from Shell)
* the default archive file name stored in global variable <TAPE> is used.
*
* The <bfactor> dictates the blocking factor the tape was written with.
* If 0, or unspecified from the shell, a default of 20 is used.
*
* The <verbose> argument is a boolean, if set to 1, will cause informative
* messages to be printed to standard error whenever an action is taken,
* otherwise, only errors are reported.
*
* All informative and error message are printed to standard error.
*
* There is no way to selectively extract tar archives with this
* utility. It extracts entire archives.
*/

STATUS tarExtract
    (
    char *	pTape,		/* tape device name */
    int		bfactor,	/* requested blocking factor */
    BOOL	verbose		/* if TRUE print progress info */
    )
    {
    register 	int rc ;		/* return codes */
    MT_TAR_SOFT	ctrl ;		/* control structure */
    STATUS	retval = OK;

    /* Set defaults */
    if( pTape == NULL )
	pTape = TAPE ;

    if( bfactor == 0 )
	bfactor = 20 ;

    if( verbose )
	printErr("Extracting from %s\n", pTape );

    bzero( (char *)&ctrl, sizeof(ctrl) );
    bzero( bZero, sizeof(bZero) );	/* not harmful for reentrancy */

    /* Open tape device and initialize control structure */
    if( strcmp(pTape, "-") == 0)
	ctrl.fd = STD_IN;
    else
	ctrl.fd = open( pTape, O_RDONLY, 0) ;

    if ( ctrl.fd < 0 )
	{
	printErr("Failed to open tape: %s\n", strerror(errnoGet()) );
	return ERROR;
	}
    
    ctrl.bFactor = bfactor ;
    ctrl.pBuf = KHEAP_ALLOC( bfactor * TBLOCK ) ;

    if ( ctrl.pBuf == NULL )
	{
	printErr("Not enough memory, exiting.\n" );
	if ( ctrl.fd != STD_IN )
	    close( ctrl.fd );
	return ERROR ;
	}

    /* all exits from now via goto in order to free the buffer */

    /* Read the first block and adjust blocking factor */

    rc = read( ctrl.fd, ctrl.pBuf->dummy, ctrl.bFactor*TBLOCK ) ;

    if ( rc == ERROR )
	{
	printErr("read error at the beginning of tape, exiting.\n" );
	retval = ERROR ;
	goto finish;
	}
    else if( rc == 0 )
	{
	printErr("empty tape file, exiting.\n" );
	goto finish;
	}
    else if( (rc % TBLOCK) != 0 )
	{
	printErr("tape block not multiple of %d, exiting.\n", TBLOCK);
	retval = ERROR ;
	goto finish;
	}

    ctrl.bValid = rc / TBLOCK ;
    ctrl.pBnext = ctrl.pBuf ;
    if ( ctrl.bFactor != ctrl.bValid )
	{
	if( verbose )
	    printErr("adjusting blocking factor to %d\n", ctrl.bValid );
	ctrl.bFactor = ctrl.bValid ;
	}

    /* End of overture, start processing files until end of tape */

    FOREVER
	{
	MT_HBLOCK *	pBlk ;

	if ( tarRdBlks( &ctrl, &pBlk, 1) != 1 )
	    {
	    retval = ERROR ;
	    goto finish;
	    }

	if ( bcmp( pBlk->dummy, bZero, TBLOCK) == 0 )
	    {
	    if(verbose)
		printErr("end of tape encountered, read until eof...\n");
	    while( tarRdBlks( &ctrl, &pBlk, 1) > 0) ;
	    if(verbose)
		printErr("done.\n");
	    retval = OK ;
	    goto finish;
	    }

	if ( tarExtractFile( &ctrl, pBlk, verbose) == ERROR )
	    {
	    retval = ERROR;
	    goto finish;
	    }

	} /* end of FOREVER */
    
finish:
	if ( ctrl.fd != STD_IN )
	    close( ctrl.fd );
	KHEAP_FREE((char *)ctrl.pBuf );
	return( retval );
    }


/*******************************************************************************
*
* tarWrtBlks - write <nBlocks> blocks to tape
*
* Receives any number of blocks, and handles the blocking factor
* mechanism using the pCtrl->pBuf internal buffer and associated
* counters.
*
* RETURNS: OK or ERROR;
*/

LOCAL STATUS tarWrtBlks
    (
    MT_TAR_SOFT *pCtrl,		/* control structure */
    char *pBuf,			/* data to write */
    unsigned int nBlocks	/* how many blocks to get */
    )
    {
    register int rc ;
    while( nBlocks > 0 )
	{
	if ((pCtrl->bValid <= 0) && (nBlocks >= (unsigned int)pCtrl->bFactor))
	    {
	    /* internal buffer empty, write directly */
	    rc = write( pCtrl->fd, pBuf, pCtrl->bFactor*TBLOCK ) ;

	    if ( rc == ERROR )
		return ERROR ;

	    /* adjust count and pointer */
	    pBuf += TBLOCK * pCtrl->bFactor ;
	    nBlocks -= pCtrl->bFactor ;
	    }
	else 
	    {
	    register int cnt ;

	    /* internal buffer partially full, fill it up */
	    cnt = pCtrl->bFactor - pCtrl->bValid ;
	    cnt = min( (ULONG)cnt, nBlocks );

	    bcopy( pBuf, (char *) (pCtrl->pBuf + pCtrl->bValid), cnt*TBLOCK );
	    pBuf += cnt*TBLOCK;
	    nBlocks -= cnt ;
	    pCtrl->bValid += cnt ;
	    }

	if( pCtrl->bValid == pCtrl->bFactor  )
	    {
	    /* one full blocked buffer, write to tape */
	    rc = write( pCtrl->fd, (char *) pCtrl->pBuf,
			pCtrl->bFactor*TBLOCK ) ;

	    if ( rc == ERROR )
		return ERROR ;

	    /* adjust count and pointer */
	    pCtrl->bValid = 0 ;
	    }

	} /* while */

    return OK;
    }

/*******************************************************************************
*
* tarCreateHdr - build a tar header block 
*
* NOTE:
* SunOS tar(5) man page is not accurate, this function
* was built using empiric methods with that tar(1) command.
* RETURNS: file size in bytes
*/

LOCAL int tarCreateHdr
    (
    MT_TAR_SOFT *pCtrl,		/* control structure */
    const char  *name,		/* file/dir name */
    struct stat *pStat,		/* file stats */
    MT_HBLOCK *	pBlk		/* block buffer for header */
    )
    {
    int	chksum;

    strncpy( pBlk->dbuf.name, name, NAMSIZ );

    if( S_ISDIR(pStat->st_mode) )
	{
	pStat->st_mode |= 0200 ;
	pStat->st_size = 0 ;
	}

    sprintf( pBlk->dbuf.mode, "%6o ", pStat->st_mode );
    sprintf( pBlk->dbuf.uid, "%6o ", pStat->st_uid );
    sprintf( pBlk->dbuf.gid, "%6o ", pStat->st_gid );
    sprintf( pBlk->dbuf.size, "%11lo ", pStat->st_size );
    sprintf( pBlk->dbuf.mtime, "%11lo ", pStat->st_mtime );
    pBlk->dbuf.linkflag = '0';
    bfill( pBlk->dbuf.chksum, 8, ' ');		/* fill blanks */

    chksum = mtChecksum( pBlk->dummy, TBLOCK ) ;

    sprintf( pBlk->dbuf.chksum, "%6o", chksum);

    return( pStat->st_size );
    }

/*******************************************************************************
*
* tarArchDo - archive a file/dir onto tape (recursive)
*
* Handle one file or one directory, in case of directory,
* write the dir entry and recurse.
*
* RETURNS: OK or ERROR.
*/

LOCAL STATUS tarArchDo
    (
    MT_TAR_SOFT *pCtrl,		/* control structure */
    BOOL	verbose,	/* verbosity flag */
    char *	name		/* file/dir name */
    )
    {
    MT_HBLOCK	hblock;		/* header block */
    struct stat	st;		/* file/dir stat */
    register int rc;

    bzero ( (char *) &hblock, sizeof( MT_HBLOCK ) );
    if( strlen( name ) >= (NAMSIZ-1))
	{
	printErr("tar: name too long %s, skip.\n", name );
	return ERROR;
	}

    rc = stat( name, &st );

    if ( rc == ERROR)
	{
	perror("tar: stat error ");
	return ERROR ;
	}

    if ( pCtrl->recurseCnt++ > MAXLEVEL )
	{
	printErr("tar: nesting too deep, skipping %s\n", name);
	goto abort ;
	}

    if( S_ISDIR(st.st_mode) )
	{
	char	fn [ NAMSIZ ];	/* subdir name */
	register DIR *	pDir;
	struct dirent *	pDirEnt ;

	/* handle directory */
	strncpy( fn, name, NAMSIZ-2 );
	strcat( fn, "/" );
	/* write directory header block to tape */
	tarCreateHdr( pCtrl, fn, &st, &hblock ) ;

	if( verbose )
	    printErr("tar: writing directory %s\n", name );

	if (tarWrtBlks( pCtrl, (char *) &hblock, 1 ) == ERROR )
	    {
	    perror("tar: error writing header to tape ");
	    goto abort;
	    }

	/* recurse on every directory member */
	pDir = opendir( name ) ;

	if( pDir == NULL )
	    {
	    perror((sprintf(fn,"tar: error opening directory %s ", name),fn));
	    goto abort;
	    }

	for( pDirEnt = readdir(pDir); pDirEnt != NULL ;
			pDirEnt = readdir(pDir))
	    {
	    /* Skip the "." and ".." entries */
	    if ( strcmp( pDirEnt->d_name, ".") == 0)
		continue ;
	    if ( strcmp( pDirEnt->d_name, "..") == 0)
		continue ;
	    /* build the new name by concatenating */
	    strncpy( fn, name, NAMSIZ-2 );
	    strcat( fn, "/" );
	    strcat( fn, pDirEnt->d_name );
	    /* recurse ! nesting level of MAXLEVEL maximum */
	    (void )tarArchDo( pCtrl, verbose, fn ) ;
	    }

	closedir( pDir );
	}
    else if( S_ISREG(st.st_mode) )
	{
	int		fileFd = -1 ;
	register int fileSize ;
	register int rc ;
	register char *	pFileBuf ;
	unsigned bufSize = pCtrl->bFactor*TBLOCK;
	unsigned nBlocks = 0 ;

	/* handle plain file */
	/* write file header to tape */
	fileSize = tarCreateHdr( pCtrl, name, &st, &hblock ) ;

	/* write the file itself to tape */
	pFileBuf = KHEAP_ALLOC( bufSize );

	if( pFileBuf == NULL )
	    {
	    printErr("tar: not enough memory\n" );
	    goto abort;
	    }
	fileFd = open( name, O_RDONLY, 0);

	if( fileFd == ERROR )
	    {
	    perror((sprintf(pFileBuf,"tar: file open error %s, ", name),
			pFileBuf));
	    KHEAP_FREE(((char *) pFileBuf) ); 
	    goto abort;
	    }
	/* Handle Regular File - calculate number of blocks */
	if( fileSize > 0 )
	    nBlocks = ( fileSize / TBLOCK ) +  ((fileSize % TBLOCK)? 1 : 0 ) ;

	if( verbose )
	    printErr("tar: writing file %s, size %d bytes, %d blocks\n",
			name, fileSize, nBlocks );

	if (tarWrtBlks( pCtrl, (char *) &hblock, 1 ) == ERROR )
	    {
	    perror("tar: error writing header to tape ");
	    KHEAP_FREE(((char *) pFileBuf) ); 
	    goto abort;
	    }

	while( nBlocks > 0 )
	    {
	    /* try to optimize into direct tape writing here */
	    rc = read( fileFd, pFileBuf,
		bufSize - (pCtrl->bValid*TBLOCK) ) ;

	    if( rc == ERROR )
		{
		perror((sprintf(pFileBuf,"tar: file read error %s, ", name),
			pFileBuf));
		rc = 1;
		}
	    else if( rc == 0 )
		{
		printErr("tar: file changed size!\n");
		rc = 1; 
		}
	    else
		{
		/* recalculate read count in blocks */
		rc = ( rc / TBLOCK ) +  ((rc % TBLOCK)? 1 : 0 ) ;
		}

	    /* Now write the file blocks to tape */
	    tarWrtBlks( pCtrl, pFileBuf, rc );
	    nBlocks -= rc ;
	    }/*while*/

	close( fileFd );
	KHEAP_FREE(((char *) pFileBuf));
	} /* end of regular file handling */
    else
	{
	printErr("tar: not a regular file nor a directory %s, skipped\n",
		name );
	goto abort;
	}

    pCtrl->recurseCnt --;
    return OK;

abort:
    pCtrl->recurseCnt --;
    return ERROR;
    }

/*******************************************************************************
*
* tarArchive - archive named file/dir onto tape in tar format
*
* This function creates a UNIX compatible tar formatted archives
* which contain entire file hierarchies from disk file systems.
* Files and directories are archived with mode and time information
* as returned by stat().
*
* The <tape> argument can be any tape drive device name or a name of any
* file that will be created if necessary, and will contain the archive.
* If <tape> is set to "-", standard output will be used.
* If <tape> is NULL (unspecified from Shell), the default archive file
* name stored in global variable <TAPE> will be used.
*
* Each write() of the archive file will be exactly <bfactor>*512 bytes
* long, hence on tapes in variable mode, this will be the physical
* block size on the tape. With Fixed Mode tapes this is only a performance
* matter. If <bfactor> is 0, or unspecified from Shell, it will be set
* to the default value of 20.
*
* The <verbose> argument is a boolean, if set to 1, will cause informative
* messages to be printed to standard error whenever an action is taken,
* otherwise, only errors are reported.
*
* The <name> argument is the path of the hierarchy to be archived.
* if NULL (or unspecified from the Shell), the current directory path "."
* will be used.  This is the path as seen from the target, not from 
* the Tornado host.
*
* All informative and error message are printed to standard error.
*
* NOTE
* Refrain from specifying absolute path names in <path>, such archives
* tend to be either difficult to extract or can cause unexpected
* damage to existing files if such exist under the same absolute name.
*
* There is no way of specifying a number of hierarchies to dump.
*/

STATUS tarArchive
    (
    char *	pTape,		/* tape device name */
    int		bfactor,	/* requested blocking factor */
    BOOL	verbose,	/* if TRUE print progress info */
    char *	pName		/* file/dir name to archive */
    )
    {
    MT_TAR_SOFT	ctrl;		/* control structure */
    STATUS	retval = OK;

    /* Set defaults */
    if( pTape == NULL )
	pTape = TAPE ;

    if( bfactor == 0 )
	bfactor = 20 ;

    if( pName == NULL )
	pName = "." ;

    if( verbose )
	printErr("Archiving to %s\n", pTape );

    bzero( (char *)&ctrl, sizeof(ctrl) );
    bzero( bZero, sizeof(bZero) );	/* not harmful for reentrancy */

    /* Open tape device and initialize control structure */
    if( strcmp(pTape, "-") == 0)
	ctrl.fd = STD_OUT;
    else
	ctrl.fd = open( pTape, O_CREAT | O_WRONLY, 0644) ;

    if ( ctrl.fd < 0 )
	{
	printErr("Failed to open tape: %s\n", strerror(errnoGet()) );
	return ERROR;
	}
    
    ctrl.bValid = 0 ;
    ctrl.bFactor = bfactor ;
    ctrl.pBuf = KHEAP_ALLOC( bfactor * TBLOCK ) ;

    if ( ctrl.pBuf == NULL )
	{
	printErr("Not enough memory, exiting.\n" );
	if ( ctrl.fd != STD_OUT)
	    close( ctrl.fd );
	return ERROR ;
	}

    /* all exits from now via goto in order to free the buffer */

    retval = tarArchDo( &ctrl, verbose, pName ) ;

    /* at end of tape, write at least two zero blocks */
    if( verbose )
	printErr("tar: writing end of tape.\n");

    tarWrtBlks( &ctrl, bZero, 1 );
    tarWrtBlks( &ctrl, bZero, 1 );

    /* and fill the last blocked block until written to tape */
    while( ctrl.bValid > 0 )
	tarWrtBlks( &ctrl, bZero, 1 );
    
    if( ctrl.fd != STD_OUT )
	close( ctrl.fd );
    KHEAP_FREE(((char *) ctrl.pBuf));
    return( retval );
    }


/*******************************************************************************
*
* tarTocFile - display one file or directory from tar tape
*
* Called from tarToc for every file/dir found on tape.
*
* RETURNS: OK or ERROR
*/

LOCAL STATUS tarTocFile
    (
    MT_TAR_SOFT	*pCtrl,		/* control structure */
    MT_HBLOCK	*pBlk		/* header block */
    )
    {
    register	int rc ;
    int		sum = -1 ;		/* checksum */
    int		size = 0 ;		/* file size in bytes */
    int		nblks = 0;		/* file size in TBLOCKs */
    int		mode ;
    char *	fn ;			/* file/dir name */


    /* Check the checksum of this header */

    rc = sscanf( pBlk->dbuf.chksum, "%o", &sum ) ;	/* extract checksum */

    bfill( pBlk->dbuf.chksum, 8, ' ');		/* fill blanks */

    if( mtChecksum( pBlk->dummy, TBLOCK ) != sum )
	{
	printErr("bad checksum %d != %d\n",
		mtChecksum( pBlk->dummy, TBLOCK), sum );
	return ERROR;
	}

    /* Parse all fields of header that we need, and store them safely */
    sscanf( pBlk->dbuf.mode, "%o", &mode );
    sscanf( pBlk->dbuf.size, "%12o", &size );
    fn = pBlk->dbuf.name ;

    /* Handle directory */
    if( (size == 0) || ( fn[ strlen(fn) - 1 ] == '/' ) )
	{
	printErr("directory %s.\n", fn );
	}

    /* Filter out links etc */

    if ((pBlk->dbuf.linkflag != '\0') &&
    	(pBlk->dbuf.linkflag != '0') &&
	(pBlk->dbuf.linkflag != ' '))
	{
	printErr("we do not support links, %s skipped\n", fn );
	return OK;
	}

    /* Handle Regular File - calculate number of blocks */
    if( size > 0 )
	nblks = ( size / TBLOCK ) +  ((size % TBLOCK)? 1 : 0 ) ;

    printErr("file %s, size %d bytes, %d blocks\n",
		fn, size, nblks );

    /* Loop until entire file skipped */
    while (size > 0)
	{
	MT_HBLOCK *pBuf;

	rc = tarRdBlks( pCtrl, &pBuf, nblks) ;

	if ( rc < 0 )
	    {
	    printErr("error reading tape\n");
	    return ERROR;
	    }

	size -= rc*TBLOCK ;
	nblks -= rc ;
	}
    return( OK );
    }

/*******************************************************************************
*
* tarToc - display all contents of a tar formatted tape
*
* This is a UNIX-tar compatible utility that displays entire
* file hierarchies from tar-formatted media, e.g. tape.
*
* The <tape> argument may be any tape device name or file name
* that contains a tar formatted archive. If <tape> is equal "-",
* standard input is used. If <tape> is NULL (or unspecified from Shell)
* the default archive file name stored in global variable <TAPE> is used.
*
* The <bfactor> dictates the blocking factor the tape was written with.
* If 0, or unspecified from Shell, default of 20 is used.
*
* Archive contents are displayed on standard output, while
* all informative and eror message are printed to standard error.
*
*/

STATUS tarToc
    (
    char *	tape,		/* tape device name */
    int		bfactor 	/* requested blocking factor */
    )
    {
    register 	int rc ;		/* return codes */
    MT_TAR_SOFT	ctrl ;		/* control structure */
    STATUS	retval = OK;

    /* Set defaults */
    if( tape == NULL )
	tape = TAPE ;

    if( bfactor == 0 )
	bfactor = 20 ;

    printErr("Contents from %s\n", tape );

    bzero( (char *)&ctrl, sizeof(ctrl) );
    bzero( bZero, sizeof(bZero) );	/* not harmful for reentrancy */

    /* Open tape device and initialize control structure */
    if( strcmp(tape, "-") == 0)
	ctrl.fd = STD_IN;
    else
	ctrl.fd = open( tape, O_RDONLY, 0) ;

    if ( ctrl.fd < 0 )
	{
	printErr("Failed to open tape: %s\n", strerror(errnoGet()) );
	return ERROR;
	}
    
    ctrl.bFactor = bfactor ;
    ctrl.pBuf = KHEAP_ALLOC( bfactor * TBLOCK ) ;

    if ( ctrl.pBuf == NULL )
	{
	printErr("Not enough memory, exiting.\n" );
	if ( ctrl.fd != STD_IN )
	    close( ctrl.fd );
	return ERROR ;
	}

    /* all exits from now via goto in order to free the buffer */

    /* Read the first block and adjust blocking factor */

    rc = read( ctrl.fd, ctrl.pBuf->dummy, ctrl.bFactor*TBLOCK ) ;

    if ( rc == ERROR )
	{
	printErr("read error at the beginning of tape, exiting.\n" );
	retval = ERROR ;
	goto finish;
	}
    else if( rc == 0 )
	{
	printErr("empty tape file, exiting.\n" );
	goto finish;
	}
    else if( (rc % TBLOCK) != 0 )
	{
	printErr("tape block not multiple of %d, exiting.\n", TBLOCK);
	retval = ERROR ;
	goto finish;
	}

    ctrl.bValid = rc / TBLOCK ;
    ctrl.pBnext = ctrl.pBuf ;
    if ( ctrl.bFactor != ctrl.bValid )
	{
	printErr("adjusting blocking factor to %d\n", ctrl.bValid );
	ctrl.bFactor = ctrl.bValid ;
	}

    /* End of overture, start processing files until end of tape */

    FOREVER
	{
	MT_HBLOCK *	pBlk ;

	if ( tarRdBlks( &ctrl, &pBlk, 1) != 1 )
	    {
	    retval = ERROR ;
	    goto finish;
	    }

	if ( bcmp( pBlk->dummy, bZero, TBLOCK) == 0 )
	    {
	    printErr("end of tape encountered, read until eof...\n");
	    while( tarRdBlks( &ctrl, &pBlk, 1) > 0) ;
	    printErr("done.\n");
	    retval = OK ;
	    goto finish;
	    }

	if ( tarTocFile( &ctrl, pBlk) == ERROR )
	    {
	    retval = ERROR;
	    goto finish;
	    }

	} /* end of FOREVER */
    
finish:
	if ( ctrl.fd != STD_IN )
	    close( ctrl.fd );
	KHEAP_FREE((char *) ctrl.pBuf );
	return( retval );
    }

/* End of File */

