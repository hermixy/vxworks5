/* ntlmssp.h - NT LM security headers */

/* Copyright (c) 1998 Wind River Systems, Inc. */

/*

modification history
--------------------
01p,13jul01,dbs  fix up includes
01o,18sep00,nel  SPR#33730. Merge T2 OPC fixes into T3 branch.
01n,09dec99,nel  Mods to DES routines
01m,05aug99,dbs  add mutex around channel-map
01l,19jul99,dbs  add more knowledge of response header layout
01k,09jul99,dbs  use new filenames
01j,06jul99,dbs  add accessors
01i,29jun99,dbs  add more details of protocol
01h,28jun99,dbs  remove defaultInstance method
01g,24jun99,dbs  implement more of NTLM protocol
01f,23jun99,dbs  create class for NTLM authn
01e,26may99,dbs  add extra magic number
01d,26may99,dbs  adding more knowledge of protocol
01c,10may99,dbs  remove refs to rpcDceLib
01b,11mar99,dbs  add more hooks for authentication
01a,15feb99,dbs  created

*/

/*
  DESCRIPTION:

  This file summarises what we know about the NTLMSSP security
  provider on Windows NT. The format of the packets is 'reverse
  engineered' from the wire, and assumptions have been made about the
  contents of some fields.

  The structure-field 'ntlmssp[8]' contains the 8 characters making up
  the NULL-terminated ASCII string "NTLMSSP". The sequence field
  contains a number which increments on every transaction.

  The 
*/


#ifndef __INCntlmssp_h
#define __INCntlmssp_h

#include <stdlib.h>
#include "rpcDceProto.h"
#include "dcomProxy.h"
#include "RpcPdu.h"
#include "private/comMisc.h"

//////////////////////////////////////////////////////////////////////////
//
// Public 'C' globals which can be set by Project-Facility
// configuration...
//

extern "C" DWORD g_defaultAuthnLevel;
extern "C" DWORD g_defaultImpLevel;

//////////////////////////////////////////////////////////////////////////
//
// A simple string-like structure to keep a username in...
//

struct username_t
    {
    char    txt [32];

    username_t (const char* s)
        { strncpy (txt, s, sizeof(txt)); }

    username_t (const username_t& x)
        { *this = x; }

    bool operator== (const username_t& x)
        { return (strcmp (txt, x.txt) == 0); }

    bool operator< (const username_t& x) const
        { return (strcmp (txt, x.txt) < 0); }

    username_t& operator= (const username_t& x)
        { strncpy (txt, x.txt, sizeof(txt)); return *this; }
        
    };
    
//////////////////////////////////////////////////////////////////////////
//
// NTLMSSP -- a class that encapsulates the NT Lan Manager Security
// Service Provider part of the DCOM protocol. The data-members are
// effectively set by calls to CoInitializeSecurity() and the only
// capabilities supported is EOAC_NONE in that API.
//

class NTLMSSP
    {
  public:
    
    // NTLMSSP ctor -- takes required authn and impersonation levels
    // for this particular instance of NTLMSSP service. The default
    // parameter values result in the defaults from the globals
    // 'g_defaultAuthnLevel' and 'g_defaultImpLevel' being assigned to
    // this instance...
    NTLMSSP
	(
	DWORD authnLvl = RPC_C_AUTHN_LEVEL_DEFAULT,
	DWORD impLvl = RPC_C_IMP_LEVEL_DEFAULT
	);

    ~NTLMSSP ();

    // serverBindValidate -- validates auth-trailer of received BIND
    // PDUs when this instance of NTMLSSP is representing the
    // server-side of a connection...
    HRESULT serverBindValidate
	(
	int		channelId,
	const RpcPdu&	bindPdu,
	RpcPdu&		reply
	);

    // serverRequestValidate -- validates auth-trailer of received
    // REQUEST PDUs when this instance of NTMLSSP is representing the
    // server-side of a connection...
    HRESULT serverRequestValidate
	(
	int		channelId,
	const RpcPdu&	reqPdu,
	RpcPdu&		reply
	);

    // serverAuth3Validate -- validates incoming auth-trailer in an
    // AUTH3 packet when this instance of NTLMSSP is representing the
    // server-side of a connection.
    HRESULT serverAuth3Validate
	(
	int		channelId,
	const RpcPdu&	auth3Pdu
	);

    // clientAuthnRequestAdd -- adds auth-trailer to an outgoing BIND
    // PDU appropriate to the current level of authentication
    // established for this instance of NTLMSSP...
    HRESULT clientAuthnRequestAdd
	(
	int 		channelId,
	RpcPdu&		bindPdu
	);

    // clientAuthnResponse -- adds auth-trailer to an outgoing AUTH3
    // PDU appropriate to the current level of authentication
    // established for this instance of NTLMSSP...
    HRESULT clientAuthnResponse
	(
	int 		channelId,
	const RpcPdu&	bindAckPdu,
	bool*		pbSendAuth3,
	RpcPdu&		auth3Pdu
	);

    // clientAuthn -- adds authn-trailer to client-side request
    // packets, this is just the 'token'...
    HRESULT clientAuthn
	(
	int 		channelId,
	RpcPdu&		reqPdu
	);
    
    // userAdd -- adds a user/password pair to the internal tables...
    static void userAdd (const char* userName, const char* userPasswd);

    // add/remove channel from table...
    void channelAdd (int channelId);
    void channelRemove (int channelId);

    // get status of a channel...
    HRESULT channelStatusGet (int cid)
	{ return m_channelStatus [cid]; }
    
    // set status of a channel...
    void channelStatusSet (int cid, HRESULT status)
	{ m_channelStatus [cid] = status; }

    // get current level
    DWORD authnLevel () const
	{ return m_authnLevel; }
    
  private:

    //////////////////////////////////////////////////////////////////////
    //
    // There are many items in these packet-layouts which are of the
    // same format, namely 2 shorts and a long, where the 2 shorts are
    // the length and max-length (always the same) of a field
    // (measured in bytes), and the long is the offset from the
    // beginning of the 'response' structure to that field. This
    // structure is described in the Samba documentation as a 'UNIHDR'
    // (Unicode String Header).
    //

    struct UNIHDR
	{
	USHORT	len;
	USHORT	max;
	ULONG	offset;
	};

    //////////////////////////////////////////////////////////////////////
    //
    // request -- client sends this request for authentication, in the
    // auth-trailer of an RPC PDU...
    //

    struct request
	{
	char	ntlmssp [8];		// string "NTLMSSP\0"
	ULONG	sequence;		// sequence number
	ULONG	ignored1;		// ignored
	UNIHDR	domainName;
	UNIHDR	machineName;
	char	text [1];		// machine+domain names
					// e.g. "BUFFYWRS-SWINDON"
	};

    //////////////////////////////////////////////////////////////////////
    //
    // challenge -- server responds with this challenge, in the
    // outgoing auth-trailer, in response to a received request...
    //

    struct challenge
	{
	char	ntlmssp [8];		// string "NTLMSSP\0"
	ULONG	sequence;		// sequence number
	ULONG	mbz1;			// unknown
	ULONG	magic1;			// value is NTLM_MAGIC1
	ULONG	magic2;			// value is NTLM_MAGIC2
	char	chal [8];		// 8-byte challenge
	ULONG	mbz2;			// unknown
	ULONG	mbz3;			// unknown
	};

    enum {
	NTLM_MAGIC1 = 0x00000028,
	NTLM_MAGIC2_BIND = 0x00108201,
	NTLM_MAGIC2_REQ = 0x00008201
    };

    //////////////////////////////////////////////////////////////////////
    //
    // response -- the client then sends this response, which should
    // include the 'response' to the previous challenge, issued by the
    // server. If this response is correct, then the client is
    // authenticated.
    //
    // The fields described in this way are the 2 encrypted blocks,
    // the domain-name, username, and machine-name.
    //

    struct response
	{
	char	ntlmssp [8];		// string "NTLMSSP\0"
	ULONG	sequence;		// sequence number
	UNIHDR	block1;
	UNIHDR	block2;
	UNIHDR	domainName;
	UNIHDR	userName;
	UNIHDR	machineName;
	ULONG	mbz1;			// MBZ
	ULONG	authLen;		// same as auth-len
	ULONG	magic2;			// value same as in BIND

	// ...followed by domain-user-machine as wide-string, followed
	// by 2 lots of 24-byte encrypted response data, of which the
	// first 24-byte block is the NTLM response. When we are
	// sending an AUTH3 we put the same into the second block as
	// we don't have the mechanism to encrypt whatever is supposed
	// to be in there.

	
	};

    HRESULT authTrailerCheck (const RpcPdu&);
    
    void encrypt (const BYTE* pwdHash, const BYTE* challenge, BYTE* p24);    

    static void ndrUnihdr (UNIHDR*, DREP);
    
    struct lmhash
	{
	BYTE	data [16];
	};

    enum { TOKEN_LEN=16 };
    
    typedef STL_MAP (username_t, lmhash) USERTABLE;
    typedef STL_MAP (int, HRESULT) STATUSMAP;
    
    DWORD	m_authnLevel;		// RPC_C_AUTHN_LEVEL_XXX
    DWORD	m_impLevel;		// RPC_C_IMPL_LEVEL_XXX
    STATUSMAP	m_channelStatus;	// channel => status
    VxMutex	m_mutex;		// thread safety for map
    
    static USERTABLE	s_userTable;	// table of user=>passwd-hash
    static BYTE		s_token [];	// NT token
    static BYTE		s_defaultChallenge [];


    //////////////////////////////////////////////////////////////////////
    // This class implements the Data Encryption Algorithm defined in
    // Federal Information Processing Standards Publication 46, 1977 January 15,
    // Specifications for the Data Encryption Standard. This algorithm is commonly
    // refered to as DES.
    //
    // This code does not implement all DES features. It only implements the
    // parts of the algorithm required by VxDCOM. It only implements an unchained
    // forward DES pass. This means that it can only be used as a hash algorithm
    // for validating Microsoft LAN Manager authentication.


    class DES
        {
        public:
            // 16 bit encrypt using standard LAN Manager key.
            static void encrypt16 (const BYTE * input, BYTE * output)
                {
                hash (output, lmKey, input);
                hash (output + 8, lmKey, input + 7);
                };

            // 24 bit encyrpt using any key.
            static void encrypt24 (const BYTE * input, const BYTE * key, BYTE * output)
                {
                hash (output, key, input);
                hash (output + 8, key, input + 7);
                hash (output + 16, key, input + 14);
                };

        private:

            enum { sRowSize = 4 };
            enum { sColSize = 16 };
            enum { sSize = 8 };
            enum { leftShiftSize = 16 };
            enum { ipSize = 64 };
            enum { ip1Size = 64 };
            enum { pSize = 32 };
            enum { eSize = 48 };
            enum { pc1Size = 56 };
            enum { pc2Size = 48 };
            enum { lmKeySize = 8 };


            enum { keySize = 8 };
            enum { bufferSize = 64 };


            // Permute table using supplied hashing table.
            static void permute
                (
                BYTE * output,      /* Output buffer */
                const BYTE * input, /* Input buffer */
                const BYTE * table, /* Permute table */ 
                int n               /* Table size */
                )
                {
                int i;

                for (i = 0; i < n; i++)
                    output [i] = input [table [i] - 1];
                }

            // Block left shift.
            static void lshift
                (
                unsigned char *p,   // Buffer
                int count,          // No of bits to shift left
                int n               // Buffer size
                )
                {
                char output [64];
                int i;

                for (i = 0; i < n; i++)
                    output [i] = p [(i + count) % n];
                for (i=0;i<n;i++)
                    p [i] = output [i];
                }

            // Standard DES tables as defined in Data Encryption Standard.

            static const BYTE s [sSize][sRowSize][sColSize]; // S1 to S8
            static const BYTE noOfLeftShifts [leftShiftSize];
            static const BYTE ip [ipSize];
            static const BYTE ip1 [ip1Size]; // IP^-1
            static const BYTE p [pSize];
            static const BYTE e [eSize];
            static const BYTE pc1 [pc1Size];
            static const BYTE pc2 [pc2Size];

            // LAN Manager decryption key.
            static const BYTE lmKey [lmKeySize];

            static void hash (BYTE * output, const BYTE * input, const BYTE * key);
            static void dohash (BYTE * output, const BYTE * input, const BYTE * key);
        };
    
    };

#endif


