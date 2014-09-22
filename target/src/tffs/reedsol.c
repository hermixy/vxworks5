/* HOOK. Fixed comments; otherwise impossible to compile */
/*
 * $Log:   P:/user/amir/lite/vcs/reedsol.c_v  $
 * 
 *    Rev 1.8   10 Sep 1997 16:29:44   danig
 * Got rid of generic names
 * 
 *    Rev 1.7   19 Aug 1997 20:08:16   danig
 * Andray's changes
 *
 *    Rev 1.6   07 Jul 1997 15:22:36   amirban
 * Ver 2.0
 *
 *    Rev 1.5   29 May 1997 15:04:52   amirban
 * More comments
 *
 *    Rev 1.4   27 May 1997 11:10:12   danig
 * Changed far to FAR1 & log() to flog()
 *
 *    Rev 1.3   25 May 1997 19:16:38   danig
 * Comments
 *
 *    Rev 1.2   25 May 1997 16:42:34   amirban
 * Up-to-date
 *
 *    Rev 1.1   18 May 1997 17:56:36   danig
 * Changed NO_ERROR to NO_EDC_ERROR
 *
 *    Rev 1.0   08 Apr 1997 18:35:32   danig
 * Initial revision.
 */

/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-1997			*/
/*									*/
/************************************************************************/


#include "reedsol.h"

#define T 2			 /* Number of recoverable errors */
#define SYND_LEN (T*2)           /* length of syndrom vector */
#define K512  (((512+1)*8+6)/10) /* number of inf symbols for record
				    of 512 bytes (K512=411) */
#define N512  (K512 + SYND_LEN)  /* code word length for record of 512 bytes */
#define INIT_DEG 510
#define MOD 1023

static short  gfi(short val);
static short  gfmul( short f, short s );
static short  gfdiv( short f, short s );
static short  flog(short val);
static short  alog(short val);

/*------------------------------------------------------------------------------*/
/* Function Name: RTLeightToTen                                                 */
/* Purpose......: convert an array of five 8-bit values into an array of        */
/*                four 10-bit values, from right to left.                       */
/* Returns......: Nothing                                                       */
/*------------------------------------------------------------------------------*/
static void RTLeightToTen(char *reg8, unsigned short reg10[])
{
	reg10[0] =  (reg8[0] & 0xFF)       | ((reg8[1] & 0x03) << 8);
	reg10[1] = ((reg8[1] & 0xFC) >> 2) | ((reg8[2] & 0x0F) << 6);
	reg10[2] = ((reg8[2] & 0xF0) >> 4) | ((reg8[3] & 0x3F) << 4);
	reg10[3] = ((reg8[3] & 0xC0) >> 6) | ((reg8[4] & 0xFF) << 2);
}


#ifdef EDC_IN_SOFTWARE

/*----------------------------------------------------------------------------*/
/* Function Name: RTLtenToEight                                               */
/* Purpose......: convert an array of four 10-bit values into an array of     */
/*                five 8-bit values, from right to left.                      */
/* Returns......: Nothing                                                     */
/*----------------------------------------------------------------------------*/
static void RTLtenToEight(unsigned short reg10[], char reg8[])
{
	reg8[0] =                                reg10[0] & 0x00FF;
	reg8[1] = ((reg10[0] & 0x0300) >> 8) | ((reg10[1] & 0x003F) << 2);
	reg8[2] = ((reg10[1] & 0x03C0) >> 6) | ((reg10[2] & 0x000F) << 4);
	reg8[3] = ((reg10[2] & 0x03F0) >> 4) | ((reg10[3] & 0x0003) << 6);
	reg8[4] =  (reg10[3] & 0x03FC) >> 2;
}


/*----------------------------------------------------------------------------*/
/* Function Name: LTReightToTen                                               */
/* Purpose......: convert an array of five 8-bit values into an array of      */
/*                four 10-bit values, from left to right.                     */
/* Returns......: Nothing                                                     */
/*----------------------------------------------------------------------------*/
static void LTReightToTen(char reg8[], unsigned short reg10[])
{
	reg10[0] = ((reg8[0] & 0xFF) << 2) | ((reg8[1] & 0xC0) >> 6);
	reg10[1] = ((reg8[1] & 0x3F) << 4) | ((reg8[2] & 0xF0) >> 4);
	reg10[2] = ((reg8[2] & 0x0F) << 6) | ((reg8[3] & 0xFC) >> 2);
	reg10[3] = ((reg8[3] & 0x03) << 8) |  (reg8[4] & 0xFF);
}

/*----------------------------------------------------------------------------*/
/* Function Name: LTRtenToEight                                               */
/* Purpose......: convert an array of four 10-bit values into an array of     */
/*                five 8-bit values, from left to right.                      */
/* Returns......: Nothing                                                     */
/*----------------------------------------------------------------------------*/
static void LTRtenToEight(unsigned short reg10[], char reg8[])
{
	reg8[0]   =                             (reg10[0] & 0x03FC) >> 2;
	reg8[1] = ((reg10[0] & 0x0003) << 6) | ((reg10[1] & 0x03F0) >> 4);
	reg8[2] = ((reg10[1] & 0x000F) << 4) | ((reg10[2] & 0x03C0) >> 6);
	reg8[3] = ((reg10[2] & 0x003F) << 2) | ((reg10[3] & 0x0300) >> 8);
	reg8[4] =  (reg10[3] & 0x00FF);
}


/*----------------------------------------------------------------------------*/
/* Function Name: LTRarrayEightToTen                                          */
/* Purpose......: convert an array of 512 8-bit values into an array of       */
/*                415 10-bit values (from left to right). The 6 msb on the    */
/*                first 10-bit value are zero, an so are the 8 lsb on the     */
/*                411'th 10-bit value. The last four 10-bit values are zeroed.*/
/* Returns......: Nothing                                                     */
/*----------------------------------------------------------------------------*/
static void LTRarrayEightToTen(char FAR1 *reg8, unsigned short *reg10)
{
  short i8;                               /* Index in the 8-bit array */
  short i10;                              /* Index in the 10-bit array */
  short j;

					  /* Create the first three 10-bit values */
					  /* (the six msb on the first 10-bit */
					  /* value are zeroes): */
	reg10[0] =                            (reg8[0] & 0xF0) >> 4;
	reg10[1] = ((reg8[0] & 0x0F) << 6) | ((reg8[1] & 0xFC) >> 2);
	reg10[2] = ((reg8[1] & 0x03) << 8) |  (reg8[2] & 0xFF);
					  /* Create the next 101 quadruples of */
					  /* 10-bit values: */
	for(i10=3, i8=3; i10 < (4*101)+3; i10+=4, i8+=5)
	{
		reg10[i10]   = ((reg8[i8]   & 0xFF) << 2) | ((reg8[i8+1] & 0xC0) >> 6);
		reg10[i10+1] = ((reg8[i8+1] & 0x3F) << 4) | ((reg8[i8+2] & 0xF0) >> 4);
		reg10[i10+2] = ((reg8[i8+2] & 0x0F) << 6) | ((reg8[i8+3] & 0xFC) >> 2);
		reg10[i10+3] = ((reg8[i8+3] & 0x03) << 8) |  (reg8[i8+4] & 0xFF);
	}
					  /* Create the last quadruple of 10-bit */
					  /* values. The the 8 lsb of the */
					  /* last 10-bit value are zeroed: */
	reg10[i10]   = ((reg8[i8]   & 0xFF) << 2) | ((reg8[i8+1] & 0xC0) >> 6);
	reg10[i10+1] = ((reg8[i8+1] & 0x3F) << 4) | ((reg8[i8+2] & 0xF0) >> 4);
	reg10[i10+2] = ((reg8[i8+2] & 0x0F) << 6) | ((reg8[i8+3] & 0xFC) >> 2);
	reg10[i10+3] =  (reg8[i8+3] & 0x03) << 8;
					/* Zero the last four 10-bit values: */
  i10+=4;
  for(j=i10; j<(i10+4); j++)
    reg10[j] = 0;
}


/*----------------------------------------------------------------------------*/
/* Function Name: resolveParity                                               */
/* Purpose......: find the parity byte for an array of bytes.                 */
/* Returns......: The parity byte                                             */
/*----------------------------------------------------------------------------*/
static char resolveParity(char FAR1 *block, short len)
{
  char par=0;
  short i;

  for(i=0; i<len; i++)
    par ^= block[i];

  return par;
}

#endif   /* EDC_IN_SOFTWARE */


/*----------------------------------------------------------------------------*/
static void unpack( short word, short length, short vector[] )
/*                                                                            */
/*   Function unpacks word into vector                                        */
/*                                                                            */
/*   Parameters:                                                              */
/*     word   - word to be unpacked                                           */
/*     vector - array to be filled                                            */
/*     length - number of bits in word                                        */

{
  short i, *ptr;

  ptr = vector + length - 1;
  for( i = 0; i < length; i++ )
  {
    *ptr-- = word & 1;
    word >>= 1;
  }
}


/*----------------------------------------------------------------------------*/
static short pack( short *vector, short length )
/*                                                                            */
/*   Function packs vector into word                                          */
/*                                                                            */
/*   Parameters:                                                              */
/*     vector - array to be packed                                            */
/*     length - number of bits in word                                        */

{
  short tmp, i;

  vector += length - 1;
  tmp = 0;
  i = 1;
  while( length-- > 0 )
  {
    if( *vector-- )
      tmp |= i;
    i <<= 1;
  }
  return( tmp );
}


/*----------------------------------------------------------------------------*/
static short gfi( short val)		/* GF inverse */
{
  return alog(MOD-flog(val));
}


/*----------------------------------------------------------------------------*/
static short gfmul( short f, short s ) /* GF multiplication */
{
  short i;
  if( f==0 || s==0 )
     return 0;
  else
  {
    i = flog(f) + flog(s);
    if( i > MOD ) i -= MOD;
    return( alog(i) );
  }
}


/*----------------------------------------------------------------------------*/
static short gfdiv( short f, short s ) /* GF division */
{
  return gfmul(f,gfi(s));
}


#ifdef EDC_IN_SOFTWARE

/*----------------------------------------------------------------------------*/
static void register_division( short reg[], short input[], short len )
/*                                                                            */
/*  Function emulates hardware division by generator polynom                  */
/*                                                                            */
/*  Parameters:                                                               */
/*    reg   - hardware register with feedback                                 */
/*    input - input sequence ( devidend )                                     */
/*    len   - number of steps for division procedure                          */

{
  short i, j, feedback, tmp0, tmp1, tmp4;

	for( i = 0; i < SYND_LEN; i++ )  reg[i] = 0x03ff;

	for( i = 0; i < len; i++ )
	{
		tmp4 = reg[0] ^ input[i];
		if( tmp4 == 0 )
		{
			reg[0] = reg[1];
			reg[1] = reg[2];
			reg[2] = reg[3];
			reg[3] = 0;
		}
		else
		{
      feedback = flog(tmp4);
			if( (j = feedback + 741) >= MOD )
	      j -= MOD;
      tmp0 = alog(j);

			if( (j = feedback +  85) >= MOD )
	j -= MOD;
      tmp1 = alog(j);


			reg[0] = reg[1] ^ tmp0;
			reg[1] = reg[2] ^ tmp1;
			reg[2] = reg[3] ^ tmp0;
			reg[3] = tmp4;
		}
	}
}

#endif  /* EDC_IN_SOFTWARE */


/*----------------------------------------------------------------------------*/
static void residue_to_syndrom( short reg[], short realsynd[] )
{
   short i,l,alpha,x,s,x4;
   short deg,deg4;


   for(i=0,deg=INIT_DEG;i<SYND_LEN;i++,deg++)
   {
      s = reg[0];
      alpha = x = alog(deg);
      deg4 = deg+deg;
      if( deg4 >= MOD ) deg4 -= MOD;
      deg4 += deg4;
      if( deg4 >= MOD ) deg4 -= MOD;
      x4 = alog(deg4);

      for(l=1;l<SYND_LEN;l++)
      {
	s ^= gfmul( reg[l], x );
	x  = gfmul( alpha, x );
      }

      realsynd[i] = gfdiv( s, x4 );
   }
}


/*----------------------------------------------------------------------------*/
static short alog(short i)
{
  short j=0, val=1;

  for( ; j < i ; j++ )
  {
    val <<= 1 ;

    if ( val > 0x3FF )
    {
      if ( val & 8 )   val -= (0x400+7);
      else             val -= (0x400-9);
    }
  }

  return val ;
}


static short flog(short val)
{
  short j, val1;

  if (val == 0)
    return (short)0xFFFF;

  j=0;
  val1=1;

  for( ; j <= MOD ; j++ )
  {
    if (val1 == val)
      return j;

    val1 <<= 1 ;

    if ( val1 > 0x3FF )
    {
      if ( val1 & 8 )   val1 -= (0x400+7);
      else              val1 -= (0x400-9);
    }

  }

  return 0;
}


#ifdef EDC_IN_SOFTWARE

static short packed_symbol[415];/* RS code word packed bit in bit */

/*----------------------------------------------------------------------------*/
/* Function Name: encodeEDC                                                   */
/* Purpose......: Encode 512 bytes of data, given in block[].                 */
/*                Upon returning code[] will contain the EDC code as          */
/*                5 bytes, and one parity byte.                               */
/* Returns......: Nothing                                                     */
/*----------------------------------------------------------------------------*/
static void encodeEDC(char FAR1 *block, char code[6])
{
  short reg[SYND_LEN];         /* register for main division procedure */
					/* Convert block[] into a 10-bit values */
					/* array: */
  LTRarrayEightToTen(block, (unsigned short *)packed_symbol);
					/* Calculate the parity value of the 512 */
					/* bytes array, and store it in the 8 */
					/* lsb of the 410'th 10-bit value: */
  packed_symbol[410] |= (resolveParity(block, 512) & 0xFF);

					/* Calculate the EDC code: */
	register_division(reg, packed_symbol, K512);
					/* Convert the EDC code from four */
					/* 10-bit values, into five bytes: */
  LTRtenToEight((unsigned short *)reg, code);
					/* Store the parity byte in the last */
					/* element of code[]: */
  code[5] = (char)(packed_symbol[410] & 0xFF);
}


/*------------------------------------------------------------------------------*/
/* Function Name: flCalcEDCSyndrom                                              */
/* Purpose......: Trys to detect errors.                                        */
/*                block[] should contain 512 bytes of data.                     */
/*                code[] should contain the EDC code as 5 bytes, and one        */
/*                parity byte (identical to the output of EDCencode::resolve()).*/
/*                Upon returning errorSyndrom[] will contain the syndrom as     */
/*                5 bytes and one       parity byte (identical to the input of  */
/*                EDCdecode::resolve()).                                        */
/* Returns......: The error status.                                             */
/*                NOTE! If the error status is NO_EDC_ERROR upon return, ignore */
/*                      the value of the arguments.                             */
/*------------------------------------------------------------------------------*/
EDCstatus flCalcEDCSyndrom(char FAR1 *block, char code[6], char errorSyndrom[6])
{
  short nonZero, i;
  short residue[SYND_LEN];
  short reg[SYND_LEN];         /* register for main division procedure */

					/* Convert block[] into a 10-bit values */
					/* array: */
  LTRarrayEightToTen(block, (unsigned short *)packed_symbol);

  packed_symbol[410] |= code[5] & 0xFF; /* Copy the parity value from code[] */
					/* into the 8 lsb of the 410'th */
					/* 10-bit value */

					/* Copy the EDC code into the last four */
					/* 10-bit values of packed_symbol: */
  LTReightToTen(code, (unsigned short *)&packed_symbol[411]);
					/* Calculate the syndrom (the result */
					/* is in reg[]): */
  register_division( reg, packed_symbol, N512 );

  for(i=nonZero=0; i<SYND_LEN; i++)     /* Chack if reg[] is all zeroes */
    if(reg[i])
      nonZero++;

  if(nonZero == 0)                      /* If reg[] is all zeroes, there is */
  {                                     /* no error */
    return NO_EDC_ERROR;                /* No error found */
  }

  for( i = 0; i < SYND_LEN; i++ )       /* reverse the order of reg[] */
    residue[SYND_LEN-1-i] = reg[ i ];   /* into residue[] */

					/* Convert the syndrom (in residue[]) */
					/* from four 10-bit values into five */
					/* bytes: */
  RTLtenToEight((unsigned short *)residue, errorSyndrom);
					/* Calculate the parity byte for the */
					/* original 512 bytes plus the parity */
					/* byte that was received in code[]: */
  errorSyndrom[5] = resolveParity(block, 512) ^ (code[5] & 0xFF);

  return EDC_ERROR;                     /* An error was detected */
}

#endif  /* EDC_IN_SOFTWARE */

/*----------------------------------------------------------------------------*/
static short convert_to_byte_patterns( short *locators, short *values,
				short noferr, short *blocs, short *bvals )
{
  static short mask[] = { 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

  short i,j,n, n0, n1, tmp;
  short n_bit, n_byte, k_bit, nb;

  for( i = 0, nb = 0; i< noferr; i++)
  {
    n = locators[i];
    tmp = values[i];
    n_bit = n *10 - 6 ;
    n_byte = n_bit >> 3;
    k_bit  = n_bit - (n_byte<<3);
    n_byte++;
    if( k_bit == 7 )
    {
      /* 3 corrupted bytes */
      blocs[nb] = n_byte+1;
      bvals[nb++] = tmp & 1 ? 0x80 : 0;

      tmp >>= 1;
      blocs[nb] = n_byte;
      bvals[nb++] = tmp & 0xff;

      tmp >>= 8;
      bvals[nb++] = tmp & 0xff;
    }
    else
    {
      n0 = 8 - k_bit;
      n1 = 10 - n0;

      blocs[nb] = n_byte;
      bvals[nb++] = (tmp & mask[n1]) << (8 - n1);

      tmp >>= n1;
      blocs[nb] = n_byte - 1;
      bvals[nb++] = (tmp & mask[n0]);
    }
  }

  for( i = 0, j = -1; i < nb; i++ )
  {
    if( bvals[i] == 0 ) continue;
    if( (blocs[i] == blocs[j]) && ( j>= 0 ) )
    {
      bvals[j] |= bvals[i];
    }
    else
    {
      j++;
      blocs[j] = blocs[i];
      bvals[j] = bvals[i];
    }
  }
  return j+1;
}


/*----------------------------------------------------------------------------*/
static short deg512( short x )
{
  short i;
  short l,m;

  l = flog(x);
  for( i=0;i<9;i++)
  {
    m = 0;
    if( (l & 0x200) )
      m = 1;
    l =  ( ( l << 1 ) & 0x3FF  ) | m;
  }
  return alog(l);
}


/*----------------------------------------------------------------------------*/
static short decoder_for_2_errors( short s[], short lerr[], short verr[] )
{
  /* decoder for correcting up to 2 errors */
  short i,j,k,temp,delta;
  short ind, x1, x2;
  short r1, r2, r3, j1, j2;
  short sigma1, sigma2;
  short xu[10], ku[10];
  short yd, yn;

  ind = 0;
  for(i=0;i<SYND_LEN;i++)
    if( s[i] != 0 )
      ind++;                /* ind = number of nonzero syndrom symbols */

  if( ind == 0 ) return 0;  /* no errors */

  if( ind < 4 )
    goto two_or_more_errors;


/* checking s1/s0 = s2/s1 = s3/s2 = alpha**j for some j */

  r1 = gfdiv( s[1], s[0] );
  r2 = gfdiv( s[2], s[1] );
  r3 = gfdiv( s[3], s[2] );

  if( r1 != r2 || r2 != r3)
    goto two_or_more_errors;

  j = flog(r1);
  if( j > 414 )
    goto two_or_more_errors;

  lerr[0] = j;

/*  pattern = (s0/s1)**(510+1) * s1

	  or

    pattern = (s0/s1)**(512 - 1 )  * s1 */

  temp = gfi( r1 );
  {
    int i;

    for (i = 0; i < 9; i++)
      temp = gfmul( temp, temp );  /* deg = 512 */
  }

  verr[0] = gfmul( gfmul(temp, r1), s[1] );

  return 1;    /* 1 error */

two_or_more_errors:

  delta = gfmul( s[0], s[2] ) ^ gfmul( s[1], s[1] );

  if( delta == 0 )
    return -1;  /* uncorrectable error */

  temp = gfmul( s[1], s[3] ) ^ gfmul( s[2], s[2] );

  if( temp == 0 )
    return -1;  /* uncorrectable error */

  sigma2 = gfdiv( temp, delta );

  temp = gfmul( s[1], s[2] ) ^ gfmul( s[0], s[3] );

  if( temp == 0 )
    return -1;  /* uncorrectable error */

  sigma1 = gfdiv( temp, delta );

  k = gfdiv( sigma2, gfmul( sigma1, sigma1 ) );

  unpack( k, 10, ku );

  if( ku[2] != 0 )
    return -1;

  xu[4] = ku[9];
  xu[5] = ku[0] ^ ku[1];
  xu[6] = ku[6] ^ ku[9];
  xu[3] = ku[4] ^ ku[9];
  xu[1] = ku[3] ^ ku[4] ^ ku[6];
  xu[0] = ku[0] ^ xu[1];
  xu[8] = ku[8] ^ xu[0];
  xu[7] = ku[7] ^ xu[3] ^ xu[8];
  xu[2] = ku[5] ^ xu[7] ^ xu[5] ^ xu[0];
  xu[9] = 0;

  x1 = pack( xu, 10 );
  x2 = x1 | 1;

  x1 = gfmul( sigma1, x1 );
  x2 = gfmul( sigma1, x2 );


  j1 = flog(x1);
  j2 = flog(x2);

  if( (j1 > 414) || (j2 > 414) )
    return -1;


  r1 = x1 ^ x2;
  r2 = deg512( x1 );
  temp = gfmul( x1, x1 );
  r2 = gfdiv( r2, temp );
  yd = gfmul( r2, r1 );

  if( yd == 0 )
    return -1;

  yn = gfmul( s[0], x2 ) ^ s[1];
  if( yn == 0 )
    return -1;

  verr[0] = gfdiv( yn, yd );

  r2 = deg512( x2 );
  temp = gfmul( x2, x2 );
  r2 = gfdiv( r2, temp );
  yd = gfmul( r2, r1 );

  if( yd == 0 )
    return -1;

  yn = gfmul( s[0], x1 ) ^ s[1];
  if( yn == 0 )
    return -1;

  verr[1] = gfdiv( yn, yd );

  if( j1 > j2 ) {
    lerr[0] = j2;
    lerr[1] = j1;
    temp = verr[0];
    verr[0] = verr[1];
    verr[1] = temp;
  }
  else
  {
    lerr[0] = j1;
    lerr[1] = j2;
  }

  return 2;
}


/*------------------------------------------------------------------------------*/
/* Function Name: flDecodeEDC                                                   */
/* Purpose......: Trys to correct errors.                                       */
/*                errorSyndrom[] should contain the syndrom as 5 bytes and one  */
/*                parity byte. (identical to the output of calcEDCSyndrom()).   */
/*                Upon returning, errorNum will contain the number of errors,   */
/*                errorLocs[] will contain error locations, and                 */
/*                errorVals[] will contain error values (to be XORed with the   */
/*                data).                                                        */
/*                Parity error is relevant only if there are other errors, and  */
/*                the EDC code fails parity check.                              */
/*                NOTE! Only the first errorNum indexes of the above two arrays */
/*                      are relevant. The others contain garbage.               */
/* Returns......: The error status.                                             */
/*                NOTE! If the error status is NO_EDC_ERROR upon return, ignore */
/*                      the value of the arguments.                             */
/*------------------------------------------------------------------------------*/
EDCstatus flDecodeEDC(char *errorSyndrom, char *errorsNum,
		    short errorLocs[3*T],  short errorVals[3*T])
{
  short noferr;                         /* number of errors */
  short dec_parity;                     /* parity byte of decoded word */
  short rec_parity;                     /* parity byte of received word */
  short realsynd[SYND_LEN];             /* real syndrom calculated from residue */
  short locators[T],                    /* error locators */
  values[T];                            /* error values */
  short reg[SYND_LEN];                  /* register for main division procedure */
  int i;

  RTLeightToTen(errorSyndrom, (unsigned short *)reg);
  rec_parity = errorSyndrom[5] & 0xFF;  /* The parity byte */

  residue_to_syndrom(reg, realsynd);
  noferr = decoder_for_2_errors(realsynd, locators, values);

  if(noferr == 0)
    return NO_EDC_ERROR;                /* No error found */

  if(noferr < 0)                        /* If an uncorrectable error was found */
    return UNCORRECTABLE_ERROR;

  for (i=0;i<noferr;i++)
    locators[i] = N512 - 1 - locators[i];

  *errorsNum = (char)convert_to_byte_patterns(locators, values, noferr, errorLocs, errorVals);

  for(dec_parity=i=0; i < *errorsNum; i++)/* Calculate the parity for all the */
  {                                       /*   errors found: */
    if(errorLocs[i] <= 512)
      dec_parity ^= errorVals[i];
  }

  if(dec_parity != rec_parity)
    return UNCORRECTABLE_ERROR;         /* Parity error */
  else
    return CORRECTABLE_ERROR;
}


/*------------------------------------------------------------------------------*/
/* Function Name: flCheckAndFixEDC                                              */
/* Purpose......: Decodes the EDC syndrom and fixs the errors if possible.      */
/*                block[] should contain 512 bytes of data.                     */
/*                NOTE! Call this function only if errors where detected by     */
/*                      syndCalc or by the ASIC module.                         */
/* Returns......: The error status.                                             */
/*------------------------------------------------------------------------------*/
EDCstatus flCheckAndFixEDC(char FAR1 *block, char *syndrom, FLBoolean byteSwap)
{
  char errorsNum;
  short errorLocs[3*T];
  short errorVals[3*T];
  EDCstatus status;

  status = flDecodeEDC(syndrom, &errorsNum, errorLocs, errorVals);

  if(status == CORRECTABLE_ERROR)       /* Fix the errors if possible */
  {
    int i;

    for (i=0; i < errorsNum; i++)
      block[errorLocs[i] ^ byteSwap] ^= errorVals[i];

    return NO_EDC_ERROR;                /* All errors are fixed */
  }
  else
    return status;                      /* Uncorrectable error */
}
