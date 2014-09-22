/* ntlmssp.cpp - NT LM security implementation */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
01s,17dec01,nel  Add include symbol for diab.
01r,16jul01,dbs  fix unix-build includes
01q,13jul01,dbs  fix up includes
01p,09dec99,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01o,09dec99,nel  Mods to DES routines
01n,19aug99,aim  change assert to VXDCOM_ASSERT
01m,05aug99,dbs  add mutex around channel-map
01l,19jul99,dbs  add more knowledge of packet layouts
01k,13jul99,aim  syslog api changes
01j,28jun99,dbs  remove defaultInstance method
01i,24jun99,dbs  implement more of NTLM protocol
01h,23jun99,dbs  create class for NTLM authn
01g,26may99,dbs  adding more knowledge of protocol
01f,26apr99,aim  added TRACE_CALL
01e,20apr99,dbs  grand renaming
01d,09apr99,drm  added diagnostic output
01c,11mar99,dbs  add more hooks for authentication
01b,02mar99,dbs  remove printf
01a,15feb99,dbs  created

*/

#include <ctype.h>
#include "ntlmssp.h"
#include "SCM.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "private/comMisc.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_ntlmssp (void)
    {
    return 0;
    }


//////////////////////////////////////////////////////////////////////////
//
// Class statics...
//

NTLMSSP::USERTABLE NTLMSSP::s_userTable;

//////////////////////////////////////////////////////////////////////////

BYTE NTLMSSP::s_token [16] =
    { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

BYTE NTLMSSP::s_defaultChallenge [] = "GO AWAY";

//////////////////////////////////////////////////////////////////////////

const BYTE NTLMSSP::DES::lmKey [NTLMSSP::DES::lmKeySize] = 
    {0x4b, 0x47, 0x53, 0x21, 0x40, 0x23, 0x24, 0x25};

const BYTE NTLMSSP::DES::pc1 [NTLMSSP::DES::pc1Size] =
    {
    57, 49, 41, 33, 25, 17,  9,
    1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
    };

const BYTE NTLMSSP::DES::pc2 [NTLMSSP::DES::pc2Size] =
    {
    14, 17, 11, 24,  1,  5,
    3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
    };

const BYTE NTLMSSP::DES::e [NTLMSSP::DES::eSize] = 
    {
    32,  1,  2,  3,  4,  5,
    4,  5,  6,  7,  8,  9,
    8,  9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32,  1
    };

const BYTE NTLMSSP::DES::p [NTLMSSP::DES::pSize] = 
    {
    16,  7, 20, 21,
    29, 12, 28, 17,
    1, 15, 23, 26,
    5, 18, 31, 10,
    2,  8, 24, 14,
    32, 27,  3,  9,
    19, 13, 30,  6,
    22, 11,  4, 25
    };

const BYTE NTLMSSP::DES::ip1 [NTLMSSP::DES::ip1Size] =
    {
    40,  8, 48, 16, 56, 24, 64, 32,
    39,  7, 47, 15, 55, 23, 63, 31,
    38,  6, 46, 14, 54, 22, 62, 30,
    37,  5, 45, 13, 53, 21, 61, 29,
    36,  4, 44, 12, 52, 20, 60, 28,
    35,  3, 43, 11, 51, 19, 59, 27,
    34,  2, 42, 10, 50, 18, 58, 26,
    33,  1, 41,  9, 49, 17, 57, 25
    };

const BYTE NTLMSSP::DES::ip [ipSize] =
    {
    58, 50, 42, 34, 26, 18, 10,  2,
    60, 52, 44, 36, 28, 20, 12,  4,
    62, 54, 46, 38, 30, 22, 14,  6,
    64, 56, 48, 40, 32, 24, 16,  8,
    57, 49, 41, 33, 25, 17,  9,  1,
    59, 51, 43, 35, 27, 19, 11,  3,
    61, 53, 45, 37, 29, 21, 13,  5,
    63, 55, 47, 39, 31, 23, 15,  7
    };

const BYTE NTLMSSP::DES::noOfLeftShifts [NTLMSSP::DES::leftShiftSize] =
    {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

const BYTE NTLMSSP::DES::s [sSize][NTLMSSP::DES::sRowSize][NTLMSSP::DES::sColSize] =
    {
        {
            {14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7},
            {0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8},
            {4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0},
            {15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13}
        },

        {
            {15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10},
            {3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5},
            {0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15},
            {13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9}
        },

        {
            {10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8},
            {13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1},
            {13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7},
            {1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12}
        },

        {
            {7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15},
            {13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9},
            {10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4},
            {3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14}
        },

        {
            {2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9},
            {14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6},
            {4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14},
            {11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3}
        },

        {
            {12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11},
            {10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8},
            {9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6},
            {4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13}
        },

        {
            {4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1},
            {13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6},
            {1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2},
            {6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12}
        },

        {
            {13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7},
            {1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2},
            {7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8},
            {2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11}
        }
    };

/////////////////////////////////////////////////////////////////////
// 
// Des :: doHash - produce encrypted data from encryption tables
// 
// This routine takes a key and input data and encypts it using the
// DES algorithm.
// 
// RETURNS: nothing
// 

void NTLMSSP::DES::dohash
    (
    BYTE *          output, // encrypted data
    const BYTE *    input,  // input data
    const BYTE *    key     // encryption key
    )
    {
    int     i;              // various counts
    int     j;
    int     k;
    BYTE    fromPc1 [pc1Size];
    BYTE    firstHalf [pc1Size / 2];
    BYTE    secondHalf [pc1Size / 2];
    BYTE    shiftedPc1 [pc1Size];
    BYTE    fromPc2 [leftShiftSize][pc2Size];
    BYTE    pIp [ipSize];
    BYTE    pIpFirstHalf [ipSize / 2];
    BYTE    pIpSecondHalf [ipSize / 2];
    BYTE    fromIp [ipSize];

    // permute pc1 with supplied key
    permute(fromPc1, key, pc1, pc1Size);

    for (i = 0; i < pc1Size / 2; i++)
        firstHalf [i] = fromPc1 [i];

    for (i = 0; i < pc1Size / 2; i++)
        secondHalf [i] = fromPc1 [i + pc1Size / 2];

    for ( i = 0; i < leftShiftSize; i++)
        {
        lshift(firstHalf, noOfLeftShifts [i], pc1Size / 2);
        lshift(secondHalf, noOfLeftShifts [i], pc1Size / 2);

        // merge two tables together for permute
        memmove (shiftedPc1, firstHalf, pc1Size / 2);
        memmove (shiftedPc1 + pc1Size / 2, secondHalf, pc1Size / 2);
        permute(fromPc2 [i], shiftedPc1, pc2, pc2Size); 
        }

    permute(pIp, input, ip, ipSize);

    for (j = 0; j < ipSize / 2; j++) 
        {
        pIpFirstHalf [j] = pIp [j];
        pIpSecondHalf [j] = pIp [j + ipSize / 2];
        }

    for (i = 0; i < leftShiftSize; i++) 
        {
        BYTE pIpShE [eSize];
        BYTE pIpShEx [eSize];
        BYTE t [8][6];
        BYTE reverseT [pSize];
        BYTE reverseTp [pSize];
        BYTE t2 [pSize];
        int  x;                 // only use for block xor

        permute(pIpShE, pIpSecondHalf, e, eSize);

        for (x = 0; x < eSize; x++)
            pIpShEx [x] = pIpShE [x] ^ fromPc2 [i][x];

        for (j = 0; j < 8; j++)
            {
            for (k = 0; k < 6; k++)
                t [j][k] = pIpShEx [j * 6 + k];
            }

        for (j = 0; j < 8; j++) 
            {
            int m, n;
            m = (t [j][0] << 1) | t [j][5];

            n = (t [j][1] << 3) | (t [j][2] << 2) | (t [j][3] << 1) | t [j][4]; 

            for (k = 0; k < 4; k++) 
                t [j][k] = (s [j][m][n] & (1 << (3 - k))) ? 1 : 0; 
            }

        for (j = 0; j < 8; j++)
            {
            for (k = 0; k < 4; k++)
                reverseT [j * 4 + k] = t [j][k];
            }

        permute(reverseTp, reverseT, p, pSize);

        for (x = 0; x < pSize; x++)
            t2 [x] = pIpFirstHalf [x] ^ reverseTp [x];

        for (j = 0; j < ipSize / 2; j++)
            pIpFirstHalf [j] = pIpSecondHalf [j];

        for (j = 0; j < ipSize / 2; j++)
            pIpSecondHalf [j] = t2 [j];
    }

    // merge two tables together for permute
    memmove (fromIp, pIpSecondHalf, ipSize / 2);
    memmove (fromIp + ipSize / 2, pIpFirstHalf, ipSize / 2);

    permute(output, fromIp, ip1, ip1Size);
    }

/////////////////////////////////////////////////////////////////////
// 
// Des :: hash - prepares key for doHash and compresses resulting
//               data into 64 bit value.
// 
// This routine is a wrapper routine around doHash. It modifies the
// encryption key for use with doHash. It also compresses the resulting
// table into 64 bits.
// 
// RETURNS: nothing
// 

void NTLMSSP::DES::hash 
    (
    BYTE *          output,    // encrypted result
    const BYTE *    input,     // input data
    const BYTE *    key        // encryption key
    )
    {
    int             i;
    BYTE            outputBuffer [bufferSize];
    BYTE            inputBuffer [bufferSize];
    BYTE            keyBuffer [bufferSize];
    BYTE            encryptKey [keySize];

    encryptKey [0] = key [0] >> 1;
    encryptKey [1] = ((key [0] & 0x01) << 6) | (key [1] >> 2);
    encryptKey [2] = ((key [1] & 0x03) << 5) | (key [2] >> 3);
    encryptKey [3] = ((key [2] & 0x07) << 4) | (key [3] >> 4);
    encryptKey [4] = ((key [3] & 0x0F) << 3) | (key [4] >> 5);
    encryptKey [5] = ((key [4] & 0x1F) << 2) | (key [5] >> 6);
    encryptKey [6] = ((key [5] & 0x3F) << 1) | (key [6] >> 7);
    encryptKey [7] = key [6] & 0x7F;
    for (i = 0; i < 8; i++) 
        {
        encryptKey[i] = (encryptKey [i] <<1);
        }

    for (i = 0; i < 64; i++) 
        {
        // Load input and key buffers with cyclic repeat of encrypted key
        inputBuffer [i] = 
            (input [i / keySize] & (1 << (7 - (i % keySize)))) ? 1 : 0;
        keyBuffer [i] = 
            (encryptKey [i / keySize] & (1 << (7 - (i % keySize)))) ? 1 : 0;
        // zero output buffer
        outputBuffer [i] = 0;
        }

    dohash  (outputBuffer, 
            const_cast<const BYTE *> (inputBuffer), 
            const_cast<const BYTE *> (keyBuffer));

    // zero first 8 bytes
    for (i = 0; i < 8; i++) 
        {
        output [i] = 0;
        }

    // compress outputBuffer from 64 bytes to 64 bits
    for (i = 0; i < 64; i++) 
        {
        if (outputBuffer [i])
            output [i / 8] |= (1 << (7 - (i % 8)));
        }
    }

//////////////////////////////////////////////////////////////////////////


static void strupper (unsigned char *s)
    {
    while (*s)
	{
	if (islower (*s))
	    *s = toupper (*s);
	s++;
	}
    }

//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP ctor -- establishes the authn and impersonation levels for
// this particular instance...
//

NTLMSSP::NTLMSSP
    (
    DWORD	authnLvl,		// the authentication level
    DWORD	impLvl			// the impersonation level
    )
      : m_authnLevel (authnLvl),
	m_impLevel (impLvl)
    {
    if (m_authnLevel == RPC_C_AUTHN_LEVEL_DEFAULT)
	m_authnLevel = g_defaultAuthnLevel;
    if (m_impLevel == RPC_C_IMP_LEVEL_DEFAULT)
	m_impLevel = g_defaultImpLevel;
    }

//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP dtor...
//

NTLMSSP::~NTLMSSP ()
    {
    }

//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP::channelAdd -- adds an RPC-channel to the record, so its
// status can be maintained when a client attempts to authenticate
// over this channel...
//

void NTLMSSP::channelAdd (int cid)
    {
    // Default to ACCESS DENIED unless we are at lower level
    HRESULT hrAuthn = E_ACCESSDENIED;
    
    if (m_authnLevel == RPC_C_AUTHN_LEVEL_NONE)
	hrAuthn = S_OK;

    VxCritSec cs (m_mutex);
    m_channelStatus [cid] = hrAuthn;
    }


//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP::channelRemove -- removes a channel from the records...
//

void NTLMSSP::channelRemove (int cid)
    {
    VxCritSec cs (m_mutex);
    m_channelStatus.erase (cid);
    }


//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP::authTrailerCheck -- checks the auth-trailer of a received
// PDU to ensure its requesting a authn-lvel we can cope with...
//

HRESULT NTLMSSP::authTrailerCheck
    (
    const RpcPdu&	pdu		// received PDU
    )
    {
    // Make sure its NTLM protocol...
    if (pdu.authTrailer()->authType != RPC_C_AUTHN_WINNT)
	return MAKE_HRESULT (SEVERITY_ERROR,
			     FACILITY_RPC,
			     RPC_S_UNKNOWN_AUTHN_SERVICE);

    // Make sure its CONNECT-level authn being asked for...
    if (pdu.authTrailer()->authLevel != RPC_C_AUTHN_LEVEL_CONNECT)
	return MAKE_HRESULT (SEVERITY_ERROR,
			     FACILITY_RPC,
			     RPC_S_UNSUPPORTED_AUTHN_LEVEL);

    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP::serverRequestValidate -- validate the auth-trailer on a
// received RPC REQUEST PDU, which usually contains the 16-byte
// 'token' and nothing else...
//

HRESULT NTLMSSP::serverRequestValidate
    (
    int			cid,		// channel ID
    const RpcPdu&	reqPdu,		// rcvd REQUEST packet 
    RpcPdu&		reply		// reply to be sent
    )
    {
    if (! reqPdu.isRequest ())
	return E_FAIL;

    // Is there an auth-trailer?
    if (reqPdu.authLen ())
	{
	rpc_cn_auth_tlr_t 	 authTlrOut;
	const rpc_cn_auth_tlr_t* pAuthTlrIn = reqPdu.authTrailer ();
	
	// Look for familiar token in REQUEST packets..
	if ((reqPdu.authLen () == TOKEN_LEN) &&
	    (memcmp (pAuthTlrIn->authValue, s_token, TOKEN_LEN) == 0))
	    {
	    // Set outbound pre-amble fields...
	    authTlrOut.authType = RPC_C_AUTHN_WINNT;
	    authTlrOut.authLevel = RPC_C_AUTHN_LEVEL_CONNECT;
	    authTlrOut.stubPadLen = 0;
	    authTlrOut.reserved = 0;
	    authTlrOut.key = pAuthTlrIn->key;

	    // Copy 'token' into place...
	    memcpy (authTlrOut.authValue,
		    s_token,
		    TOKEN_LEN);
	    reply.authTrailerAppend (authTlrOut, TOKEN_LEN);

	    }
	}
    
    return S_OK;
    }


//////////////////////////////////////////////////////////////////////////
//
// serverBindValidate -- performs authentication checks on incoming
// BIND (or ALTER-CONTEXT) PDUs received by a server connection. The
// incoming PDU will contain the requesting machine and domain-name.
//
// Currently these fields are ignored, as we don't recognise NT
// domains or do anything with them, so we simply respond by returning
// a challenge. The difference between authenticating or not occurs
// when the AUTH3 PDU arrives, which we process in the method
// serverAuth3Validate().
//
// If we require authenticated connections, but we receive a BIND
// without an auth-trailer, then it must also be rejected, as we
// have been configured not to handle un-authenticated
// connections. Conversely, if we don't *require* authentication, but
// there is a request there in the packet, we still honour it by
// adding a challenge, i.e. we move up to the client's level of authn,
// but when his AUTH3 arrives we will simply accept it.
//

HRESULT NTLMSSP::serverBindValidate
    (
    int			cid,		// channel ID
    const RpcPdu&	bindPdu,	// rcvd BIND packet
    RpcPdu&		reply		// reply to be sent
    )
    {
    TRACE_CALL;

    // Only process BIND packets...
    if (! (bindPdu.isBind ()))
	return E_FAIL;

    // Get current status...
    VxCritSec cs (m_mutex);
    HRESULT status = m_channelStatus [cid];
    
    // Check what level we require...
    switch (m_authnLevel)
	{
    case RPC_C_AUTHN_LEVEL_NONE:
    case RPC_C_AUTHN_LEVEL_CONNECT:
	// This case handles both NONE and CONNECT levels, but treats
	// NONE as a special case when a received BIND PDU has an
	// auth-trailer, in that we respond to it with a challenge
	// (just like we do when we require authentication), but we
	// effectively ignore the response in the next AUTH3 packet.
	//
	// CONNECT-level is supported if the user/application has
	// registered some user/password combinations, so we *do*
	// require some authentication to be present, so if there's
	// none, we reject the PDU.
	//
	// For CONNECT level, we must defer the status-change (to S_OK
	// from ACCESS-DENIED) to the time we get the response to our
	// challenge, whereas for the NONE case we can simply let the
	// connection continue at S_OK...
	if (m_authnLevel == RPC_C_AUTHN_LEVEL_CONNECT)
	    status = E_ACCESSDENIED;
	else
	    status = S_OK;

	// Now check there is a trailer to process...
	if (bindPdu.authLen ())
	    {
	    // Check the auth-trailer for correctness...
	    HRESULT hr = authTrailerCheck (bindPdu);
	    if (FAILED (hr))
		status = hr;
	    else
		{
		// Okay -- now we can go ahead and prepare the reply
		// trailer for sending the challenge...
    
		rpc_cn_auth_tlr_t	authTlrOut;
		size_t			lenTlr=0;
		DREP			drep = bindPdu.drep ();
		const rpc_cn_auth_tlr_t* pAuthTlrIn = bindPdu.authTrailer ();
    
		// Set outbound pre-amble fields...
		authTlrOut.authType = RPC_C_AUTHN_WINNT;
		authTlrOut.authLevel = RPC_C_AUTHN_LEVEL_CONNECT;
		authTlrOut.stubPadLen = 0;
		authTlrOut.reserved = 0;
		authTlrOut.key = pAuthTlrIn->key;

		// Find request and challenge structures in PDUs...
		request* preq = (request*) pAuthTlrIn->authValue;
		challenge* pch = (challenge*) authTlrOut.authValue;
    
		// Find received sequence-number
		ULONG seq = preq->sequence;
		ndr_make_right (seq, drep);
    
		// Normal client request -- fill in codename
		strcpy (pch->ntlmssp, "NTLMSSP");

		// Look for repeated sequence-number 3 in ALTER-CONTEXT PDUs. If
		// its one of these then we can get away without putting an
		// auth-trailer on the response, and continue...
		if ((seq == 3) &&
		    (bindPdu.packetType () == RPC_CN_PKT_ALTER_CONTEXT))
		    status = S_OK;
		else
		    {
		    // Increment sequence-number...
		    pch->sequence = seq + 1;
		    ndr_make_right (pch->sequence, drep);

		    // Fill in fields and NDR them
		    pch->magic1 = NTLM_MAGIC1;
		    ndr_make_right (pch->magic1, drep);
		    pch->magic2 = NTLM_MAGIC2_BIND;
		    ndr_make_right (pch->magic2, drep);
	    
		    memcpy (pch->chal, s_defaultChallenge, 8);
		    pch->mbz1 = 0;
		    pch->mbz2 = 0;
		    pch->mbz3 = 0;

		    lenTlr = sizeof (NTLMSSP::challenge);
		    reply.authTrailerAppend (authTlrOut, lenTlr);
		    }
		}
	    }
	break;
	
    default:
	// Other levels we simply don't handle...
	status = E_ACCESSDENIED;
	}

    m_channelStatus [cid] = status;
    return status;
    }

//////////////////////////////////////////////////////////////////////////
//

void NTLMSSP::ndrUnihdr (UNIHDR* puh, DREP drep)
    {
    ndr_make_right (puh->len, drep);
    ndr_make_right (puh->max, drep);
    ndr_make_right (puh->offset, drep);
    }

//////////////////////////////////////////////////////////////////////////
//
// userAdd -- adds a user to the user-table, storing the 16-byte
// LM-hash rather than the plain-text password...
//
	
void NTLMSSP::userAdd (const char* userName, const char* userPasswd)
    {
    // Compute LM-hash of plain-text password...
    unsigned char ntlmPasswd [16];
   
    // Mangle the user-password into LanMan format (14 chars max, upper case)
    memset (ntlmPasswd, '\0', sizeof (ntlmPasswd));
    strncpy ((char*) ntlmPasswd, userPasswd, sizeof (ntlmPasswd));
    ntlmPasswd [14] = '\0';
    strupper (ntlmPasswd);
   
    // Calculate the SMB (lanman) hash function of the password
    NTLMSSP::lmhash ntlmHash;
    
    memset (&ntlmHash, 0, sizeof (ntlmHash));
    NTLMSSP::DES::encrypt16 (ntlmPasswd, ntlmHash.data);

    // Save the hash into the user-table...
    s_userTable [userName] = ntlmHash;
    
    // Erase the hash and plain-text from the stack
    memset (&ntlmHash, 0, 16);
    memset (ntlmPasswd, 0, 16);
    }


//////////////////////////////////////////////////////////////////////////
//
// encrypt -- encrypts the user's password-hash with the 8-byte
// challenge and produces a 24-byte response...
//

void NTLMSSP::encrypt
    (
    const BYTE*		pwdHash,	// the password-hash
    const BYTE*		challenge,	// the challenge
    BYTE*		p24		// output
    )
    {
    BYTE p21 [21];
 
    memset (p21,'\0',21);
    memcpy (p21, pwdHash, 16);    

    NTLMSSP::DES::encrypt24 (p21, const_cast<BYTE*> (challenge), p24);
    }

//////////////////////////////////////////////////////////////////////////
//
// serverAuth3Validate -- this function is called when an AUTH3 PDU is
// received at the server-side of a connection. This will happen when
// the client has attempted to open an authenticated connection to
// this machine, and we have sent it a challenge. What we get back is
// the response to that challenge, which must match our calculated
// response if we are bothered about authentication at all. If we
// aren't bothered, (i.e. the authn-level is set to NONE) then we
// simply let the AUTH3 PDU 'pass through' as if it were okay...
//

HRESULT NTLMSSP::serverAuth3Validate
    (
    int			cid,		// channel ID
    const RpcPdu&	auth3Pdu	// rcvd AUTH3 packet
    )
    {
    // Get current status...
    VxCritSec cs (m_mutex);
    HRESULT status = m_channelStatus [cid];
    
    // Check how much authn we require...
    switch (m_authnLevel)
	{
    case RPC_C_AUTHN_LEVEL_NONE:
	// No authn required, we can let this one through...
	status = S_OK;
	break;
	
    case RPC_C_AUTHN_LEVEL_CONNECT:
	// Connect-level is supported if the user/application has
	// registered some user/password combinations, so we must
	// continue processing. Check the auth-trailer for
	// correctness...
	status = authTrailerCheck (auth3Pdu);
	if (SUCCEEDED (status))
	    {
	    // Check there is some data to deal with -- if we get to
	    // here then we have received an AUTH3 packet and so there
	    // *must* be some, if not we return an error-value and
	    // effectively reject the packet...
	    if (auth3Pdu.authLen () == 0)
		status = E_ACCESSDENIED;
	    else
		{
		// Find the start of the trailer data...
		const rpc_cn_auth_tlr_t* pAuthTlr = auth3Pdu.authTrailer ();
		response* phdr = (response*) pAuthTlr->authValue;

		// NDR the important fields...
		ndr_make_right (phdr->sequence, auth3Pdu.drep ());
		ndrUnihdr (&phdr->domainName, auth3Pdu.drep ());
		ndrUnihdr (&phdr->userName, auth3Pdu.drep ());
		ndrUnihdr (&phdr->machineName, auth3Pdu.drep ());
		ndr_make_right (phdr->magic2, auth3Pdu.drep ());
    
		int textBytes = phdr->domainName.len +
				phdr->userName.len +
				phdr->machineName.len;

		USHORT* pwsText = (USHORT*) (phdr+1);
		for (int t=0; t < textBytes / 2; ++t)
		    ndr_make_right (pwsText [t], auth3Pdu.drep ());

		// Extract user-name as ASCII string...
		char userName [32];
		int n = phdr->userName.len / 2;
		pwsText += (phdr->domainName.len / 2);

		for (int i=0; i < n; ++i)
		    userName [i] = (char) pwsText [i];
		userName [n] = 0;

		// Find the 'response' data block in the PDU
		BYTE* pdata = ((BYTE*) (phdr+1)) + textBytes;

		// Calculate what the response should be...
		BYTE calculatedResult[24];
		memset (calculatedResult, 0, 24);
		encrypt (s_userTable [userName].data,
			 s_defaultChallenge,
			 calculatedResult);

		// Did the encrypted response come out right?
		if (memcmp (pdata, calculatedResult, 24) != 0)
		    {
		    S_INFO (LOG_AUTH, "AUTH3:access denied:" << userName);
		    status = E_ACCESSDENIED;
		    }
		else
		    {
		    S_INFO (LOG_AUTH, "AUTH3:access granted:" << userName);
		    status = S_OK;
		    }
		}
	    }
	break;
	
    case RPC_C_AUTHN_LEVEL_CALL:
    case RPC_C_AUTHN_LEVEL_PKT:
    case RPC_C_AUTHN_LEVEL_PKT_INTEGRITY:
    case RPC_C_AUTHN_LEVEL_PKT_PRIVACY:
	// These levels are not supported...
	status = MAKE_HRESULT (SEVERITY_ERROR,
			       FACILITY_RPC,
			       RPC_S_UNSUPPORTED_AUTHN_LEVEL);

    case RPC_C_AUTHN_LEVEL_DEFAULT:
    default:
	// Unknown errors!
	COM_ASSERT (0);
	status = E_FAIL;
	}

    m_channelStatus [cid] = status;
    return status;
    }


//////////////////////////////////////////////////////////////////////////
//
// clientAuthnRequestAdd -- empty stub for now.
//

HRESULT NTLMSSP::clientAuthnRequestAdd
    (
    int 		channelId,
    RpcPdu&		bindPdu
    )
    {
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// clientAuthnResponse -- empty stub for now.
//

HRESULT NTLMSSP::clientAuthnResponse
    (
    int 		channelId,
    const RpcPdu&	bindAckPdu,
    bool*		pbSendAuth3,
    RpcPdu&		auth3Pdu
    )
    {
    *pbSendAuth3 = false;
    return S_OK;
    }

//////////////////////////////////////////////////////////////////////////
//
// clientAuthn -- empty stub for now.
//

HRESULT NTLMSSP::clientAuthn
    (
    int 		channelId,
    RpcPdu&		reqPdu
    )
    {
    return S_OK;
    }

    



