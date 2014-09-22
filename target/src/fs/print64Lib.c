/* print64Lib.c - pretty-print to stdout 64-bit integer values */

/* Copyright 1998-2002 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01f,17oct01,jkf   Cleaned !string literal is not checked  warning.
01e,29feb00,jkf   T3 merge, cleanup.
01d,31jul99,jkf   T2 merge, tidiness & spelling.
01c,08jul98,vld   print64Lib.h moved to h/private directory. 
01b,02jul98,lrn   ready for pre-release
01a,18jan98,vld	  written,
*/

/*
DESCRIPTION
This library is used internally by dosFsLib and is not intended
for general public consumption.
In a way, this is a temporary solution until standard printf() is
capable of 64 bit integers, but still this has many more bells.

LIMITATION
These functions always use printf() to produce output, always on
STD_OUT.

NOMANUAL
*/

/* includes */
#include "vxWorks.h"
#include "private/dosFsVerP.h"
#include "stdio.h"
#include "string.h"

#include "private/dosFsLibP.h"

LOCAL char * sufList[] = 	/* List of common suffixes */
    {
    NULL,
    " Kb",
    " Mb",
    " Gb",
    " Tb",
    NULL
    };

/*******************************************************************************
*
* print64 - print value.
*
* This routine outputs unsigned value onto stdout in radix based format.
* Supported radixes are 2, 10 and 16.
* If <groupeSize> != 0, it inserts separators ('.' for binary and
*  hexadecimal formats, ',' for decimal format) between every
* <groupeSize> ciphers..
* Strings <pHeader> and <pFooter> if not NULL, are printed ahead and after
* printed value.
*
* RETURNS: N/A.
*
* NOMANUAL
*/
void print64
    (
    char *      pHeader,		/* string to print before value */
    fsize_t	val,		/* should be log long */
    char *      pFooter,	/* string to print after value */
    u_int	radix,		/* 2, 10 or 16 */
    u_int	groupeSize
    )
    {
    int		shift;
    char        result[130];
    char *      pRes = result + sizeof( result ) - 1;
    char	separator = ' ';

    radix = ( radix != 0 )? radix : 10;
    if( radix != 2 && radix != 10 && radix != 16 )
    	{
    	printf( "Radix %d  not supported\n", radix );
    	return;
    	}
    if( radix == 2 || radix == 16 )
	separator = '.';
    else if( radix == 10 )
    	separator = ',';
    else
        {
        printf( "Radix %d  not supported\n", radix );
        return;
        }
    	
    groupeSize = (groupeSize != 0)? groupeSize : (-1);

    if( pHeader != NULL )
    	printf("%s", pHeader );
    
    if( val == 0 )
    	{
    	printf("0");
    	goto ret;;
    	}

    *pRes = EOS;
    for( shift = 1; val != 0; val /= radix, shift ++ )
    	{
    	pRes --;
    	*pRes = '0' + ( (u_int)val % radix );
    	if( *pRes > '9' )
    	    *pRes += 'a' - '9' - 1;
    	
    	if( shift % groupeSize == 0 )
    	    {
    	    pRes --;
    	    *pRes = separator;
    	    }
    	}

    if( *pRes == separator )
   	pRes ++;

    printf( "%s", pRes );

ret:
    if( pFooter != NULL )
    	printf("%s", pFooter );
    } /* print64() */
     
/*******************************************************************************
*
* print64Fine - print any unsigned value in pretty format.
*
* RETURNS: N/A.
*
* NOMANUAL
*/
void print64Fine
    (
    char *      pHeader,
    fsize_t 	val,
    char *      pFooter,
    u_int	radix
    )
    {
    u_int	groupeSize = 0;

    groupeSize = (radix == 2 )? 4 :( (radix == 10)? 3 : 0 );
    print64( pHeader, val, pFooter, radix, groupeSize );
    } /* print64Fine() */

/*******************************************************************************
*
* print64Row - row print any unsigned value.
*
* RETURNS: N/A.
*
* NOMANUAL
*/
void print64Row
    (
    char *      pHeader,
    fsize_t	val,
    char *      pFooter,
    u_int	radix
    )
    {
    print64( pHeader, val, pFooter, radix, (-1) );
    } /* print64Row() */

/*******************************************************************************
*
* print64Mult - output any unsigned value as multiple of 1024^n bytes.
*
* This routine prints value as up to thousands of bytes/KB/MB/GB/TB.
*
* RETURNS: N/A.
*
* NOMANUAL
*/
void print64Mult
    (
    char *	pHeader,
    fsize_t 	val,
    char *	pFooter
    )
    {
    int	shift;
    
    for( shift = 0;
         val  > 0x100000 && sufList[shift + 1] != NULL;
         shift++, val /= 1024 );

    print64( pHeader, val, sufList[shift], 10, 3 );

    if( pFooter != NULL )
    	printf("%s", pFooter );
    } /* print64Mult() */
/* End of File */

