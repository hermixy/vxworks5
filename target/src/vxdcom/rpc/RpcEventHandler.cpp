/* RpcEventHandler */

/* Copyright (c) 1999 Wind River Systems, Inc. */

/*
modification history
--------------------
01x,17dec01,nel  Add include symbol for diab.
01w,02oct01,nel  Move debug hooks into SockStream class.
01v,25sep01,nel  Correct prototype error under test harness build.
01u,10sep01,nel  Add dcomShow hooks into EventHandler.
01t,03aug01,dbs  remove usage of Thread class
01s,13jul01,dbs  fix up includes
01r,26jun00,dbs  implement presentation context IDs
01q,24may00,dbs  add fault diagnostics
01p,19aug99,aim  change assert to VXDCOM_ASSERT
01o,30jul99,aim  added thread pooling
01n,13jul99,aim  syslog api changes
01m,09jul99,dbs  use final filenames
01l,09jul99,dbs  tidy up logging of packets
01k,07jul99,aim  added ostream operator<<
01j,02jul99,aim  fix for name changes in RpcPduFactory
01i,28jun99,dbs  remove defaultInstance method
01h,28jun99,dbs  make sure authn-status is preserved after BIND PDU
01g,24jun99,dbs  add authn checking
01f,24jun99,dbs  move authn into new class
01e,22jun99,dbs  fix includes again
01d,18jun99,aim  set data rep on outgoing packets
01c,08jun99,aim  rework
01b,08jun99,aim  now uses NRpcPdu
01a,27may99,aim  created
*/

#include "RpcEventHandler.h"
#include "RpcDispatcher.h"
#include "RpcDispatchTable.h"
#include "RpcPduFactory.h"
#include "RpcIfServer.h"
#include "Reactor.h"
#include "SCM.h"
#include "Syslog.h"
#include "TraceCall.h"
#include "private/comMisc.h"
#include "private/DebugHooks.h"
#include "taskLib.h"

/* Include symbol for diab */
extern "C" int include_vxdcom_RpcEventHandler (void)
    {
    return 0;
    }

RpcEventHandler::~RpcEventHandler ()
    {
    TRACE_CALL;

    S_INFO (LOG_RPC, "disconnect: " << (*this));

    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->channelRemove (channelId ());

    DELZERO (m_pdu);
    }

RpcEventHandler::RpcEventHandler (Reactor* reactor)
  : SvcHandler<SockStream> (reactor),
    m_pdu (0),
    m_hostAddr (),
    m_peerAddr (),
    m_dispatcher (0),
    m_creatorTaskId (::taskIdSelf ()),
    m_acceptor (0)
    {
    TRACE_CALL;

    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->channelAdd (channelId ());
    }

RpcEventHandler::RpcEventHandler
    (
    Reactor*		reactor,
    RpcDispatcher*	dispatcher
    )
  : SvcHandler<SockStream> (reactor),
    m_pdu (0),
    m_hostAddr (),
    m_peerAddr (),
    m_dispatcher (dispatcher),
    m_creatorTaskId (::taskIdSelf ()),
    m_acceptor (0)
    {
    TRACE_CALL;

    COM_ASSERT (m_dispatcher);

    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->channelAdd (channelId ());
    }

int RpcEventHandler::open (void* pv)
    {
    acceptorSet (static_cast<RpcIfServer*> (pv));

    if (concurrency() == RpcIfServer::ThreadPerConnection)
	{
	COM_ASSERT (0);	// NYI
	}

    return super::open (pv);
    }

int RpcEventHandler::close (unsigned long flags)
    {
    return super::close (flags);
    }

const INETSockAddr&
RpcEventHandler::peerAddr ()
    {
    stream().peerAddrGet (m_peerAddr);
    return m_peerAddr;
    }

const INETSockAddr&
RpcEventHandler::hostAddr ()
    {
    stream().hostAddrGet (m_hostAddr);
    return m_hostAddr;
    }

int
RpcEventHandler::handleInput (REACTOR_HANDLE handle)
    {
    TRACE_CALL;

    if (concurrency () == RpcIfServer::ThreadPooled)
	{
	if (::taskIdSelf () == m_creatorTaskId)
	    {
	    reactorGet()->handlerRemove (this,
					 EventHandler::READ_MASK |
					 EventHandler::DONT_CALL);

	    return threadPool()->enqueue (this);
	    }
	}

    // Any strategy other than thread-per-connection will eventually get
    // here.  If we're in the single-threaded implementation or the
    // thread-pool, we still have to pass this way.

    int result = process ();

    // Now, we look again to see if we're in the thread-pool
    // implementation.  If so then we need to re-register ourselves with
    // the reactor so that we can get more work when it is available.

    if (concurrency () == RpcIfServer::ThreadPooled)
	{
	if (result != -1)
	    reactorGet()->handlerAdd (this, EventHandler::READ_MASK);
	}

    return result;
    }

int RpcEventHandler::process ()
    {
    TRACE_CALL;

    const int buflen = 1024;
    char buf [buflen];
    char* pbuf = buf;

    ssize_t n = stream().recv (pbuf, buflen);

    if (n > 0)
	{
	S_DEBUG (LOG_RPC, (*this) << " read: " << (int) n);
	/* We only process the first fragment of the packet because */
	/* RpcPdu doesn't store the data contiguiouly in memory at the */
	/* moment so we can't just feed the byte stream into the debug */
	/* hook */
	stream ().processDebugOutput (pRpcServerInput, (BYTE *)pbuf, n);
	}

    int result = -1; // guilty until proved innocent

    while (n > 0)
	{
	if (m_pdu == 0 && (m_pdu = new RpcPdu ()) == 0)
	    break;		// ENOMEM

	int consumed = m_pdu->append (pbuf, n);

	if (m_pdu->complete ())
	    {
	    result = dispatchPdu (*m_pdu);
	    DELZERO (m_pdu);
	    if (result != 0)
		break;
	    }
	else
	    {
	    S_DEBUG (LOG_RPC, "pdu not complete");
	    result = 0;		// hang on for more data
	    }

	// move offset into appended data
	pbuf += consumed;
	n -= consumed;
	}
    
    if (result < 0)
	DELZERO (m_pdu);

    return result;
    }
    
int
RpcEventHandler::dispatchAuth3 (const RpcPdu& auth3Pdu)
    {
    TRACE_CALL;

    NTLMSSP* ssp = SCM::ssp ();
    if (ssp)
	ssp->serverAuth3Validate (channelId (), auth3Pdu);

    return 0;
    }


int
RpcEventHandler::dispatchBind (const RpcPdu& bindPdu)
    {
    TRACE_CALL;

    if (m_dispatcher == 0)
	return -1;

    RpcPdu responsePdu;

    IID iid = bindPdu.bind().presCtxList.presCtxElem[0].abstractSyntax.id;
    USHORT presCtxId = bindPdu.bind().presCtxList.presCtxElem[0].presCtxId;

    // If the dispatcher supports this interface-ID, then we are bound
    // to it, via the given presentation-context ID...
    if (m_dispatcher->supportsInterface (iid))
	RpcPduFactory::formatBindAckPdu (bindPdu,
					 responsePdu,
					 reinterpret_cast<ULONG> (this));
    else
	RpcPduFactory::formatBindNakPdu (bindPdu, responsePdu);

    // Process authentication trailers...
    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->serverBindValidate (channelId (),
				 bindPdu,
				 responsePdu);

    // Reply to the BIND...
    int result = reply (bindPdu, responsePdu);

    // If successful, record the presentation context...
    if (result == 0)
	m_presCtxMap [presCtxId] = iid;

    return result;
    }

int
RpcEventHandler::dispatchRequest (const RpcPdu& requestPdu)
    {
    RpcPdu responsePdu;

    // Find which presentation context this request is for, and so,
    // which interface ID to use...
    USHORT presCtxId = requestPdu.request().presCtxId;
    PresCtxMap::const_iterator i = m_presCtxMap.find (presCtxId);
    if (i == m_presCtxMap.end ())
	return -1;
    
    // Use the presentation context to select the right IID...    
    m_dispatcher->dispatch (requestPdu,
			    responsePdu,
			    channelId (),
			    (*i).second);

    // Process authentication trailers...
    NTLMSSP* ssp = SCM::ssp ();

    if (ssp)
	ssp->serverRequestValidate (channelId (),
				    requestPdu,
				    responsePdu);
        
    return reply (requestPdu, responsePdu);
    }

int
RpcEventHandler::dispatchPdu (const RpcPdu& pdu)
    {
    TRACE_CALL;
    
    S_DEBUG (LOG_RPC, "recvPdu: " << pdu);

    if (pdu.isRequest ())
	return dispatchRequest (pdu);
    else if (pdu.isBind ())
	return dispatchBind (pdu);
    else if (pdu.isAuth3 ())
	return dispatchAuth3 (pdu);

    return 0;
    }

int RpcEventHandler::sendPdu (RpcPdu& pdu)
    {
    TRACE_CALL;

    size_t len;
    char *buf = 0;
    int status = -1;

    S_DEBUG (LOG_RPC, "sendPdu: " << pdu);

    if (pdu.makeReplyBuffer (buf, len) < 0)
	stream().close ();
    else if (stream().send (buf, len) == len)
	{
	stream ().processDebugOutput (pRpcServerOutput, (BYTE *)buf, len);
	status = 0;
	}

    delete [] buf;

    return status;
    }

int
RpcEventHandler::reply (const RpcPdu& pdu, RpcPdu& replyPdu)
    {
    TRACE_CALL;

    replyPdu.drepSet (pdu.drep ());

    if (replyPdu.isFault ())
	{
	S_ERR (LOG_RPC, "RxREQUEST:" << pdu);
	S_ERR (LOG_RPC, "TxFAULT:" << replyPdu);
	}
    
    // send reply
    return sendPdu (replyPdu);
    }

int
RpcEventHandler::channelId () const
    {
    TRACE_CALL;
    return reinterpret_cast<int> (this);
    }

int RpcEventHandler::concurrency ()
    {	
    TRACE_CALL;
    return acceptorGet()->concurrency ();
    }

ThreadPool* RpcEventHandler::threadPool ()
    {
    TRACE_CALL;
    return acceptorGet()->threadPool ();
    }

RpcIfServer* RpcEventHandler::acceptorGet () const
    {
    TRACE_CALL;
    return m_acceptor;
    }

RpcIfServer* RpcEventHandler::acceptorSet (RpcIfServer* pRpcIfServer)
    {
    TRACE_CALL;
    return m_acceptor = pRpcIfServer;
    }
    
ostream& operator<< (ostream& os, const RpcEventHandler& eh)
    {
    RpcEventHandler* p = const_cast<RpcEventHandler*> (&eh);

    os << p->hostAddr ()
       << " => "
       << p->peerAddr ()
       << " (fd="
       << p->handleGet ()
       << ") ";

    return os;
    }

