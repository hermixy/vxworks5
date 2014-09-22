/* wavFormat.h - WAV audio file format */

/* Copyright 2000 Wind River Systems, Inc. */

/*
Modification history
--------------------
01a,14jan00,rcb  First.
*/

#ifndef __INCwavFormath
#define __INCwavFormath

#ifdef	__cplusplus
extern "C" {
#endif


/* RIFF file header */

#define RIFF_HDR_SIG		"RIFF"
#define RIFF_HDR_SIG_LEN	4

typedef struct riff_hdr 
    {
    char signature [4]; 	    /* identifier string = "RIFF" */
    UINT32 length;		    /* remaining length after this header */
    } RIFF_HDR, *pRIFF_HDR;


/* RIFF .wav file signature */

#define RIFF_WAV_DATA_SIG	"WAVE"
#define RIFF_WAV_DATA_SIG_LEN	4


/* RIFF chunk header */

#define RIFF_CHUNK_ID_LEN	4

typedef struct riff_chunk_hdr 
    {
    char chunkId [RIFF_CHUNK_ID_LEN]; /* ID of chunk */
    UINT32 length;		    /* remaining length after header */
    } RIFF_CHUNK_HDR, *pRIFF_CHUNK_HDR;


/* .wav format chunk */

#define RIFF_WAV_FMT_CHUNK_ID	"fmt "

typedef struct wav_format_chunk 
    {
    UINT16 formatTag;		    /* format category */
    UINT16 channels;		    /* number of channels */
    UINT32 samplesPerSec;	    /* sampling rate */
    UINT32 avgBytesPerSec;	    /* used to estminate bfr requirements */
    UINT16 blockAlign;		    /* data block size */
    union 
	{
	struct			    /* MS PCM-specific data */
	    {			    
	    UINT16 bitsPerSample;   /* sample size */
	    } msPcm;
	} fmt;
    } WAV_FORMAT_CHUNK, *pWAV_FORMAT_CHUNK;


/* .wav format categories */

#define WAV_FMT_MS_PCM	    0x0001  /* Microsoft PCM */
#define WAV_FMT_IBM_MULAW   0x0101  /* IBM mul-law */
#define WAV_FMT_IBM_ALAW    0x0102  /* IBM a-law */
#define WAV_FMT_IBM_ADPCM   0x0103  /* IBM AVC ADPCM */


/* data chunk */

#define RIFF_WAV_DATA_CHUNK_SIG "data"


#ifdef	__cplusplus
}
#endif

#endif	/* __INCwavFormath */


/* End of file. */

